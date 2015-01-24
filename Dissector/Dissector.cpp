#include "Dissector.h"
#include <string>
#include <assert.h>
#include <fstream>
#include <list>
#include <stdarg.h>
#include "DissectorHelpers.h"

static const int EVENT_TYPE_SIZE = 32*1024; // 32k for storing event state types
static const int EVENT_NAME_SIZE = 32*1024; // 32k for storing event state type names

static const int DRAW_CALL_DATA_SIZE = 256*1024; // 256k for storing draw call structures. Room for ~11k draw calls.

static const int kMaxEventTypes  = EVENT_TYPE_SIZE / sizeof(Dissector::EventType);
static const int kMaxCaptureSize = DRAW_CALL_DATA_SIZE / sizeof(Dissector::DrawCallData);

// ------------------------------------------------------------------------------------
// Static Data -- All this is done so we don't have to dynamically allocate memory and
//                deal with custom memory managers.
// ------------------------------------------------------------------------------------
struct DissectorData
{
    DissectorData()
    {
        mBigBuffer      = 0;
        mBigBufferIter  = 0;
        mBigBufferReset = 0;
        mBigBufferEnd   = 0;

        mCaptureData    = 0;
        mCaptureSize    = 0;

        mDrawCallLimit  = -1;

        mEnumData = 0;
        mShouldStartCapture = 0;
        mInSlaveMode = 0;

        mMallocCallback = NULL;
        mFreeCallback = NULL;
        mSendMessageCallback = NULL;
        mNetworkFrameUpdate = NULL;
        mSlaveEnded = NULL;
        mCallbacks = NULL;
    }

    char*                        mBigBuffer;
    char*                        mBigBufferIter;
    char*                        mBigBufferReset;
    char*                        mBigBufferEnd;

    char*                        mStateTypeBuffer;
    int                          mStateTypeBufferSize;

    Dissector::EventType*        mEventTypes;
    char*                        mEventNames;
    char*                        mEventNamesEnd;
    int                          mNumEventTypes;

    Dissector::DrawCallData*     mCaptureData;
    int                          mCaptureSize;

    int                          mDrawCallLimit;

    int                          mInSlaveMode;
    int                          mShouldExitCapture;
    int                          mShouldStartCapture;

    char*                        mEnumData;


    struct CommandData
    {
        int     mCommand;
        int     mDataSize;
        void*   mData;
    };

    std::list<CommandData>       mCommands;

    void* (*mMallocCallback)(size_t size);
    void  (*mFreeCallback)(void* ptr);
    void (*mSendMessageCallback)(const char* iMessage, unsigned int iSize);
    void (*mNetworkFrameUpdate)();
    void (*mSlaveEnded)();

    Dissector::RenderCallbacks* mCallbacks;
};

static DissectorData sDissectorData;

namespace Dissector
{
    void Initialize( char* iMemory, int iMemorySize )
    {
        sDissectorData.mBigBuffer     = (char*)iMemory;
        sDissectorData.mBigBufferIter = iMemory;
        sDissectorData.mBigBufferEnd  = iMemory + iMemorySize;

        // Dole out memory blocks from our big block to each buffer
        sDissectorData.mEventTypes     = (Dissector::EventType*)sDissectorData.mBigBufferIter;
        sDissectorData.mBigBufferIter += EVENT_TYPE_SIZE;
        sDissectorData.mNumEventTypes  = 0;

        sDissectorData.mEventNames = sDissectorData.mBigBufferIter;
        sDissectorData.mBigBufferIter += EVENT_NAME_SIZE;
        sDissectorData.mEventNamesEnd = sDissectorData.mBigBufferIter;

        sDissectorData.mCaptureData = (DrawCallData*)sDissectorData.mBigBufferIter;
        sDissectorData.mBigBufferIter += DRAW_CALL_DATA_SIZE;
        sDissectorData.mCaptureSize = 0;

        // Rest of the buffer will be used for captures.
        sDissectorData.mBigBufferReset = sDissectorData.mBigBufferIter;
        sDissectorData.mBigBufferIter = NULL;

        sDissectorData.mStateTypeBuffer = NULL;
        sDissectorData.mStateTypeBufferSize = 0;
    }

    void Shutdown()
    {
        sDissectorData.mBigBuffer      = NULL;
        sDissectorData.mBigBufferIter  = NULL;
        sDissectorData.mBigBufferEnd   = NULL;
        sDissectorData.mCaptureData    = 0;
        sDissectorData.mCaptureSize    = 0;

        if( sDissectorData.mStateTypeBuffer )
        {
            FreeCallback( sDissectorData.mStateTypeBuffer );
            sDissectorData.mStateTypeBuffer = NULL;
        }

        while( sDissectorData.mEnumData )
        {
            // Free linked list.
            char* item = sDissectorData.mEnumData;
            sDissectorData.mEnumData = *(char**)item;
            FreeCallback( item );
        }
    }

    // ------------------------------------------------------------------------------------
    // Messaging Functions
    // ------------------------------------------------------------------------------------
    char greetMessage[] = "HELO";
    char commandMessage[] = "COMD";
    char byeMessage[] = "LATR";

    void SendResponse( int iType, ... ) /*takes pairs of const char* iData, unsigned int iSize. Then end with a 0 */
    {
        static const unsigned int headerSize = sizeof(unsigned int) * 3;
        char header[ headerSize ];
        char *headerIter = header;
        unsigned int begin = Dissector::PACKET_BEGIN;
        memcpy( headerIter, &begin, sizeof(unsigned int) );
        headerIter += sizeof(unsigned int);

        unsigned int totalSize = sizeof(unsigned int);
        {
            va_list argList;
            va_start( argList, iType );
            const char* dataPtr = va_arg( argList, const char* );
            while( dataPtr )
            {
                unsigned int dataSize = va_arg( argList, unsigned int );
                totalSize += dataSize;
                dataPtr = va_arg( argList, const char* );
            }

            va_end( argList );
        }

        memcpy( headerIter, &totalSize, sizeof(unsigned int) );
        headerIter += sizeof(unsigned int);

        memcpy( headerIter, &iType, sizeof(int) );
        headerIter += sizeof(int);

        sDissectorData.mSendMessageCallback( header, headerSize );

        {
            va_list argList;
            va_start( argList, iType );
            const char* dataPtr = va_arg( argList, const char* );
            while( dataPtr )
            {
                unsigned int dataSize = va_arg( argList, unsigned int );
                sDissectorData.mSendMessageCallback( dataPtr, dataSize );
                dataPtr = va_arg( argList, const char* );
            }

            va_end( argList );
        }

    }

    void ReceiveMessage( char* iMessage, unsigned int iSize )
    {
        if( iSize >= 4 )
        {
            char msg[5];
            for( int ii = 0; ii < 4; ++ii )
                msg[ii] = iMessage[ii];
            msg[4] = 0;

            if( strcmp( msg, greetMessage ) == 0 )
            {
                // Connection established.
            }
            else if( strcmp( msg, commandMessage ) == 0 )
            {
                DissectorData::CommandData cd;
                iMessage += 4;
                cd.mCommand = *(int*)iMessage;
                iMessage += 4;
                cd.mDataSize = *(int*)iMessage;
                iMessage += 4;

                if( cd.mDataSize )
                {
                    cd.mData = MallocCallback( cd.mDataSize );
                    memcpy( cd.mData, iMessage, cd.mDataSize );
                }
                else
                {
                    cd.mData = NULL;
                }

                if( cd.mCommand == CMD_CANCELOUTSTANDINGTHUMBNAILS )
                {
                    for( std::list<DissectorData::CommandData>::iterator iter = sDissectorData.mCommands.begin();
                        iter != sDissectorData.mCommands.end(); )
                    {
                        if( iter->mCommand == CMD_GETTHUMBNAIL )
                        {
                            iter = sDissectorData.mCommands.erase( iter );
                        }
                        else
                        {
                            ++iter;
                        }
                    }
                }
                else
                {
                    sDissectorData.mCommands.push_back( cd );
                }
            }
            else if( strcmp( msg, byeMessage ) == 0 )
            {
                // Other side is closing socket.
            }
        }
    }

    // ------------------------------------------------------------------------------------
    // Functions for platforms
    // ------------------------------------------------------------------------------------
    Dissector::DrawCallData* GetCaptureData()
    {
        return sDissectorData.mCaptureData;
    }

    int GetCaptureSize()
    {
        return sDissectorData.mCaptureSize;
    }

    // ------------------------------------------------------------------------------------
    // Frame functions
    // ------------------------------------------------------------------------------------
    bool ToolWantsToCapture()
    {
        return sDissectorData.mShouldStartCapture != 0;
    }

    void TriggerCapture()
    {
        sDissectorData.mCaptureSize    = 0;
        sDissectorData.mBigBufferIter  = sDissectorData.mBigBufferReset;
        sDissectorData.mShouldStartCapture = 0;
        sDissectorData.mShouldExitCapture = 0;
    }

    void TriggerCaptureStop()
    {
        sDissectorData.mShouldExitCapture = 1;
    }

    void BeginFrame( void* iDevice )
    {
        sDissectorData.mCallbacks->BeginCaptureFrame( iDevice );
    }

    void EndFrame( void* iDevice )
    {
        if( sDissectorData.mBigBufferIter )
        {
            sDissectorData.mCallbacks->EndCaptureFrame( iDevice );
            sDissectorData.mBigBufferIter  = NULL;
        }
    }

    TimingData* GatherTimingData( void* iDevice )
    {
        TimingData* tdata = (TimingData*)MallocCallback( sDissectorData.mCaptureSize * sizeof( TimingData ) );

        sDissectorData.mCallbacks->ResimulateStart( iDevice );

        DrawCallData* iterEnd = &sDissectorData.mCaptureData[ sDissectorData.mCaptureSize ];
        TimingData* timingIter = tdata;
        for( DrawCallData* iter = sDissectorData.mCaptureData; iter != iterEnd; ++iter, ++timingIter )
        {
            if( iter->mEventType >= 0 )
            {
                sDissectorData.mCallbacks->ResimulateEvent( iDevice, *iter, true, timingIter );
            }
        }

        sDissectorData.mCallbacks->ResimulateEnd( iDevice );

        return tdata;
    }

    void ResimulateFrame( void* iDevice, bool iUseFrameLimit )
    {
        sDissectorData.mCallbacks->ResimulateStart( iDevice );

        int limit = (sDissectorData.mDrawCallLimit >= 0 && iUseFrameLimit) ?
            std::min( 1+sDissectorData.mDrawCallLimit, sDissectorData.mCaptureSize ) : sDissectorData.mCaptureSize;

        DrawCallData* iterEnd = &sDissectorData.mCaptureData[ limit ];
        for( DrawCallData* iter = sDissectorData.mCaptureData; iter != iterEnd; ++iter )
        {
            if( iter->mEventType >= 0 )
            {
                sDissectorData.mCallbacks->ResimulateEvent( iDevice, *iter, false, NULL );
            }
        }

        sDissectorData.mCallbacks->ResimulateEnd( iDevice );
    }

    void DebugShader( void* iDevice, char* iData, unsigned int iDataSize )
    {
        const unsigned int dataOffset = 2 * sizeof(unsigned int);
        unsigned int iShaderType = *(unsigned int*)iData;
        unsigned int iEventId = *(unsigned int*)(iData + sizeof(unsigned int));
        if( (int)iEventId >= sDissectorData.mCaptureSize )
            return;

        sDissectorData.mCallbacks->ResimulateStart( iDevice );

        DrawCallData* iterEnd = &sDissectorData.mCaptureData[ 1 + iEventId ];
        DrawCallData* debugIter = &sDissectorData.mCaptureData[ iEventId ];
        for( DrawCallData* iter = sDissectorData.mCaptureData; iter != iterEnd; ++iter )
        {
            if( iter->mEventType >= 0 )
            {
                sDissectorData.mCallbacks->ResimulateEvent( iDevice, *iter, false, NULL );
            }
        }

        sDissectorData.mCallbacks->ShaderDebug( iDevice, iEventId, *debugIter, iShaderType, iData + dataOffset, iDataSize - dataOffset );

        sDissectorData.mCallbacks->ResimulateEnd( iDevice );
    }

    void GatherEventStatistics( void* iDevice )
    {

    }

    void SendStateTypes()
    {
        SendResponse( Dissector::RSP_STATETYPES, sDissectorData.mStateTypeBuffer, sDissectorData.mStateTypeBufferSize, 0 );
    }

    void SendEventList()
    {
        size_t bufferLen = 32*1024;
        char* outData = (char*)MallocCallback( bufferLen );
        if( !outData )
            return;

        char* dataIter = outData;
        char* dataIterEnd = outData + bufferLen;
        char tempData[512];

        DrawCallData* iterEnd = &sDissectorData.mCaptureData[ sDissectorData.mCaptureSize ];
        for( DrawCallData* iter = sDissectorData.mCaptureData; iter != iterEnd; ++iter )
        {
            size_t dataLen = 0;
            const char* dataPtr = NULL;
            if( iter->mEventType == -1 )
            {
                dataLen = strlen( iter->mDrawCallData ) + 1;  // Need to copy null char
                dataPtr = iter->mDrawCallData;
            }
            else if( iter->mEventType == -2 )
            {
                // End group, send no extra data.
            }
            else if( iter->mEventType >= 0 )
            {
                sDissectorData.mCallbacks->GetEventText( iter, sizeof(tempData), tempData, dataLen );
                if( dataLen != 0 )
                {
                    dataPtr = tempData;
                }
                else
                {
                    dataLen = strlen( sDissectorData.mEventTypes[ iter->mEventType ].mName ) + 1; // Need to copy null char
                    dataPtr = sDissectorData.mEventTypes[ iter->mEventType ].mName;
                }
            }

            if( (dataIter + sizeof(int) + sizeof(size_t) + dataLen) >= dataIterEnd ) 
            {
                // Buffer needs to grow
                size_t bufferLen = dataIterEnd - outData;
                size_t newBufferLen = 2*bufferLen;

                char* newData = (char*)MallocCallback( newBufferLen );
                if( !newData )
                {
                    FreeCallback( outData );
                    return;
                }

                memcpy( newData, outData, bufferLen );

                dataIter = newData + (dataIter - outData);
                bufferLen = newBufferLen;
                dataIterEnd = newData + bufferLen;
                FreeCallback( outData );

                outData = newData;
            }

            StoreBufferData( iter->mEventType, dataIter );
            StoreBufferData( dataLen, dataIter );

            if( dataLen )
            {
                memcpy( dataIter, dataPtr, dataLen );
                dataIter += dataLen;
            }
        }

        assert( dataIter <= dataIterEnd );

        SendResponse( Dissector::RSP_EVENTLIST, outData, dataIter - outData, 0 );

        FreeCallback( outData );
    }

    void SendEventInfo( int iEventNum )
    {
        // Count up the modified render states sizes.
        unsigned int toolDataSize = 0;
        for( ToolDataItem* iter = sDissectorData.mCaptureData[iEventNum].mToolRenderStates; iter; iter = iter->nextItem )
        {
            toolDataSize += sizeof( RenderState );
            toolDataSize += iter->stateData.mSize;
        }

        ScopedFree<char> buffer = toolDataSize ? (char*)MallocCallback( toolDataSize ) : NULL;

        if( toolDataSize && buffer.mPtr )
        {
            char* bufIter = buffer.mPtr;

            for( ToolDataItem* iter = sDissectorData.mCaptureData[iEventNum].mToolRenderStates; iter; iter = iter->nextItem )
            {
                StoreBufferData< RenderState >( iter->stateData, bufIter );
                memcpy( bufIter, ((char*)iter) + sizeof( ToolDataItem ), iter->stateData.mSize );
                bufIter += iter->stateData.mSize;
            }
        }

        SendResponse( Dissector::RSP_EVENTINFO,
            (char*)&iEventNum, sizeof(int),
            (char*)sDissectorData.mCaptureData[iEventNum].mFirstRenderState,
            sDissectorData.mCaptureData[iEventNum].mSizeRenderStateData,
            buffer.mPtr, toolDataSize,
            0 );
    }

    void SendEnumInfo()
    {
        // Gather size of all the enums
        int size = 0;
        char* enumIter = sDissectorData.mEnumData;
        while( enumIter )
        {
            size += *(unsigned int*)(enumIter + sizeof(char*));
            size -= sizeof(char*) + sizeof(unsigned int); // Don't send over the linked list pointers, or size blocks.
            enumIter = *(char**)enumIter;
        }

        char* data = (char*)MallocCallback( size );
        char* dataIter = data;

        enumIter = sDissectorData.mEnumData;
        while( enumIter )
        {
            unsigned int enumSize = *(unsigned int*)(enumIter + sizeof(char*)) - sizeof(unsigned int) - sizeof(char*);
            char* enumData = enumIter + sizeof(char*) + sizeof(unsigned int);

            memcpy( dataIter, enumData, enumSize );

            dataIter += enumSize;
            enumIter = *(char**)enumIter;
        }

        SendResponse( Dissector::RSP_ENUMTYPES, data, size, 0 );

        FreeCallback( data );
    }

    void SendCurrentRT( void* iDevice, void* iMessageData )
    {
        sDissectorData.mCallbacks->GetCurrentRT( iDevice );
    }

    void SendImage( void* iDevice, void* iMessageData )
    {
        char* iData = (char*)iMessageData;
        __int64 id = GetBufferData< __int64 >( iData );
        void* texture = (void*)id;
        int eventNum = GetBufferData< int >( iData );
        unsigned int vistype = GetBufferData< unsigned int >( iData );

        sDissectorData.mCallbacks->GetTextureImage( iDevice, texture, vistype, eventNum );
    }
    
    void SendThumbnail( void* iDevice, void* iMessageData )
    {
        char* iData = (char*)iMessageData;
        __int64 id = GetBufferData< __int64 >( iData );
        void* texture = (void*)id;
        int eventNum = GetBufferData< int >( iData );
        unsigned int vistype = GetBufferData< unsigned int >( iData );

        sDissectorData.mCallbacks->GetTextureThumbnail( iDevice, texture, vistype, eventNum );
    }

    void TestPixelsWritten( void* iDevice, void* iMessageData, unsigned int iDataSize )
    {
        unsigned int iEventId = *(unsigned int*)iMessageData;
        unsigned int numPixels = *(unsigned int*)( ((char*)iMessageData) + sizeof(unsigned int) );
        PixelLocation* pixels =  (PixelLocation*)( ((char*)iMessageData) + 2*sizeof(unsigned int) );

        if( (int)iEventId >= sDissectorData.mCaptureSize )
            return;

        sDissectorData.mCallbacks->ResimulateStart( iDevice );

        DrawCallData* iterEnd = &sDissectorData.mCaptureData[ 1 + iEventId ];
        DrawCallData* debugIter = &sDissectorData.mCaptureData[ iEventId ];
        for( DrawCallData* iter = sDissectorData.mCaptureData; iter != iterEnd; ++iter )
        {
            if( iter->mEventType >= 0 )
            {
                sDissectorData.mCallbacks->ResimulateEvent( iDevice, *iter, false, NULL );
            }
        }

        sDissectorData.mCallbacks->TestPixelsWritten( iDevice, *debugIter, pixels, numPixels );

        sDissectorData.mCallbacks->ResimulateEnd( iDevice );
    }

    void TestPixelFailure( void* iDevice, void* iMessageData, unsigned int iDataSize )
    {
        unsigned int iEventId = *(unsigned int*)iMessageData;
        PixelLocation* pixel   =  (PixelLocation*)( ((char*)iMessageData) + sizeof(unsigned int) );

        if( (int)iEventId >= sDissectorData.mCaptureSize )
            return;

        struct 
        {
            unsigned int numFailures;
            unsigned int fails[4];
        } data;

        data.numFailures = sizeof( data.fails ) / sizeof(unsigned int);
        DrawCallData* debugIter = &sDissectorData.mCaptureData[ iEventId ];
        sDissectorData.mCallbacks->TestPixelFailure( iDevice, iEventId, *pixel, data.fails );
        SendResponse( Dissector::RSP_TESTPIXELFAILURE, (char*)&data, sizeof(data), 0 );
    }

    void OverrideRenderState( void* iDevice, void* iMessageData, unsigned int iDataSize )
    {
        char* iData = (char*)iMessageData;
        int iEventId = GetBufferData< int >( iData );
        RenderState rs = GetBufferData< RenderState >( iData );
        ToolDataItem* dataPtr = (ToolDataItem*)MallocCallback( sizeof( ToolDataItem ) + rs.mSize );
        if( iEventId < sDissectorData.mCaptureSize )
        {
            DrawCallData& dc = sDissectorData.mCaptureData[iEventId];
            dataPtr->nextItem = dc.mToolRenderStates;
            dc.mToolRenderStates = dataPtr;
            dataPtr->stateData = rs;
            memcpy( ((char*)dataPtr) + sizeof(ToolDataItem), iData, rs.mSize );

            // Delete any other overrides of this type.
            ToolDataItem* lastIter = dc.mToolRenderStates;
            ToolDataItem* iter = dc.mToolRenderStates->nextItem;
            while( iter )
            {
                if( iter->stateData.mType == rs.mType )
                {
                    lastIter->nextItem = iter->nextItem;
                    FreeCallback( iter );
                    break;
                }
                iter = iter->nextItem;
            }
        }
    }

    void ClearRenderStateOverrides( void* iDevice, void* iMessageData, unsigned int iDataSize )
    {
        char* iData = (char*)iMessageData;
        int iEventId = GetBufferData< int >( iData );
        int iTypeId = GetBufferData< int >( iData );

        int eventStart = (iEventId == -1) ? 0 : iEventId;
        int eventEnd   = (iEventId == -1) ? sDissectorData.mCaptureSize : iEventId + 1;

        if( iTypeId == -1 )
        {
            for( int ii = eventStart; ii < eventEnd; ++ii )
            {
                ToolDataItem* iter = sDissectorData.mCaptureData[ii].mToolRenderStates;
                while( iter )
                {
                    ToolDataItem* deleteItem = iter;
                    iter = iter->nextItem;
                    FreeCallback( deleteItem );
                }

                sDissectorData.mCaptureData[ii].mToolRenderStates = NULL;
            }
        }
        else
        {
            for( int ii = eventStart; ii < eventEnd; ++ii )
            {
                ToolDataItem* lastIter = NULL;
                ToolDataItem* iter = sDissectorData.mCaptureData[ii].mToolRenderStates;
                while( iter )
                {
                    ToolDataItem* deleteItem = iter;
                    iter = iter->nextItem;
                    if( deleteItem->stateData.mType == iTypeId )
                    {
                        if( lastIter )
                        {
                            lastIter->nextItem = iter;
                        }
                        else
                        {
                            sDissectorData.mCaptureData[ii].mToolRenderStates = iter;
                        }

                        FreeCallback( deleteItem );
                    }
                    else
                    {
                        lastIter = deleteItem;
                    }
                }
            }
        }

        SendResponse( RSP_CLEARRENDERSTATEOVERRIDE, 0 );
    }

    void RetrieveMesh( void* iDevice, void* iMessageData, unsigned int iDataSize )
    {
        char* iData = (char*)iMessageData;
        int iEventId = GetBufferData< int >( iData );
        int iTypeId = GetBufferData< int >( iData );

        sDissectorData.mCallbacks->GetMesh( iDevice, iEventId, iTypeId );
    }

    void CheckCommands( void* iDevice )
    {
        while( !sDissectorData.mCommands.empty() )
        {
            bool shouldBreak = false;
            DissectorData::CommandData cd = sDissectorData.mCommands.front();
            sDissectorData.mCommands.pop_front();
            switch( cd.mCommand )
            {
            case( CMD_TRIGGERCAPTURE ): sDissectorData.mShouldStartCapture = 1; break;
            case( CMD_RESUME ): sDissectorData.mShouldExitCapture = 1; break;
            case( CMD_GETEVENTLIST ):  SendEventList(); break;
            case( CMD_GETSTATETYPES ): SendStateTypes(); break;
            case( CMD_GETENUMTYPES ):  SendEnumInfo(); break;
            }

            if( sDissectorData.mInSlaveMode )
            {
                switch( cd.mCommand )
                {
                case( CMD_GETALLTIMINGS ): break;
                case( CMD_GETTIMING ):     break;
                case( CMD_GOTOCALL ):      sDissectorData.mDrawCallLimit = *(int*)cd.mData; shouldBreak = true; break;
                case( CMD_GETEVENTINFO ):  SendEventInfo( *(int*)cd.mData ); break;
                case( CMD_GETIMAGE ):      SendImage( iDevice, cd.mData ); break;
                case( CMD_GETCURRENTRENDERTARGET ): SendCurrentRT( iDevice, cd.mData ); break;
                case( CMD_GETTHUMBNAIL ):  SendThumbnail( iDevice, cd.mData ); break;
                case( CMD_DEBUGSHADER ):   DebugShader( iDevice, (char*)cd.mData, cd.mDataSize ); break;
                case( CMD_TESTPIXELSWRITTEN ): TestPixelsWritten( iDevice, (char*)cd.mData, cd.mDataSize ); break;
                case( CMD_TESTPIXELFAILURE ): TestPixelFailure( iDevice, (char*)cd.mData, cd.mDataSize ); break;
                case( CMD_OVERRIDERENDERSTATE ): OverrideRenderState( iDevice, (char*)cd.mData, cd.mDataSize ); break;
                case( CMD_CLEARRENDERSTATEOVERRIDE ): ClearRenderStateOverrides( iDevice, (char*)cd.mData, cd.mDataSize ); break;
                case( CMD_GETMESH ): RetrieveMesh( iDevice, (char*)cd.mData, cd.mDataSize ); break;
                }

                if( sDissectorData.mNetworkFrameUpdate )
                {
                    // Check this every command in case a cancel message came in.
                    sDissectorData.mNetworkFrameUpdate();
                }
            }
            
            if( cd.mData )
            {
                FreeCallback( cd.mData );
            }

            if( shouldBreak )
                break;
        }

        if( sDissectorData.mNetworkFrameUpdate )
        {
            sDissectorData.mNetworkFrameUpdate();
        }
    }

    void FrameUpdate( void* iDevice )
    {
        CheckCommands( iDevice );
    }

    void StartSlave( void* iDevice )
    {
        SendResponse( Dissector::RSP_CAPTURECOMPLETED, NULL );

        sDissectorData.mInSlaveMode = 1;
        sDissectorData.mCallbacks->SlaveBegin( iDevice );
        while( !sDissectorData.mShouldExitCapture )
        {
            CheckCommands( iDevice );
            ResimulateFrame( iDevice, true );
        }

        sDissectorData.mInSlaveMode = 0;
        sDissectorData.mCallbacks->SlaveEnd( iDevice );

        if( sDissectorData.mSlaveEnded )
            sDissectorData.mSlaveEnded();

        // Free all the tool allocated render states now that it's stopping.
        for( int ii = 0; ii < sDissectorData.mCaptureSize; ++ii )
        {
            DrawCallData& dc = sDissectorData.mCaptureData[ ii ];
            ToolDataItem* iter = dc.mToolRenderStates;
            while( iter )
            {
                ToolDataItem* old = iter;
                iter = iter->nextItem;
                FreeCallback( old );
            }
        }
    }

    bool IsCapturing()
    {
        return sDissectorData.mBigBufferIter != NULL;
    }

    // Draw call registering functions
    void StartDrawCallGroup( const char* iName )
    {
        if( sDissectorData.mBigBufferIter )
        {
            sDissectorData.mCaptureData[ sDissectorData.mCaptureSize ].mFirstRenderState = 0;
            sDissectorData.mCaptureData[ sDissectorData.mCaptureSize ].mSizeDrawCallData = 0;
            sDissectorData.mCaptureData[ sDissectorData.mCaptureSize ].mSizeRenderStateData = 0;
            sDissectorData.mCaptureData[ sDissectorData.mCaptureSize ].mEventType = -1;
            sDissectorData.mCaptureData[ sDissectorData.mCaptureSize ].mToolRenderStates = 0;
            sDissectorData.mCaptureData[ sDissectorData.mCaptureSize ].mDrawCallData = sDissectorData.mBigBufferIter;

            int nameLen = strlen( iName );
            assert( (sDissectorData.mBigBufferIter + nameLen) < sDissectorData.mBigBufferEnd ); // OOM in the big buffer
            memcpy( sDissectorData.mBigBufferIter, iName, nameLen+1 );
            sDissectorData.mBigBufferIter += nameLen+1;
            ++sDissectorData.mCaptureSize;
        }
    }

    void EndDrawCallGroup()
    {
        if( sDissectorData.mBigBufferIter )
        {
            sDissectorData.mCaptureData[ sDissectorData.mCaptureSize ].mFirstRenderState = 0;
            sDissectorData.mCaptureData[ sDissectorData.mCaptureSize ].mSizeDrawCallData = 0;
            sDissectorData.mCaptureData[ sDissectorData.mCaptureSize ].mSizeRenderStateData = 0;
            sDissectorData.mCaptureData[ sDissectorData.mCaptureSize ].mEventType = -2;
            sDissectorData.mCaptureData[ sDissectorData.mCaptureSize ].mDrawCallData = 0;
            ++sDissectorData.mCaptureSize;
        }
    }

    void RegisterEvent( void* iDevice, int iEventType, const void* iEventData, unsigned int iDataSize )
    {
        if( sDissectorData.mBigBufferIter )
        {
            assert( (sDissectorData.mBigBufferIter + iDataSize) < sDissectorData.mBigBufferEnd );
            sDissectorData.mCaptureData[ sDissectorData.mCaptureSize ].mSizeDrawCallData = iDataSize;
            sDissectorData.mCaptureData[ sDissectorData.mCaptureSize ].mDrawCallData = sDissectorData.mBigBufferIter;
            sDissectorData.mCaptureData[ sDissectorData.mCaptureSize ].mEventType = iEventType;

            memcpy( sDissectorData.mBigBufferIter, iEventData, iDataSize );
            sDissectorData.mBigBufferIter += iDataSize;

            sDissectorData.mCallbacks->FillRenderStates( iDevice, iEventType, iEventData, iDataSize );
            ++sDissectorData.mCaptureSize;
        }
    }

    void AddRenderStateEntries( void* iRenderStateData, unsigned int iDataSize )
    {
        assert( (sDissectorData.mBigBufferIter + iDataSize ) < sDissectorData.mBigBufferEnd );
        sDissectorData.mCaptureData[ sDissectorData.mCaptureSize ].mSizeRenderStateData = iDataSize;
        sDissectorData.mCaptureData[ sDissectorData.mCaptureSize ].mFirstRenderState = (Dissector::RenderState*)sDissectorData.mBigBufferIter;

        memcpy( sDissectorData.mBigBufferIter, iRenderStateData, iDataSize );
        sDissectorData.mBigBufferIter += iDataSize;
    }

    void AddEventStats( const QueryData* iData, unsigned int iNumData )
    {
    }

    // ------------------------------------------------------------------------------------
    // Platform functions
    // ------------------------------------------------------------------------------------
    void ClearTypeInformation()
    {
        sDissectorData.mNumEventTypes = 0;
        if( sDissectorData.mStateTypeBuffer )
        {
            FreeCallback( sDissectorData.mStateTypeBuffer );
            sDissectorData.mStateTypeBuffer = NULL;
        }
        sDissectorData.mStateTypeBufferSize = NULL;

        while( sDissectorData.mEnumData )
        {
            // Free linked list.
            char* item = sDissectorData.mEnumData;
            sDissectorData.mEnumData = *(char**)item;
            FreeCallback( item );
        }
    }

    void RegisterEventType( unsigned int iType, unsigned int iFlags, const char* iName )
    {
        int nameLen = strlen( iName );
        assert( (sDissectorData.mEventNames + nameLen) < sDissectorData.mEventNamesEnd );

        sDissectorData.mEventTypes[ sDissectorData.mNumEventTypes ].mFlags = iFlags;
        sDissectorData.mEventTypes[ sDissectorData.mNumEventTypes ].mType = iType;
        sDissectorData.mEventTypes[ sDissectorData.mNumEventTypes ].mName = sDissectorData.mEventNames;
        memcpy( sDissectorData.mEventNames, iName, nameLen+1 ); // +1 is for 0 terminator

        sDissectorData.mEventNames += nameLen+1;
        ++sDissectorData.mNumEventTypes;
    }


#define CHECK_FOR_DUPE_STATE_TYPES
    void RegisterRenderStateTypes( RenderStateType* iTypes, unsigned int iNumTypes )
    {
        int allocSize = 0;

        RenderStateType* iter = iTypes;
        RenderStateType* iterEnd = iTypes + iNumTypes;

#ifdef CHECK_FOR_DUPE_STATE_TYPES
        size_t* typeCheck = (size_t*)MallocCallback( sizeof(size_t)*iNumTypes*2 );
        memset( typeCheck, 0, sizeof(size_t)*iNumTypes*2 );
#endif
        for( iter = iTypes; iter != iterEnd; ++iter )
        {
#ifdef CHECK_FOR_DUPE_STATE_TYPES
            if( iter->mType != -1 )
            {
                assert( iter->mType < iNumTypes*2 );
                if( typeCheck[iter->mType] )
                {
                    assert( false );
                }

                typeCheck[iter->mType] = iter - iTypes;
            }
#endif

            allocSize += sizeof(unsigned int) * 4;
            allocSize += iter->mName ? strlen( iter->mName ) : 0;
        }

#ifdef CHECK_FOR_DUPE_STATE_TYPES
        FreeCallback( typeCheck );
        typeCheck = NULL;
#endif

        char* insertPointer;
        if( sDissectorData.mStateTypeBuffer )
        {
            char* oldBuffer = sDissectorData.mStateTypeBuffer;
            unsigned int oldSize = sDissectorData.mStateTypeBufferSize;

            sDissectorData.mStateTypeBufferSize += allocSize;
            sDissectorData.mStateTypeBuffer = (char*)MallocCallback( sDissectorData.mStateTypeBufferSize );
            memcpy( sDissectorData.mStateTypeBuffer, oldBuffer, oldSize );
            insertPointer = sDissectorData.mStateTypeBuffer + oldSize;
        }
        else
        {
            sDissectorData.mStateTypeBufferSize = allocSize;
            sDissectorData.mStateTypeBuffer = (char*)MallocCallback( sDissectorData.mStateTypeBufferSize );
            insertPointer = sDissectorData.mStateTypeBuffer;
        }

        for( iter = iTypes; iter != iterEnd; ++iter )
        {
            StoreBufferData( iter->mType, insertPointer );
            StoreBufferData( iter->mFlags, insertPointer );
            StoreBufferData( iter->mVisualizer, insertPointer );

            if( iter->mName )
            {
                unsigned int len = strlen( iter->mName );
                StoreBufferData( len, insertPointer );

                memcpy( insertPointer, iter->mName, len );
                insertPointer += len;
            }
            else
            {
                StoreBufferData< unsigned int >( 0, insertPointer );
            }
        }

        assert( insertPointer <= ( sDissectorData.mStateTypeBuffer + sDissectorData.mStateTypeBufferSize ) );
    }

    void RegisterRenderStateEnum( unsigned int iVisualizerId, ... )
    {
        va_list argList;

        int count = 0;
        int size = sizeof(unsigned int) * 3 + sizeof(char*);

        va_start( argList, iVisualizerId );

        const char* iter = va_arg( argList, const char* );
        unsigned int value;
        while( iter != NULL )
        {
            ++count;
            size += strlen( iter );
            size += sizeof(unsigned int)*2;
            value = va_arg( argList, unsigned int );
            iter = va_arg( argList, const char* );
        }

        va_end( argList );

        char* data = (char*)MallocCallback( size );
        char* baseData = data;

        *(char**)data = sDissectorData.mEnumData;
        sDissectorData.mEnumData = data;
        data += sizeof(char*);

        StoreBufferData( size, data );
        StoreBufferData( count, data );
        StoreBufferData( iVisualizerId, data );

        va_start( argList, iVisualizerId );

        iter = va_arg( argList, const char* );
        while( iter != NULL )
        {
            value = va_arg( argList, unsigned int );
            *(unsigned int*)data = value;
            StoreBufferData( value, data );

            unsigned int strsize = strlen(iter);
            StoreBufferData( strsize, data );

            memcpy( data, iter, strsize );
            data += strsize;

            iter = va_arg( argList, const char* );
        }

        assert( (baseData + size) == data );
        va_end( argList );
    }

    void CurrentRTRetrievalCallback( void* iImage, unsigned int iSizeX, unsigned int iSizeY, unsigned int iPitch )
    {
        char header[ 4 + 4 + 4 ];
        char* iter = header;
        StoreBufferData( iSizeX, iter );
        StoreBufferData( iSizeY, iter );
        StoreBufferData( iPitch, iter );

        SendResponse( Dissector::RSP_CURRENTRENDERTARGET, header, sizeof(header), 
            iImage, iSizeY * iPitch, 0 );
    }

    void ImageRetrievalCallback( void* iTexture, unsigned int iRSType, void* iImage, unsigned int iSizeX,
                                 unsigned int iSizeY, unsigned int iPitch )
    {
        char header[ 8 + 4 + 4 + 4 + 4 ];
        char* iter = header;
        StoreBufferData( __int64(iTexture), iter );
        StoreBufferData( iRSType, iter );
        StoreBufferData( iSizeX, iter );
        StoreBufferData( iSizeY, iter );
        StoreBufferData( iPitch, iter );

        SendResponse( Dissector::RSP_IMAGE, header, sizeof(header), 
            iImage, iSizeY * iPitch, 0 );
    }

    void ThumbnailRetrievalCallback( void* iTexture, unsigned int iVisualizerType, void* iImage, unsigned int iSizeX,
                                 unsigned int iSizeY, unsigned int iPitch, int iEventNum )
    {
        char header[ 8 + 4 + 4 + 4 + 4 + 4 ];
        char* iter = header;
        StoreBufferData( __int64(iTexture), iter );
        StoreBufferData( iVisualizerType, iter );
        StoreBufferData( iSizeX, iter );
        StoreBufferData( iSizeY, iter );
        StoreBufferData( iPitch, iter );
        StoreBufferData( iEventNum, iter );

        SendResponse( Dissector::RSP_THUMBNAIL, header, sizeof(header), 
            iImage, iSizeY * iPitch, 0 );
    }

    void ShaderDebugDataCallback( char* iDataBlob, unsigned int iDataSize, int iEventId, unsigned int loc0, unsigned int loc1 )
    {
        struct
        {
            int eventId;
            unsigned int loc0;
            unsigned int loc1;
        } message;

        message.eventId = iEventId;
        message.loc0 = loc0;
        message.loc1 = loc1;
        SendResponse( Dissector::RSP_SHADERDEBUGINFO, &message, sizeof(message), iDataBlob, iDataSize, 0 );
    }

    void ShaderDebugFailedCallback( int iEventId, unsigned int loc0, unsigned int loc1 )
    {
        struct
        {
            int eventId;
            unsigned int loc0;
            unsigned int loc1;
        } message;

        message.eventId = iEventId;
        message.loc0 = loc0;
        message.loc1 = loc1;
        SendResponse( Dissector::RSP_SHADERDEBUGFAILED, &message, sizeof(message), 0 );
    }


    // Call from GetMeshCallback to supply the mesh.
    void MeshDebugDataCallback( unsigned int iEventId, Dissector::PrimitiveType::Type iType,
        unsigned int iNumPrims, char* iVertsBlob, unsigned int iVertsSize, char* iIndices, unsigned int iIndicesSize )
    {
        struct
        {
            unsigned int eventId;
            unsigned int primtype;
            unsigned int numPrims;
            unsigned int vertSize;
            unsigned int indexSize;
        } messageHeader;
        messageHeader.eventId = iEventId;
        messageHeader.primtype = iType;
        messageHeader.numPrims = iNumPrims;
        messageHeader.vertSize = iVertsSize;
        messageHeader.indexSize = iIndicesSize;

        if( iIndicesSize )
            SendResponse( Dissector::RSP_MESH, &messageHeader, sizeof(messageHeader), iVertsBlob, iVertsSize, iIndices, iIndicesSize, 0 );
        else
            SendResponse( Dissector::RSP_MESH, &messageHeader, sizeof(messageHeader), iVertsBlob, iVertsSize, 0 );
    }

    // ------------------------------------------------------------------------------------
    // Platform Callbacks
    // ------------------------------------------------------------------------------------
    void SetMallocCallback( void* (*iMallocCallback)(size_t size) )
    {
        sDissectorData.mMallocCallback = iMallocCallback;
    }

    void SetFreeCallback( void (*iFreeCallback)(void* ptr) )
    {
        sDissectorData.mFreeCallback = iFreeCallback;
    }

    void SetSendMessageCallback( void (*iSendMessage)(const char* iMessage, unsigned int iSize) )
    {
        sDissectorData.mSendMessageCallback = iSendMessage;
    }

    void SetNetworkCallback( void (*iNetworkFrameUpdate)() )
    {
        sDissectorData.mNetworkFrameUpdate = iNetworkFrameUpdate;
    }

    void SetSlaveEndingCallback( void (*iSlaveEnded)() )
    {
        sDissectorData.mSlaveEnded = iSlaveEnded;
    }

    void SetRenderCallbacks( RenderCallbacks* iCallbacks )
    {
        sDissectorData.mCallbacks = iCallbacks;
    }

    // For other libs to use easily
    void* MallocCallback( size_t size )
    {
        return sDissectorData.mMallocCallback( size );
    }

    void FreeCallback( void* ptr )
    {
        sDissectorData.mFreeCallback( ptr );
    }

};

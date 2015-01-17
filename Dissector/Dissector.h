#pragma once
#ifndef DISSECTOR_H
#define DISSECTOR_H

namespace Dissector
{
    // Setup
    void Initialize( char* iMemory, int iMemorySize );
    void Shutdown();

    enum MagicNumbers
    {
        PACKET_BEGIN = 0x85BA573D,
    };

    enum ShaderTypes
    {
        ST_NONE = 0,
        ST_PIXEL,
        ST_VERTEX,
        ST_GEOMETRY,
        ST_COMPUTE,
        ST_DOMAIN,
        ST_HULL,
    };

    enum CommandTypes
    {
        CMD_NONE = 0,
        CMD_TRIGGERCAPTURE,
        CMD_RESUME,
        CMD_GETEVENTLIST,
        CMD_GETALLTIMINGS,
        CMD_GETTIMING,
        CMD_GOTOCALL,
        CMD_GETSTATETYPES,
        CMD_GETEVENTINFO,
        CMD_GETENUMTYPES,
        CMD_GETIMAGE,
        CMD_GETCURRENTRENDERTARGET,
        CMD_GETTHUMBNAIL,
        CMD_DEBUGSHADER,
        CMD_TESTPIXELSWRITTEN,
        CMD_TESTPIXELFAILURE,
        CMD_OVERRIDERENDERSTATE,
        CMD_CLEARRENDERSTATEOVERRIDE,
        CMD_GETTEXTURE,
        CMD_GETRENDERTARGET,
        CMD_GETMESH,
        CMD_CANCELOUTSTANDINGTHUMBNAILS,
    };

    enum ResponseTypes
    {
        RSP_EVENTLIST = 0,
        RSP_ALLTIMINGS,
        RSP_GETTIMING,
        RSP_STATETYPES,
        RSP_EVENTINFO,
        RSP_ENUMTYPES,
        RSP_CURRENTRENDERTARGET,
        RSP_IMAGE,
        RSP_THUMBNAIL,
        RSP_SHADERDEBUGINFO,
        RSP_TESTPIXELFAILURE,
        RSP_CLEARRENDERSTATEOVERRIDE,
        RSP_CAPTURECOMPLETED,
        RSP_SHADERDEBUGFAILED,
        RSP_MESH,
    };

    enum QueryType
    {
        QF_None = 0,
        QF_Timing,
        QF_NumPixels,
        QF_VertexUtilization,
        QF_PixelUtilization,
        QF_ROPUtilization,
        QF_TextureCacheUtilization,
        QF_VertexCacheUtilization,
        QF_TimeInVertexPercent,
        QF_TimeInPixelPercent,
        QF_TimeInOtherPercent,
        QF_TimeIdlePercent,
        QF_ShaderBandwidthPercent,
        QF_ShaderALUPercent,
    };

    struct QueryData
    {
        QueryType mType;
        float     mValue;
    };

    // Structures

    enum RenderStateVisualizerType
    {
        RSTF_VISUALIZE_BOOL,
        RSTF_VISUALIZE_HEX,
        RSTF_VISUALIZE_INT,
        RSTF_VISUALIZE_UINT,
        RSTF_VISUALIZE_FLOAT,
        RSTF_VISUALIZE_FLOAT4,
        RSTF_VISUALIZE_UINT4,
        RSTF_VISUALIZE_INT4,
        RSTF_VISUALIZE_BOOL4,

        RSTF_VISUALIZE_GROUP_START,
        RSTF_VISUALIZE_GROUP_END,

        RSTF_VISUALIZE_RESOURCE_PIXEL_SHADER,
        RSTF_VISUALIZE_RESOURCE_VERTEX_SHADER,
        RSTF_VISUALIZE_RESOURCE_GEOMETRY_SHADER,
        RSTF_VISUALIZE_RESOURCE_HULL_SHADER,
        RSTF_VISUALIZE_RESOURCE_DOMAIN_SHADER,
        RSTF_VISUALIZE_RESOURCE_COMPUTE_SHADER,
        RSTF_VISUALIZE_RESOURCE_TEXTURE,
        RSTF_VISUALIZE_RESOURCE_BUFFER,
        RSTF_VISUALIZE_RESOURCE_VERTEXTYPE,
        RSTF_VISUALIZE_RESOURCE_RENDERTARGET,
        RSTF_VISUALIZE_RESOURCE_DEPTHTARGET,

        RSTF_VISUALIZE_ENUM_BEGIN,
    };

    namespace PixelTests
    {
        enum Type
        {
            // In order of appearance in pipeline
            TriangleCulled          = 0x00000001,
            ShaderClips             = 0x00000002,
            AlphaTest               = 0x00000004,
            ClipPlanes              = 0x00000008,
            Scissor                 = 0x00000010,
            DepthTest               = 0x00000020,
            Stencil                 = 0x00000040,
            LastPipeline            = Stencil,
            AllPipeline             = 0x0000007F,

            BlendFactor             = 0x00000080,
            ColorMatches            = 0x00000100,
            ColorWriteMask          = 0x00000200,
            RenderTargetsBound      = 0x00000400,
            RenderTargetsNotEnabled = 0x00000800,

            Rasterization       = 0x80000000,
        };
    }

    namespace PrimitiveType
    {
        enum Type
        {
            Unknown = 0,
            PointList,
            LineList,
            LineStrip,
            TriangleList,
            TriangleStrip,
        };
    };

    struct RenderStateType
    {
        unsigned int                mType;
        unsigned int                mFlags;
        unsigned int                mVisualizer; // RenderStateVisualizerType
        const char*                 mName;
    };

    struct EventType
    {
        unsigned int mType;
        unsigned int mFlags;
        const char*  mName;
    };

    // This structure is only a header. The actual data of the render state should follow this header in a buffer
    // of the total render state. This will be used by the tool to identify data and create relevant UI.
    struct RenderState
    {
        unsigned int mType;
        unsigned int mFlags;
        unsigned int mSize;
    };

    enum RenderStateFlags
    {
        RSF_EditedByTool = 0x00000001,
    };

    struct ToolDataItem // Actual data follows item in memory like a render state normally would.
    {
        ToolDataItem* nextItem;
        RenderState   stateData;
    };

    struct DrawCallData
    {
        Dissector::RenderState* mFirstRenderState;
        char*                   mDrawCallData; 
        ToolDataItem*           mToolRenderStates;     // Linked list of overridden render states
        int                     mSizeDrawCallData;
        int                     mSizeRenderStateData;
        int                     mEventType; // -1 means a draw call group, -2 means end of draw group
    };

    struct ShaderDebugHeader
    {
        int mNumFilenames;
        int mNumVariables;
        int mVariableBlockSize;
        int mNumSteps;
    };

    struct ShaderDebugVariable // followed by a string with displayname
    {
        int mType;
        int mColumns;
        int mRows;
        int mVariableBlockOffset;
    };

    struct ShaderDebugStep
    {
        int mFilename;
        int mLine;
        int mNumUpdates;
    };

    struct ShaderDebugVariableUpdate
    {
        unsigned int mDataBlockOffset;
        short mUpdateOffsets[4];
        unsigned int mDataInt[4];
    };

    struct TimingData
    {
        float mTimeInMS;
    };

    struct PixelLocation
    {
        PixelLocation(){}
        PixelLocation( unsigned int iX, unsigned int iY, unsigned int iZ ) : mX( iX ), mY( iY ), mZ( iZ ){}

        unsigned int mX, mY, mZ;
    };

    // Frames
    void FrameUpdate( void* iDevice );
    bool ToolWantsToCapture(); // returns true if the user triggers a capture from the tool.
    void TriggerCapture();
    void TriggerCaptureStop();

    void BeginFrame(void* iDevice);
    void EndFrame(void* iDevice);

    void StartSlave( void* iDevice ); // Blocks

    bool IsCapturing();

    // Draw call registering functions
    void StartDrawCallGroup( const char* iName );
    void EndDrawCallGroup();
    void RegisterEvent( void* iDevice, int iEventType, const void* iEventData, unsigned int iDataSize );

    // Messaging Functions
    void ReceiveMessage( char* iMessage, unsigned int iSize );

    // Functions for platforms
    Dissector::DrawCallData* GetCaptureData();
    int                      GetCaptureSize();

    // Platform functions
    void ClearTypeInformation();
    void RegisterEventType( unsigned int iType, unsigned int iFlags, const char* iName );
    void RegisterRenderStateTypes( RenderStateType* iTypes, unsigned int iNumTypes );
    void RegisterRenderStateEnum( unsigned int iVisualizerId, ... );
    void AddRenderStateEntries( void* iRenderStateData, unsigned int iDataSize ); // Called from the FillRenderStatesCallback
    void AddEventStats( const QueryData* iData, unsigned int iNumData ); // Called from GatherStatsForEventCallback
    
    // Call this from GetTextureImageCallback to supply the image.
    void ImageRetrievalCallback( void* iTexture, unsigned int iVisualizerType, void* iImage, unsigned int iSizeX,
                                 unsigned int iSizeY, unsigned int iPitch );

    // Call this from GetCurrentRTCallback to supply the image.
    void CurrentRTRetrievalCallback( void* iImage, unsigned int iSizeX, unsigned int iSizeY, unsigned int iPitch );

    // Call this from GetTextureImageCallback to supply the image.
    void ImageRetrievalCallback( void* iTexture, unsigned int iVisualizerType, void* iImage, unsigned int iSizeX,
                                 unsigned int iSizeY, unsigned int iPitch );
    
    // Call this from GetTextureThumbnailCallback to supply the image.
    void ThumbnailRetrievalCallback( void* iTexture, unsigned int iVisualizerType, void* iImage, unsigned int iSizeX,
                                 unsigned int iSizeY, unsigned int iPitch, int iEventNum );

    // Call from ShaderDebugCallback to supply debug information.
    void ShaderDebugDataCallback( char* iDataBlob, unsigned int iDataSize, int iEventId, unsigned int loc0, unsigned int loc1 );
    void ShaderDebugFailedCallback( int iEventId, unsigned int loc0, unsigned int loc1 );

    // Call from GetMeshCallback to supply the mesh.
    void MeshDebugDataCallback( unsigned int iEventId, Dissector::PrimitiveType::Type iType,
        unsigned int iNumPrims, char* iVertsBlob, unsigned int iVertsSize, char* iIndices, unsigned int iIndicesSize );

    // Platform Callbacks
    void SetMallocCallback( void* (*iMallocCallback)(size_t size) );
    void SetFreeCallback( void (*iFreeCallback)(void* ptr) );
    void SetSendMessageCallback( void (*iSendMessage)(const char* iMessage, unsigned int iSize) );
    void SetNetworkCallback( void (*iNetworkFrameUpdate)() );
    void SetSlaveEndingCallback( void (*iSlaveEnded)() );

    // Render Callbacks
    struct RenderCallbacks
    {
        virtual void FillRenderStates(void* iDevice, int iEventType, const void* iEventData,  unsigned int iDataSize) { iDevice; iEventType; iEventData; iDataSize; }
        virtual void ResimulateStart(void* iDevice) { iDevice; }
        virtual void ResimulateEvent(void* iDevice, const DrawCallData& iData, bool iRecordTiming, TimingData* oTiming ) { iDevice; iData; iRecordTiming; oTiming; }
        virtual void GatherStatsForEvent(void* iDevice, const DrawCallData& iData) { iDevice; iData; }
        virtual bool ResimulateEnd(void* iDevice) { iDevice; return false; }
        virtual void ShaderDebug(void* iDevice, int iEventId, const DrawCallData& iData, unsigned int iShaderType, char* iDebugData, unsigned int iDataSize) { iDevice; iEventId; iData; iShaderType; iDebugData; iDataSize; }
        virtual void GetTextureThumbnail( void* iDevice, void* iTexture, unsigned int iVisualizerType, int iEventNum ) { iDevice; iTexture; iVisualizerType; iEventNum; }
        virtual void GetTextureImage( void* iDevice, void* iTexture, unsigned int iRSType, unsigned int iEventNum ) { iDevice; iTexture; iRSType; iEventNum; }
        virtual void GetCurrentRT( void* iDevice ) { iDevice; }
        virtual bool TestPixelsWritten(void* iDevice, const Dissector::DrawCallData& iData,
                        PixelLocation* iPixels, unsigned int iNumPixels) { iDevice; iData; iPixels; iNumPixels; return false; }
        virtual void SlaveBegin( void* iDevice ) { iDevice; }
        virtual void SlaveEnd( void* iDevice ) { iDevice; }
        virtual void TestPixelFailure(void* iDevice, unsigned int iEventId, Dissector::PixelLocation iPixel,
                        unsigned int *oFails ) { iDevice; iEventId; iPixel; oFails; }
        virtual void GetMesh(void* iDevice, unsigned int iEventId, unsigned int iType ) { iDevice; iEventId; iType; }
        virtual void GetEventText( DrawCallData* iDC, size_t iMaxSize, char* oBuffer, size_t& oSize ) { iDC; iMaxSize; oBuffer; oSize; }
        virtual void SlaveFrameBegin() {}
        virtual void SlaveFrameEnd() {}

        virtual void BeginCaptureFrame( void* iDevice ) { iDevice; }
        virtual void EndCaptureFrame( void* iDevice ) { iDevice; }
    };

    void SetRenderCallbacks( RenderCallbacks* iCallbacks );

    // For other libs to use easily
    void* MallocCallback( size_t size );
    void FreeCallback( void* ptr );
};

#define SAFE_DEALLOC( iObject ) if( iObject ){ Dissector::FreeCallback( iObject ); iObject = 0; }

template< typename tType >
struct ScopedFree
{
    ScopedFree(): mPtr( 0 ){}
    ScopedFree(tType* iPtr): mPtr( iPtr ){}
    ~ScopedFree(){ SAFE_DEALLOC( mPtr ); }
    const ScopedFree& operator=( tType* iPtr ){ mPtr = iPtr; return *this; }
    tType* operator->(){ return mPtr; }
    operator tType*(){ return mPtr; }

    tType* mPtr;
};
#endif

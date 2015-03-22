#include "DissectorDX9.h"
#include "ShaderDebugDX9.h"
#include "DX9Tests.h"
#include "DX9Helpers.h"
#include "DX9Capture.h"

namespace DissectorDX9
{
    void DrawFullScreenQuad( IDirect3DDevice9* iD3DDevice )
    {
        iD3DDevice->SetVertexDeclaration( sDX9Data.mPosVert );
        iD3DDevice->SetVertexShader( sDX9Data.mPosTexVS );
        iD3DDevice->SetStreamSource( 0, sDX9Data.mFullscreenQuadVerts, 0, sizeof(float)*4 );
        iD3DDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2 );
    }

    void ResetDevice( IDirect3DDevice9* iD3DDevice )
    {
        iD3DDevice->SetRenderState( D3DRS_CLIPPING, TRUE );
        iD3DDevice->SetRenderState( D3DRS_STENCILENABLE, FALSE );
        iD3DDevice->SetRenderState( D3DRS_ZENABLE, D3DZB_FALSE );
        iD3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
        iD3DDevice->SetRenderState( D3DRS_ZWRITEENABLE, FALSE );
        iD3DDevice->SetRenderState( D3DRS_ALPHATESTENABLE, FALSE );
        iD3DDevice->SetRenderState( D3DRS_LASTPIXEL, FALSE );
        iD3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
        iD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
        iD3DDevice->SetRenderState( D3DRS_CLIPPLANEENABLE, FALSE );
        iD3DDevice->SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_RED |
            D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE );
        iD3DDevice->SetRenderState( D3DRS_COLORWRITEENABLE1, D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_RED |
            D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE );
        iD3DDevice->SetRenderState( D3DRS_COLORWRITEENABLE2, D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_RED |
            D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE );
        iD3DDevice->SetRenderState( D3DRS_COLORWRITEENABLE3, D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_RED |
            D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE );
        iD3DDevice->SetRenderState( D3DRS_SCISSORTESTENABLE, FALSE );
        iD3DDevice->SetRenderState( D3DRS_SEPARATEALPHABLENDENABLE, FALSE );
    }

    void ResetSampler( IDirect3DDevice9* iD3DDevice, unsigned int iSampler )
    {
        iD3DDevice->SetSamplerState( iSampler, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
        iD3DDevice->SetSamplerState( iSampler, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
        iD3DDevice->SetSamplerState( iSampler, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP );
        iD3DDevice->SetSamplerState( iSampler, D3DSAMP_BORDERCOLOR, 0 );
        iD3DDevice->SetSamplerState( iSampler, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
        iD3DDevice->SetSamplerState( iSampler, D3DSAMP_MINFILTER, D3DTEXF_POINT );
        iD3DDevice->SetSamplerState( iSampler, D3DSAMP_MIPFILTER, D3DTEXF_POINT );
        iD3DDevice->SetSamplerState( iSampler, D3DSAMP_MIPMAPLODBIAS, 0 );
        iD3DDevice->SetSamplerState( iSampler, D3DSAMP_MAXMIPLEVEL, 0 );
        iD3DDevice->SetSamplerState( iSampler, D3DSAMP_MAXANISOTROPY, 1 );
        iD3DDevice->SetSamplerState( iSampler, D3DSAMP_SRGBTEXTURE, 0 );
        iD3DDevice->SetSamplerState( iSampler, D3DSAMP_ELEMENTINDEX, 0 );
        iD3DDevice->SetSamplerState( iSampler, D3DSAMP_DMAPOFFSET, 0 );
    }

    void ExecuteEvent( IDirect3DDevice9* iD3DDevice, const Dissector::DrawCallData& iData )
    {
        switch( iData.mEventType )
        {
        case(ET_DRAWINDEXED):
            {
                assert( iData.mSizeDrawCallData == sizeof(DrawIndexedData) );
                DrawIndexedData* dc = (DrawIndexedData*)iData.mDrawCallData;
                iD3DDevice->DrawIndexedPrimitive( dc->mPrimitiveType, dc->mBaseVertexIndex, dc->mMinIndex,
                    dc->mNumVertices, dc->mStartElement, dc->mPrimitiveCount );
            }break;
        case(ET_DRAW):
            {
                assert( iData.mSizeDrawCallData == sizeof(DrawData) );
                DrawData* dc = (DrawData*)iData.mDrawCallData;
                iD3DDevice->DrawPrimitive( dc->mPrimitiveType, dc->mStartElement, dc->mPrimitiveCount );
            }break;
        case(ET_DRAWINDEXEDUP):
            {
                DrawIndexedUpData* diu = (DrawIndexedUpData*)iData.mDrawCallData;
                char* indexData = ((char*)diu) + sizeof(DrawIndexedUpData);
                char* vertData = indexData + diu->mNumIndices * (diu->mIndexDataFormat == D3DFMT_INDEX16 ? 2 : 4);
                iD3DDevice->DrawIndexedPrimitiveUP( diu->mPrimitiveType, diu->mStartElement, diu->mNumVertices,
                    diu->mPrimitiveCount, indexData, diu->mIndexDataFormat, vertData, diu->mStride );
            }break;
        case(ET_DRAWUP):
            {
                DrawUpData* dc = (DrawUpData*)iData.mDrawCallData;
                void* vertexData = iData.mDrawCallData + sizeof(DrawUpData);
                iD3DDevice->DrawPrimitiveUP( dc->mPrimitiveType, dc->mPrimitiveCount, vertexData, dc->mStride );
            }break;
        case(ET_CLEAR):
            {
                assert( iData.mSizeDrawCallData == sizeof(ClearData) );
                ClearData* dc = (ClearData*)iData.mDrawCallData;
                iD3DDevice->Clear( 0, 0, dc->mFlags, dc->mColor, dc->mZ, dc->mStencil );
            }break;
        case(ET_VERTEXLOCK):
            {
                BufferLockData* ld = (BufferLockData*)iData.mDrawCallData;
                IDirect3DVertexBuffer9* buf = (IDirect3DVertexBuffer9*)ld->mBuffer;
                void* src = ld+1;
                void* dst;
                if( S_OK == buf->Lock( ld->mOffsetToLock, ld->mSizeToLock, &dst, ld->mFlags ) )
                {
                    memcpy( dst, src, ld->mDataSize );
                    buf->Unlock();
                }
            }break;
        case(ET_INDEXLOCK):
            {
                BufferLockData* ld = (BufferLockData*)iData.mDrawCallData;
                IDirect3DIndexBuffer9* buf = (IDirect3DIndexBuffer9*)ld->mBuffer;
                void* src = ld+1;
                void* dst;
                if( S_OK == buf->Lock( ld->mOffsetToLock, ld->mSizeToLock, &dst, ld->mFlags ) )
                {
                    memcpy( dst, src, ld->mDataSize );
                    buf->Unlock();
                }
            }break;
        case(ET_ENDFRAME):
            {
            }break;
        default:
            break;
        }
    }

    void BeginFrame( void* iDevice )
    {
        IDirect3DDevice9* D3DDevice = (IDirect3DDevice9*)iDevice;
        D3DDevice->BeginScene();

        // Copy all our base RTs
        for( RenderTargetData* rt = sDX9Data.mRTCopies; rt != NULL; rt = rt->mNext )
        {
            if( rt->mSurface && rt->mSurfaceCopy )
            {
                D3DXLoadSurfaceFromSurface( rt->mSurface, NULL, NULL, rt->mSurfaceCopy, NULL, NULL, D3DX_FILTER_NONE, 0 );
                //D3DDevice->StretchRect( rt->mSurfaceCopy, NULL, rt->mSurface, NULL, D3DTEXF_POINT );
            }
        }
    }

    bool EndFrame( void* iDevice, bool iShowRT0 )
    {
        IDirect3DDevice9* D3DDevice = (IDirect3DDevice9*)iDevice;
        if( iShowRT0 )
        {
            ScopedRelease<IDirect3DSurface9> backBuffer, currentRT0;
            D3DDevice->GetBackBuffer( 0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer.mPtr );
            D3DDevice->GetRenderTarget( 0, &currentRT0.mPtr );

            if( currentRT0.mPtr != backBuffer.mPtr )
            {
                // Copy one to the other
                D3DDevice->StretchRect( currentRT0.mPtr, NULL, backBuffer.mPtr, NULL, D3DTEXF_POINT );
            }
        }

        D3DDevice->EndScene();
        D3DDevice->Present( NULL, NULL, NULL, NULL ); // Present just to be safe.
        if( sDX9Data.mPumpFunction )
        {
            if( !sDX9Data.mPumpFunction() )
            {
                Dissector::TriggerCaptureStop();
                return true;
            }
        }

        return false;
    }

    void ResimulateEvent( void* iDevice, const Dissector::DrawCallData& iData, bool iRecordTiming, 
                       Dissector::TimingData* oTiming )
    {
        IDirect3DDevice9* D3DDevice = (IDirect3DDevice9*)iDevice;

        SetRenderStates( D3DDevice, &iData );

        LARGE_INTEGER timingStart, timingStop;
        if( iRecordTiming )
        {
            sDX9Data.mTimingQuery->Issue(D3DISSUE_END);
            while(S_FALSE == sDX9Data.mTimingQuery->GetData( NULL, 0, D3DGETDATA_FLUSH ));
            QueryPerformanceCounter(&timingStart);
        }

        ExecuteEvent( D3DDevice, iData );

        if( iRecordTiming )
        {
            sDX9Data.mTimingQuery->Issue(D3DISSUE_END);
            while(S_FALSE == sDX9Data.mTimingQuery->GetData( NULL, 0, D3DGETDATA_FLUSH ));
            QueryPerformanceCounter(&timingStop);
            oTiming->mTimeInMS = float(timingStop.QuadPart - timingStart.QuadPart) * sDX9Data.mTimingFrequency;
        }
    }

    Dissector::DrawCallData* ExecuteToEvent( void* iDevice, int iEventId )
    {
        if( (int)iEventId >= Dissector::GetCaptureSize() )
            return NULL;

        IDirect3DDevice9* D3DDevice = (IDirect3DDevice9*)iDevice;
        BeginFrame( iDevice );

        Dissector::DrawCallData* dcData = Dissector::GetCaptureData();

        Dissector::DrawCallData* iterEnd = &dcData[ iEventId ];
        for( Dissector::DrawCallData* iter = dcData; iter != iterEnd; ++iter )
        {
            if( iter->mEventType >= 0 )
            {
                ResimulateEvent( iDevice, *iter, false, NULL );
            }
        }

        return iterEnd;
    }

    void* GetPixelShaderByteCode( IDirect3DDevice9* D3DDevice, UINT& oSize )
    {
        ScopedRelease<IDirect3DPixelShader9> pixShader;
        if( D3DDevice->GetPixelShader( &pixShader.mPtr ) != D3D_OK || !pixShader.mPtr )
        {
            return NULL;
        }

        UINT shaderSize = 0;
        pixShader->GetFunction( NULL, &shaderSize );

        void* shaderData = Dissector::MallocCallback( shaderSize );
        if( !shaderData )
        {
            return NULL;
        }
        pixShader->GetFunction( shaderData, &shaderSize );

        oSize = shaderSize;
        return shaderData;
    }

    void* GetVertexShaderByteCode( IDirect3DDevice9* D3DDevice, UINT& oSize )
    {
        ScopedRelease<IDirect3DVertexShader9> vertShader;
        if( D3DDevice->GetVertexShader( &vertShader.mPtr ) != D3D_OK || !vertShader.mPtr )
        {
            return NULL;
        }

        UINT shaderSize = 0;
        vertShader->GetFunction( NULL, &shaderSize );

        void* shaderData = Dissector::MallocCallback( shaderSize );
        if( !shaderData )
        {
            return NULL;
        }
        vertShader->GetFunction( shaderData, &shaderSize );

        oSize = shaderSize;
        return shaderData;
    }

    bool  GetPixelShaderInstructionOutput( void* iDevice, const Dissector::PixelLocation& iPixel, 
                                           Dissector::DrawCallData* iDrawCall, unsigned int iInstructionNum,
                                           bool iDontResetDevice, unsigned int iRemoveTypeMask, float* oValue )
    {
        IDirect3DDevice9* D3DDevice = (IDirect3DDevice9*)iDevice;

        // Find how many render targets we have
        ShaderDebugDX9::ShaderInfo info;
        UINT shaderDataSize = 0, shaderDataPatchedSize = 0;
        ScopedFree<void> shaderData = GetPixelShaderByteCode( D3DDevice, shaderDataSize );
        ScopedFree<void> shaderDataPatched = Dissector::MallocCallback( shaderDataSize + 
                                                                        ShaderDebugDX9::ShaderPatchSize );

        ScopedRelease<IDirect3DPixelShader9> debugPixelShader;
        info = ShaderDebugDX9::GetShaderInfo( shaderData, shaderDataSize, D3DDevice );

        // Setup pixel output render targets
        ScopedRelease<IDirect3DSurface9> curTarget, newTarget, instrTarget;
        HRESULT res;
        if( (res = D3DDevice->GetRenderTarget( 0, &curTarget.mPtr ) ) != D3D_OK )
        {
            return false;
        }

        D3DSURFACE_DESC curDesc;
        curTarget->GetDesc( &curDesc );

        if( ( res = D3DDevice->CreateRenderTarget( curDesc.Width, curDesc.Height, D3DFMT_A32B32G32R32F,
                                        D3DMULTISAMPLE_NONE, 0, true, &newTarget.mPtr, NULL ) ) != D3D_OK )
        {
            return false;
        }

        if( ( res = D3DDevice->CreateRenderTarget( curDesc.Width, curDesc.Height, D3DFMT_A32B32G32R32F,
                                        D3DMULTISAMPLE_NONE, 0, true, &instrTarget.mPtr, NULL ) ) != D3D_OK )
        {
            return false;
        }

        // Debug area to lock
        RECT area = { iPixel.mX, iPixel.mY, iPixel.mX + 1, iPixel.mY + 1 };

        unsigned char outputMask = 0;
        shaderDataPatchedSize = shaderDataSize + ShaderDebugDX9::ShaderPatchSize;
        if( !ShaderDebugDX9::PatchShaderForDebug( iInstructionNum, &info, iRemoveTypeMask, shaderData, shaderDataSize,
            shaderDataPatched, shaderDataPatchedSize, outputMask ) )
        {
            return false;
        }

        if( (res = D3DDevice->CreatePixelShader( (const DWORD*)shaderDataPatched.mPtr, &debugPixelShader.mPtr )) !=
            D3D_OK || !debugPixelShader )
        {
            return false;
        }

        // Setup device states
        if( iDontResetDevice )
        {
            // Outside code has setup the test to see if write out the value with their current render states.
            // Still, reset color write to make sure full output values will be written if it makes it that far.
            D3DDevice->SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_RED |
                D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE );
            D3DDevice->SetRenderState( D3DRS_COLORWRITEENABLE1, D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_RED |
                D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE );

            D3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
            D3DDevice->SetRenderState( D3DRS_SEPARATEALPHABLENDENABLE, FALSE );
            D3DDevice->SetRenderState( D3DRS_ZWRITEENABLE, FALSE );
        }
        else
        {
            ResetDevice( D3DDevice );
        }

        D3DDevice->SetRenderTarget( 0, instrTarget );
        D3DDevice->SetRenderTarget( 1, newTarget );
        D3DDevice->SetRenderTarget( 2, 0 );
        D3DDevice->SetRenderTarget( 3, 0 );

        D3DDevice->Clear( 0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB( 0,0,0,0 ), 0.f, 0 );
        D3DDevice->SetPixelShader( debugPixelShader );
        ExecuteEvent( D3DDevice, *iDrawCall );

        D3DLOCKED_RECT lrect;
        res = newTarget->LockRect( &lrect, &area, NULL );

        oValue[0] = ((float*)lrect.pBits)[0];
        oValue[1] = ((float*)lrect.pBits)[1];
        oValue[2] = ((float*)lrect.pBits)[2];
        oValue[3] = ((float*)lrect.pBits)[3];

        newTarget->UnlockRect();

        bool rvalue = false;
        instrTarget->LockRect( &lrect, &area, NULL );
        if( ((float*)lrect.pBits)[0] == float(iInstructionNum) &&
            ((float*)lrect.pBits)[1] == 2.f &&
            ((float*)lrect.pBits)[2] == 1.f &&
            ((float*)lrect.pBits)[3] == 0.f
            )
        {
            rvalue = true;
        }

        instrTarget->UnlockRect();

        D3DDevice->SetRenderTarget( 0, NULL );
        D3DDevice->SetRenderTarget( 1, NULL ); // Weird things happen if we don't rest this. Dunno why. That's hobby coding for you.

        return rvalue;
    }

    static int D3DDECLTYPESize[] = {
         4, //D3DDECLTYPE_FLOAT1   
         8, //D3DDECLTYPE_FLOAT2   
        12, //D3DDECLTYPE_FLOAT3   
        16, //D3DDECLTYPE_FLOAT4   
         4, //D3DDECLTYPE_D3DCOLOR 
         4, //D3DDECLTYPE_UBYTE4   
         4, //D3DDECLTYPE_SHORT2   
         8, //D3DDECLTYPE_SHORT4   
         4, //D3DDECLTYPE_UBYTE4N  
         4, //D3DDECLTYPE_SHORT2N  
         8, //D3DDECLTYPE_SHORT4N  
         4, //D3DDECLTYPE_USHORT2N 
         8, //D3DDECLTYPE_USHORT4N 
         4, //D3DDECLTYPE_UDEC3    
         4, //D3DDECLTYPE_DEC3N    
         4, //D3DDECLTYPE_FLOAT16_2
         8, //D3DDECLTYPE_FLOAT16_4
         0, //D3DDECLTYPE_UNUSED   
    };

    bool ConvertD3DDECLTYPEToFloat4( D3DDECLTYPE iType, char* iInput, float* oOutput )
    {
        switch( iType )
        {
        case(D3DDECLTYPE_FLOAT1):
        case(D3DDECLTYPE_FLOAT2):
        case(D3DDECLTYPE_FLOAT3):
        case(D3DDECLTYPE_FLOAT4):
        {
            int num = 1 + int(iType) - int(D3DDECLTYPE_FLOAT1);
            for( int ii = 0; ii < num; ++ii )
            {
                oOutput[ii] = ((float*)iInput)[ii];
            }
            for( int ii = num; ii < 4; ++ii )
            {
                oOutput[ii] = (ii == 3) ? 1.f : 0.f;
            }
        }break;

        case(D3DDECLTYPE_D3DCOLOR):
        case(D3DDECLTYPE_UBYTE4N):
        {
            for( int ii = 0; ii < 4; ++ii )
            {
                oOutput[ii] = float( ((BYTE*)iInput)[ii] * (1.f/255.f) );
            }
        }break;

        case(D3DDECLTYPE_UBYTE4):
        {
            for( int ii = 0; ii < 4; ++ii )
            {
                oOutput[ii] = float( ((BYTE*)iInput)[ii] );
            }
        }break;

        case(D3DDECLTYPE_SHORT2):
        case(D3DDECLTYPE_SHORT4):
        {
            int num = iType == D3DDECLTYPE_SHORT2 ? 2 : 4;
            for( int ii = 0; ii < num; ++ii )
            {
                oOutput[ii] = ((short*)iInput)[ii];
            }
            for( int ii = num; ii < 4; ++ii )
            {
                oOutput[ii] = (ii == 3) ? 1.f : 0.f;
            }
        }break;

        case(D3DDECLTYPE_SHORT2N):
        case(D3DDECLTYPE_SHORT4N):
        {
            int num = iType == D3DDECLTYPE_SHORT2N ? 2 : 4;
            for( int ii = 0; ii < num; ++ii )
            {
                oOutput[ii] = float( ((short*)iInput)[ii] * 1.f/255.f );
            }
            for( int ii = num; ii < 4; ++ii )
            {
                oOutput[ii] = (ii == 3) ? 1.f : 0.f;
            }
        }break;

        case(D3DDECLTYPE_USHORT2N):
        case(D3DDECLTYPE_USHORT4N):
        {
            int num = iType == D3DDECLTYPE_USHORT2N ? 2 : 4;
            for( int ii = 0; ii < num; ++ii )
            {
                oOutput[ii] = float( ((unsigned short*)iInput)[ii] * 1.f/255.f );
            }
            for( int ii = num; ii < 4; ++ii )
            {
                oOutput[ii] = (ii == 3) ? 1.f : 0.f;
            }
        }break;

        // Currently unsupported
        case(D3DDECLTYPE_UDEC3):
        case(D3DDECLTYPE_DEC3N):
        case(D3DDECLTYPE_FLOAT16_2):
        case(D3DDECLTYPE_FLOAT16_4):
        case(D3DDECLTYPE_UNUSED):
        default: return false;
        }

        return true;
    }

    UINT* GetIndices( void* iDevice, UINT iStartElement, UINT iNumElements, UINT& oBufferSize )
    {
        IDirect3DDevice9* D3DDevice = (IDirect3DDevice9*)iDevice;
        ScopedRelease<IDirect3DIndexBuffer9> indices;
        UINT* rvalue = NULL;

        if( S_OK == D3DDevice->GetIndices( &indices.mPtr ) )
        {
            D3DINDEXBUFFER_DESC desc;
            indices->GetDesc( &desc );

            UINT* buf;
            if( S_OK == indices->Lock( 0, 0, (void**)&buf, D3DLOCK_READONLY ) )
            {
                if( desc.Format == D3DFMT_INDEX16 )
                {
                    size_t baseSize = iNumElements * 2;
                    if( baseSize > desc.Size )
                        baseSize = desc.Size;

                    size_t size = iNumElements * sizeof(UINT);
                    size = (size > (2*desc.Size)) ? (2*desc.Size) : size;

                    rvalue = (UINT*)Dissector::MallocCallback( size );
                    oBufferSize = (unsigned int)size;
                    unsigned short* iter = ((unsigned short*)buf) + iStartElement;
                    unsigned short* end = (unsigned short*)(((unsigned char*)iter) + baseSize);
                    UINT* outIter = rvalue;
                    for( ; iter < end; ++iter, ++outIter )
                    {
                        *outIter = (UINT)(*iter);
                    }
                }
                else
                {
                    size_t size = iNumElements * sizeof(UINT);
                    size = (size > desc.Size) ? desc.Size : size;

                    oBufferSize = (unsigned int)size;
                    rvalue = (UINT*)Dissector::MallocCallback( size );
                    if( rvalue )
                    {
                        memcpy( rvalue, buf + iStartElement, size );
                    }
                }
                indices->Unlock();
            }
        }

        return rvalue;
    }

    float* GetVertexShaderInputPositions( void* iDevice, UINT iStartElement, UINT iNumElements, UINT& oBufferSize )
    {
        IDirect3DDevice9* D3DDevice = (IDirect3DDevice9*)iDevice;
        ScopedRelease<IDirect3DVertexDeclaration9> vertDecl;
        ScopedRelease<IDirect3DVertexBuffer9> vertBuffer;
        D3DDevice->GetVertexDeclaration( &vertDecl.mPtr );

        D3DVERTEXELEMENT9 decl[MAXD3DDECLLENGTH];
        UINT numElements;

        vertDecl->GetDeclaration( decl, &numElements );
        int offset = -1;
        int stream = -1;
        int sizeInBytes = -1;

        D3DDECLTYPE type;
        for( unsigned int ii = 0; ii < numElements; ++ii )
        {
            if( decl[ii].Type < D3DDECLTYPE_UNUSED && decl[ii].Usage == D3DDECLUSAGE_POSITION && decl[ii].UsageIndex == 0 )
            {
                offset = decl[ii].Offset;
                stream = decl[ii].Stream;
                type = D3DDECLTYPE(decl[ii].Type);
                sizeInBytes = D3DDECLTYPESize[decl[ii].Type];
                break;
            }
        }

        if( offset < 0 || stream < 0 || sizeInBytes < 0 ) return NULL;

        HRESULT res;

        UINT offsetInBytes, streamStride;
        res = D3DDevice->GetStreamSource( stream, &vertBuffer.mPtr, &offsetInBytes, &streamStride );
        if( !vertBuffer.mPtr || !streamStride ) return NULL;

        D3DVERTEXBUFFER_DESC desc;
        if( D3D_OK != vertBuffer->GetDesc( &desc ) ) return NULL;

        
        char* vertData;
        if( D3D_OK != vertBuffer->Lock( 0, 0, (void**)&vertData, D3DLOCK_READONLY ) ) return NULL;
        char* vertIter = vertData + offsetInBytes + offset;
        vertIter += (iStartElement * streamStride);

        char* vertEnd = vertData + desc.Size;

        size_t numVerts = (vertEnd - vertIter) / streamStride;
        numVerts = (numVerts < iNumElements) ? numVerts : iNumElements;

        vertEnd = vertIter + numVerts * streamStride;

        //TODO: Convert the position for each vertex into a float4 and return that buffer.
        oBufferSize = (unsigned int)(numVerts * 4 * sizeof(float));
        float* rvalue = (float*)Dissector::MallocCallback( oBufferSize );
        if( rvalue )
        {
            float* outIter = rvalue;

            for( ;vertIter < vertEnd; vertIter += streamStride, outIter += 4 )
            {
                ConvertD3DDECLTYPEToFloat4( type, vertIter, outIter );
            }
        }

        vertBuffer->Unlock();

        return rvalue;
    }

    void* GetVertexShaderOutputPositions( void* iDevice )
    {
        assert( false );
        return NULL;
    }

    unsigned int GetFormatSize( D3DFORMAT iFormat )
    {
        switch( iFormat )
        {
        case(D3DFMT_UNKNOWN):       
        case(D3DFMT_DXT1):          
        case(D3DFMT_DXT2):          
        case(D3DFMT_DXT3):          
        case(D3DFMT_DXT4):          
        case(D3DFMT_DXT5):          
        case(D3DFMT_VERTEXDATA):    
        case(D3DFMT_MULTI2_ARGB8):
        default:
            return 0;

        case(D3DFMT_R3G3B2):        
        case(D3DFMT_A8):            
        case(D3DFMT_P8):            
        case(D3DFMT_L8):            
        case(D3DFMT_A4L4):          
            return 1;

        case(D3DFMT_R5G6B5):        
        case(D3DFMT_X1R5G5B5):      
        case(D3DFMT_A1R5G5B5):      
        case(D3DFMT_A4R4G4B4):      
        case(D3DFMT_A8R3G3B2):      
        case(D3DFMT_X4R4G4B4):      
        case(D3DFMT_A8P8):          
        case(D3DFMT_A8L8):          
        case(D3DFMT_V8U8):          
        case(D3DFMT_L6V5U5):        
        case(D3DFMT_UYVY):          
        case(D3DFMT_R8G8_B8G8):     
        case(D3DFMT_YUY2):          
        case(D3DFMT_G8R8_G8B8):     
        case(D3DFMT_D16_LOCKABLE):  
        case(D3DFMT_D15S1):         
        case(D3DFMT_D16):           
        case(D3DFMT_L16):           
        case(D3DFMT_INDEX16):       
        case(D3DFMT_R16F):          
        case(D3DFMT_CxV8U8):        
            return 2;

        case(D3DFMT_R8G8B8):
            return 3;

        case(D3DFMT_A8R8G8B8):     
        case(D3DFMT_X8R8G8B8):     
        case(D3DFMT_A2B10G10R10):  
        case(D3DFMT_A8B8G8R8):     
        case(D3DFMT_X8B8G8R8):     
        case(D3DFMT_G16R16):       
        case(D3DFMT_A2R10G10B10):  
        case(D3DFMT_X8L8V8U8):     
        case(D3DFMT_Q8W8V8U8):     
        case(D3DFMT_V16U16):       
        case(D3DFMT_A2W10V10U10):  
        case(D3DFMT_D32):          
        case(D3DFMT_D24S8):        
        case(D3DFMT_D24X8):        
        case(D3DFMT_D24X4S4):      
        case(D3DFMT_D32F_LOCKABLE):
        case(D3DFMT_D24FS8):       
        case(D3DFMT_INDEX32):      
        case(D3DFMT_G16R16F):      
        case(D3DFMT_R32F):         
            return 4;

        case(D3DFMT_A16B16G16R16): 
        case(D3DFMT_Q16W16V16U16): 
        case(D3DFMT_A16B16G16R16F):
        case(D3DFMT_G32R32F):       
            return 8;

        case(D3DFMT_A32B32G32R32F): 
            return 16;
        }
    }

    unsigned int GetFormatWriteMask( D3DFORMAT iFormat )
    {
        switch( iFormat )
        {

        case(D3DFMT_UNKNOWN):       
        case(D3DFMT_VERTEXDATA):    
        case(D3DFMT_MULTI2_ARGB8):
        case(D3DFMT_INDEX16):       
        case(D3DFMT_INDEX32):      
        default:
            return 0;

        case(D3DFMT_A8):            
            return 0x8;

        case(D3DFMT_D32):          
        case(D3DFMT_D24S8):        
        case(D3DFMT_D24X8):        
        case(D3DFMT_D24X4S4):      
        case(D3DFMT_D32F_LOCKABLE):
        case(D3DFMT_D24FS8):       
        case(D3DFMT_P8):            
        case(D3DFMT_L8):            
        case(D3DFMT_D16_LOCKABLE):  
        case(D3DFMT_D15S1):         
        case(D3DFMT_D16):           
        case(D3DFMT_L16):           
        case(D3DFMT_R16F):          
        case(D3DFMT_R32F):         
            return 0x1;

        case(D3DFMT_A4L4):          
        case(D3DFMT_A8P8):          
        case(D3DFMT_A8L8):
            return 0x9;

        case(D3DFMT_V8U8):          
        case(D3DFMT_G16R16):       
        case(D3DFMT_V16U16):       
        case(D3DFMT_G16R16F):      
        case(D3DFMT_G32R32F):       
            return 0x3;

        case(D3DFMT_R3G3B2):        
        case(D3DFMT_R5G6B5):        
        case(D3DFMT_X1R5G5B5):      
        case(D3DFMT_X8R8G8B8):     
        case(D3DFMT_X8B8G8R8):     
        case(D3DFMT_L6V5U5):        
        case(D3DFMT_G8R8_G8B8):     
        case(D3DFMT_UYVY):          
        case(D3DFMT_R8G8_B8G8):     
        case(D3DFMT_YUY2):          
        case(D3DFMT_CxV8U8):        
        case(D3DFMT_X8L8V8U8):     
        case(D3DFMT_R8G8B8):
        case(D3DFMT_X4R4G4B4):      
            return 0x7;

        case(D3DFMT_DXT1):          
        case(D3DFMT_DXT2):          
        case(D3DFMT_DXT3):          
        case(D3DFMT_DXT4):          
        case(D3DFMT_DXT5):          
        case(D3DFMT_A1R5G5B5):      
        case(D3DFMT_A4R4G4B4):      
        case(D3DFMT_A8R3G3B2):      
        case(D3DFMT_A8R8G8B8):     
        case(D3DFMT_A2B10G10R10):  
        case(D3DFMT_A8B8G8R8):     
        case(D3DFMT_A2R10G10B10):  
        case(D3DFMT_Q8W8V8U8):     
        case(D3DFMT_A2W10V10U10):  
        case(D3DFMT_A16B16G16R16): 
        case(D3DFMT_Q16W16V16U16): 
        case(D3DFMT_A16B16G16R16F):
        case(D3DFMT_A32B32G32R32F): 
            return 0xF;
        }
    }

    void ApplyColorWriteMask( DWORD iMask, float* ioColor )
    {
        if( 0 == (iMask & 0x1) )
            ioColor[2] = 0.f;

        if( 0 == (iMask & 0x2) )
            ioColor[1] = 0.f;

        if( 0 == (iMask & 0x4) )
            ioColor[0] = 0.f;

        if( 0 == (iMask & 0x8) )
            ioColor[3] = 0.f;
    }


    bool GetRenderTargetPixelColor( void* iDevice, unsigned int iRenderTargetNumber,
                                    const Dissector::PixelLocation& iPixel, float* oColor )
    {
        IDirect3DDevice9* D3DDevice = (IDirect3DDevice9*)iDevice;

        ScopedRelease<IDirect3DSurface9> rt, newRt;
        D3DDevice->GetRenderTarget( iRenderTargetNumber, &rt.mPtr );
        if( !rt )
            return false;

        D3DSURFACE_DESC desc;

        rt->GetDesc( &desc );
        D3DDevice->CreateRenderTarget( 1, 1, D3DFMT_A32B32G32R32F, D3DMULTISAMPLE_NONE, 0, true, &newRt.mPtr, NULL );
        if( !newRt )
            return false;

        ScopedRelease<IDirect3DTexture9> rtTex;
        if( S_OK != D3DDevice->CreateTexture( desc.Width, desc.Height, 0, D3DUSAGE_RENDERTARGET, desc.Format,
                D3DPOOL_DEFAULT, &rtTex.mPtr, NULL ) )
                return false;

        HRESULT res;
        {
            ScopedRelease<IDirect3DSurface9> surface;
            rtTex->GetSurfaceLevel( 0, &surface.mPtr );
            res = D3DDevice->StretchRect( rt, NULL, surface, NULL, D3DTEXF_NONE );
        }

        ResetDevice( D3DDevice );
        float vertBufferUp[4] = { 0.f, 0.f, (iPixel.mX + .5f) / float(desc.Width), (iPixel.mY + .5f) / float(desc.Height) };

        D3DDevice->SetVertexShader( sDX9Data.mPosTexVS );
        D3DDevice->SetPixelShader( sDX9Data.mTexPS );

        float uvOffset[4];
        uvOffset[0] = uvOffset[1] = uvOffset[2] = uvOffset[3] = 0.f;

        D3DDevice->SetPixelShaderConstantF( 0, uvOffset, 1 );

        D3DDevice->SetTexture( 0, rtTex.mPtr );
        D3DDevice->SetRenderTarget( 0, newRt );
        D3DDevice->DrawPrimitiveUP( D3DPT_POINTLIST, 1, vertBufferUp, sizeof(vertBufferUp) );

        D3DLOCKED_RECT lrect;
        RECT area = { 0, 0, 1, 1 };
        newRt->LockRect( &lrect, &area, 0 );
        if( !lrect.pBits )
        {
            newRt->UnlockRect();
            return false;
        }

        oColor[0] = ((float*)lrect.pBits)[0];
        oColor[1] = ((float*)lrect.pBits)[1];
        oColor[2] = ((float*)lrect.pBits)[2];
        oColor[3] = ((float*)lrect.pBits)[3];

        newRt->UnlockRect();

        D3DDevice->SetTexture( 0, NULL );
        D3DDevice->SetRenderTarget( 0, rt );

        return true;
    }

    bool GetBlendFactor( DWORD iBlend, DWORD iBlendFactor, const float* iDst, const float* iSrc, float* oFactor )
    {
        switch( iBlend )
        {
        case( D3DBLEND_ZERO ):          
            oFactor[0] = 0.f; oFactor[1] = 0.f; oFactor[2] = 0.f; oFactor[3] = 0.f; break;
        case( D3DBLEND_ONE ):           
            oFactor[0] = 1.f; oFactor[1] = 1.f; oFactor[2] = 1.f; oFactor[3] = 1.f; break;
        case( D3DBLEND_SRCCOLOR ):
            oFactor[0] = iSrc[0]; oFactor[1] = iSrc[1]; oFactor[2] = iSrc[2]; oFactor[3] = iSrc[3]; break;
        case( D3DBLEND_INVSRCCOLOR ):
            oFactor[0] = 1.f - iSrc[0]; oFactor[1] = 1.f - iSrc[1];
            oFactor[2] = 1.f - iSrc[2]; oFactor[3] = 1.f - iSrc[3]; break;
        case( D3DBLEND_SRCALPHA ):
            oFactor[0] = iSrc[3]; oFactor[1] = iSrc[3]; oFactor[2] = iSrc[3]; oFactor[3] = iSrc[3]; break;
        case( D3DBLEND_INVSRCALPHA ):
            oFactor[0] = 1.f - iSrc[3]; oFactor[1] = 1.f - iSrc[3];
            oFactor[2] = 1.f - iSrc[3]; oFactor[3] = 1.f - iSrc[3]; break;
        case( D3DBLEND_DESTALPHA ):
            oFactor[0] = iDst[3]; oFactor[1] = iDst[3]; oFactor[2] = iDst[3]; oFactor[3] = iDst[3]; break;
        case( D3DBLEND_INVDESTALPHA ):
            oFactor[0] = 1.f - iDst[3]; oFactor[1] = 1.f - iDst[3];
            oFactor[2] = 1.f - iDst[3]; oFactor[3] = 1.f - iDst[3]; break;
        case( D3DBLEND_DESTCOLOR ):
            oFactor[0] = iDst[0]; oFactor[1] = iDst[1]; oFactor[2] = iDst[2]; oFactor[3] = iDst[3]; break;
        case( D3DBLEND_INVDESTCOLOR ):
            oFactor[0] = 1.f - iDst[0]; oFactor[1] = 1.f - iDst[1];
            oFactor[2] = 1.f - iDst[2]; oFactor[3] = 1.f - iDst[3]; break;
        case( D3DBLEND_SRCALPHASAT ):
        {
            float fac = min( iSrc[3], 1.f - iDst[3] );
            oFactor[0] = fac; oFactor[1] = fac; oFactor[2] = fac; oFactor[3] = 1.f;
        } break;
        case( D3DBLEND_BLENDFACTOR ):
        {
            float a = float( (iBlendFactor>>24)&0xFF ) * 1.f/255.f;
            float r = float( (iBlendFactor>>16)&0xFF ) * 1.f/255.f;
            float g = float( (iBlendFactor>> 8)&0xFF ) * 1.f/255.f;
            float b = float( (iBlendFactor    )&0xFF ) * 1.f/255.f;
            oFactor[0] = r; oFactor[1] = g;
            oFactor[2] = b; oFactor[3] = a;
        } break;
        case( D3DBLEND_INVBLENDFACTOR ):
        {
            float a = float( (iBlendFactor>>24)&0xFF ) * 1.f/255.f;
            float r = float( (iBlendFactor>>16)&0xFF ) * 1.f/255.f;
            float g = float( (iBlendFactor>> 8)&0xFF ) * 1.f/255.f;
            float b = float( (iBlendFactor    )&0xFF ) * 1.f/255.f;
            oFactor[0] = 1.f - r; oFactor[1] = 1.f - g;
            oFactor[2] = 1.f - b; oFactor[3] = 1.f - a;
        }break;

        case( D3DBLEND_SRCCOLOR2 ):
        case( D3DBLEND_INVSRCCOLOR2 ):
        case( D3DBLEND_BOTHSRCALPHA ):
        case( D3DBLEND_BOTHINVSRCALPHA ):
        default:
            return false;
        }

        return true;
    }

    bool ApplyD3DBlending( DWORD iSrcBlend, DWORD iDstBlend, DWORD iBlendOp, DWORD iSepAlpha,
                           DWORD iAlphaSrcBlend, DWORD iAlphaDstBlend, DWORD iBlendOpAlpha,
                           DWORD iBlendFactor, const float* iBufferColor, const float* iColor, float* oColor )
    {
        float srcFactor[4], dstFactor[4];
        float srcFactorAlpha[4], dstFactorAlpha[4];
        if( GetBlendFactor( iSrcBlend, iBlendFactor, iBufferColor, iColor, srcFactor ) &&
            GetBlendFactor( iDstBlend, iBlendFactor, iBufferColor, iColor, dstFactor ) )
        {
            if( iSepAlpha && 
                (!GetBlendFactor( iAlphaSrcBlend, iBlendFactor, iBufferColor, iColor, srcFactorAlpha ) ||
                 !GetBlendFactor( iAlphaDstBlend, iBlendFactor, iBufferColor, iColor, dstFactorAlpha ) ) )
            {
                return false;
            }

            for( int ii = 0; ii < 4; ++ii )
            {
                oColor[ii] = iColor[ii] * ( ( (ii == 3) && iSepAlpha ) ? srcFactorAlpha[ii] : srcFactor[ii]);
            }

            for( int ii = 0; ii < 4; ++ii )
            {
                float val = iBufferColor[ii] * ( ( (ii == 3) && iSepAlpha ) ? dstFactorAlpha[ii] : dstFactor[ii]);
                DWORD blendOp = ( (ii == 3) && iSepAlpha ) ? iBlendOpAlpha : iBlendOp;
                switch( blendOp )
                {
                case(D3DBLENDOP_ADD):
                    oColor[ii] += val;
                    break;
                case(D3DBLENDOP_SUBTRACT):
                    oColor[ii] -= val;
                    break;
                case(D3DBLENDOP_REVSUBTRACT):
                    oColor[ii] = val - oColor[ii];
                    break;
                case(D3DBLENDOP_MIN):
                    oColor[ii] = min( oColor[ii], val );
                    break;
                case(D3DBLENDOP_MAX):
                    oColor[ii] = max( oColor[ii], val );
                    break;
                }
            }
            return true;
        }

        return false;
    }

    IDirect3DTexture9* GetDepthBufferTexture( IDirect3DSurface9* iSurface )
    {
        for( DepthBufferReplacement* iter = sDX9Data.mDepthBuffers; NULL != iter; iter = iter->mNext )
        {
            if( iter->mOriginal == iSurface || iter->mReplacement == iSurface )
                return iter->mTexture;
        }

        return NULL;
    }

    IDirect3DSurface9* GetDepthBufferReplacement( IDirect3DDevice9* iD3DDevice, IDirect3DSurface9* iSurface )
    {
        if( sDX9Data.mNoDepthReplacements )
            return iSurface;

        for( DepthBufferReplacement* iter = sDX9Data.mDepthBuffers; NULL != iter; iter = iter->mNext )
        {
            if( iter->mOriginal == iSurface || iSurface == iter->mReplacement )
                return iter->mReplacement;
        }

        D3DSURFACE_DESC desc;
        iSurface->GetDesc( &desc );
        D3DFORMAT fmt;
        switch( desc.Format )
        {
        case( D3DFMT_D16 ):
        case( D3DFMT_D32 ):
        case( D3DFMT_D24X8 ):
        case( D3DFMT_D24S8 ): fmt = (D3DFORMAT)MAKEFOURCC( 'I', 'N', 'T', 'Z' ); break;
        default: return iSurface; // Can't replace certain types of depth buffer formats.
        }

        ScopedRelease<IDirect3DTexture9> newTexture;
        HRESULT res = iD3DDevice->CreateTexture( desc.Width, desc.Height, 1, D3DUSAGE_DEPTHSTENCIL, fmt, D3DPOOL_DEFAULT,
            &newTexture.mPtr, NULL );

        if( res != S_OK )
            return iSurface;

        DepthBufferReplacement* rep = (DepthBufferReplacement*)Dissector::MallocCallback( sizeof(DepthBufferReplacement) );
        rep->mNext = sDX9Data.mDepthBuffers;
        rep->mOriginal = iSurface;
        rep->mTexture = newTexture.mPtr;
        newTexture->GetSurfaceLevel( 0, &rep->mReplacement );
        ULONG ref = rep->mTexture->AddRef();

        sDX9Data.mDepthBuffers = rep;

        return rep->mReplacement;
    }

    void RenderTextureToRT( IDirect3DDevice9* iD3DDevice, IDirect3DBaseTexture9* iTexture, IDirect3DSurface9* iRenderTarget,
        IDirect3DPixelShader9* overrideShader, bool iInFrame, float* iExtraConstants, int iNumConstants )
    {
        if( !iInFrame )
            iD3DDevice->BeginScene();

        ResetDevice( iD3DDevice );
        ResetSampler( iD3DDevice, 0 );
        HRESULT res = iD3DDevice->SetRenderTarget( 0, iRenderTarget );
        res = iD3DDevice->SetTexture( 0, iTexture );
        res = iD3DDevice->SetPixelShader( overrideShader ? overrideShader : sDX9Data.mTexPS );
        ResetSampler( iD3DDevice, 0 );
        res = iD3DDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
        res = iD3DDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );
        res = iD3DDevice->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_POINT );

        D3DSURFACE_DESC desc;
        iRenderTarget->GetDesc( &desc );

        float uvOffset[4];
        uvOffset[0] = .5f / desc.Width;
        uvOffset[1] = .5f / desc.Height;
        uvOffset[2] = uvOffset[3] = 0.f;

        iD3DDevice->SetPixelShaderConstantF( 0, uvOffset, 1 );
        if( iExtraConstants && iNumConstants )
        {
            iD3DDevice->SetPixelShaderConstantF( 1, iExtraConstants, iNumConstants );
        }

        DrawFullScreenQuad( iD3DDevice );

        if( !iInFrame )
            iD3DDevice->EndScene();
    }

    UINT GetVertexCountFromPrimitiveCount( D3DPRIMITIVETYPE iPrimitiveType, UINT iNumElements )
    {
        switch( iPrimitiveType )
        {
        case(D3DPT_POINTLIST):      return iNumElements;
        case(D3DPT_LINELIST):       return iNumElements * 2;
        case(D3DPT_LINESTRIP):      return iNumElements + 1;
        case(D3DPT_TRIANGLELIST):   return iNumElements * 3;
        case(D3DPT_TRIANGLESTRIP):  return iNumElements + 2;
        case(D3DPT_TRIANGLEFAN):    return iNumElements + 2;
        default: break;
        }

        return 0;
    }

    UINT GetPrimtiveCountFromVertexCount( D3DPRIMITIVETYPE iPrimitiveType, UINT iNumVertices )
    {
        switch( iPrimitiveType )
        {
        case(D3DPT_POINTLIST):      return iNumVertices;
        case(D3DPT_LINELIST):       return iNumVertices / 2;
        case(D3DPT_LINESTRIP):      return iNumVertices - 1;
        case(D3DPT_TRIANGLELIST):   return iNumVertices / 3;
        case(D3DPT_TRIANGLESTRIP):  return iNumVertices - 2;
        case(D3DPT_TRIANGLEFAN):    return iNumVertices - 2;
        default: break;
        }

        return 0;
    }

    Dissector::PrimitiveType::Type GetDissectorPrimitiveType( D3DPRIMITIVETYPE iPrimitiveType )
    {
        switch( iPrimitiveType )
        {
        case(D3DPT_POINTLIST):      return Dissector::PrimitiveType::PointList;
        case(D3DPT_LINELIST):       return Dissector::PrimitiveType::LineList;
        case(D3DPT_LINESTRIP):      return Dissector::PrimitiveType::LineStrip;
        case(D3DPT_TRIANGLELIST):   return Dissector::PrimitiveType::TriangleList;
        case(D3DPT_TRIANGLESTRIP):  return Dissector::PrimitiveType::TriangleStrip;
        case(D3DPT_TRIANGLEFAN):
        default: return Dissector::PrimitiveType::Unknown;
        }
    }

    void AddCapturedAssetHandle( IUnknown* iAsset )
    {
        if( !iAsset )
            return;

        if( sDX9Data.mAssetHandles ) 
        {
            for( unsigned int ii = 0; ii < sDX9Data.mAssetHandlesCount; ++ii )
            {
                if( sDX9Data.mAssetHandles[ii] == iAsset )
                    return;
            }
        }

        if( (sDX9Data.mAssetHandlesCount + 1) >= sDX9Data.mAssetHandlesSize )
        {
            unsigned int oldSize = sDX9Data.mAssetHandlesSize;
            if( sDX9Data.mAssetHandlesSize < 32 )
                sDX9Data.mAssetHandlesSize = 32;
            else
                sDX9Data.mAssetHandlesSize *= 2;

            IUnknown** newArray = (IUnknown**)Dissector::MallocCallback( sDX9Data.mAssetHandlesSize * sizeof(IUnknown*) );
            if( sDX9Data.mAssetHandles )
            {
                memcpy( newArray, sDX9Data.mAssetHandles, sizeof(IUnknown*) * oldSize );
                Dissector::FreeCallback( sDX9Data.mAssetHandles );
            }

            sDX9Data.mAssetHandles = newArray;
        }

        iAsset->AddRef();
        sDX9Data.mAssetHandles[sDX9Data.mAssetHandlesCount] = iAsset;
        sDX9Data.mAssetHandlesCount++;
    }
};
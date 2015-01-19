#include "DX9Capture.h"
#include "DX9Tests.h"
#include "DX9Helpers.h"
#include "DX9Capture.h"

namespace DissectorDX9
{
    inline void AddRenderStateHeader( unsigned int iType, char*& iDataIter, unsigned int& iDataSize )
    {
        int size = sizeof(unsigned int);
        memcpy( iDataIter, &iType, size );
        iDataIter += size;
        iDataSize += size;
        *((unsigned int*)(iDataIter)) = 0; // flags
        iDataIter += size;
        iDataSize += size;
    }

    inline void AddRenderStateData( void* iData, unsigned int iSize, char*& iDataIter, unsigned int& iDataSize )
    {
        // Write the size of the data.
        int intSize = sizeof(unsigned int);
        memcpy( iDataIter, &iSize, intSize );
        iDataIter += intSize;
        iDataSize += intSize;

        // Write the data
        memcpy( iDataIter, iData, iSize );
        iDataIter += iSize;
        iDataSize += iSize;
    }

    void GetClipPlane( IDirect3DDevice9* iD3DDevice, char*& iDataIter, unsigned int& iDataSize )
    {
        int planeNum = 0;
        float clipPlane[16];
        
        do
        {
            HRESULT res = iD3DDevice->GetClipPlane( planeNum, &clipPlane[planeNum*4] );
            if( res == D3D_OK )
            {
                ++planeNum;
            }
            else
            {
                break;
            }
        } while( planeNum < 4 );

        for( int ii = 0; ii < 16; ++ii )
        {
            AddRenderStateHeader( RT_CLIPPLANE_BEGIN + ii, iDataIter, iDataSize );
            AddRenderStateData( &clipPlane[ii], sizeof(float), iDataIter, iDataSize );
        }
    }

    void GetClipStatus( IDirect3DDevice9* iD3DDevice, char*& iDataIter, unsigned int& iDataSize )
    {
        D3DCLIPSTATUS9 clipstatus;
        if( iD3DDevice->GetClipStatus( &clipstatus ) == D3D_OK )
        {
            clipstatus.ClipIntersection &= D3DCS_ALL;
            AddRenderStateHeader( RT_CLIPSTATUS, iDataIter, iDataSize );
            AddRenderStateData( &clipstatus, sizeof(clipstatus), iDataIter, iDataSize );
        }
    }

    void SetClipStatus( IDirect3DDevice9* iD3DDevice, char* iData, unsigned int iDataSize )
    {
        assert( iDataSize == sizeof(D3DCLIPSTATUS9) );
        iD3DDevice->SetClipStatus( (D3DCLIPSTATUS9*)iData );
    }

    void GetDepthStencilSurface( IDirect3DDevice9* iD3DDevice, char*& iDataIter, unsigned int& iDataSize )
    {
        ScopedRelease<IDirect3DSurface9> surface;
        if( iD3DDevice->GetDepthStencilSurface( &surface.mPtr ) == D3D_OK )
        {
            AddRenderStateHeader( RT_DEPTHSTENCILSURFACE, iDataIter, iDataSize );
            AddRenderStateData( &surface.mPtr, sizeof(IDirect3DSurface9*), iDataIter, iDataSize );
        }
    }

    void SetDepthStencilSurface( IDirect3DDevice9* iD3DDevice, char* iData, unsigned int iDataSize )
    {
        assert( iDataSize == sizeof(IDirect3DSurface9*) );
        IDirect3DSurface9* surface = GetDepthBufferReplacement( iD3DDevice, *((IDirect3DSurface9**)iData) );
        iD3DDevice->SetDepthStencilSurface( surface );
    }

    void GetIndices( IDirect3DDevice9* iD3DDevice, char*& iDataIter, unsigned int& iDataSize )
    {
        ScopedRelease<IDirect3DIndexBuffer9> indexBuffer;
        if( iD3DDevice->GetIndices( &indexBuffer.mPtr ) == D3D_OK )
        {
            AddRenderStateHeader( RT_INDEXBUFFER, iDataIter, iDataSize );
            AddRenderStateData( &indexBuffer.mPtr, sizeof(IDirect3DIndexBuffer9*), iDataIter, iDataSize );
        }
    }

    void SetIndices( IDirect3DDevice9* iD3DDevice, char* iData, unsigned int iDataSize )
    {
        assert( iDataSize == sizeof(IDirect3DIndexBuffer9*) );
        iD3DDevice->SetIndices( *(IDirect3DIndexBuffer9**)iData );
    }

    void GetPixelShader( IDirect3DDevice9* iD3DDevice, char*& iDataIter, unsigned int& iDataSize )
    {
        ScopedRelease<IDirect3DPixelShader9> shader;
        if( iD3DDevice->GetPixelShader( &shader.mPtr ) == D3D_OK )
        {
            AddRenderStateHeader( RT_PIXELSHADER, iDataIter, iDataSize );
            AddRenderStateData( &shader.mPtr, sizeof(IDirect3DPixelShader9*), iDataIter, iDataSize );
        }
    }

    void SetPixelShader( IDirect3DDevice9* iD3DDevice, char* iData, unsigned int iDataSize )
    {
        assert( iDataSize == sizeof(IDirect3DPixelShader9*) );
        iD3DDevice->SetPixelShader( *(IDirect3DPixelShader9**)iData );
    }

    void GetPixelShaderConstantB( IDirect3DDevice9* iD3DDevice, char*& iDataIter, unsigned int& iDataSize )
    {
        BOOL data[sNumBoolRegisters];
        if( iD3DDevice->GetPixelShaderConstantB( 0, data, sNumBoolRegisters ) == D3D_OK )
        {
            for( int ii = 0; ii < sNumBoolRegisters; ++ii )
            {
                AddRenderStateHeader( RT_PIXELSHADERCONSTANTB_BEGIN + ii, iDataIter, iDataSize );
                AddRenderStateData( &data[ii], sizeof(BOOL), iDataIter, iDataSize );
            }
        }
    }

    void GetPixelShaderConstantF( IDirect3DDevice9* iD3DDevice, char*& iDataIter, unsigned int& iDataSize )
    {
        float data[4*224];
        if( iD3DDevice->GetPixelShaderConstantF( 0, data, 224 ) == D3D_OK )
        {
            for( int ii = 0; ii < 224; ++ii )
            {
                AddRenderStateHeader( RT_PIXELSHADERCONSTANTF_BEGIN + ii, iDataIter, iDataSize );
                AddRenderStateData( &data[ii * 4], 4*sizeof(float), iDataIter, iDataSize );
            }
        }
    }

    void GetPixelShaderConstantI( IDirect3DDevice9* iD3DDevice, char*& iDataIter, unsigned int& iDataSize )
    {
        int data[4*sNumIntRegisters];
        if( iD3DDevice->GetPixelShaderConstantI( 0, data, sNumIntRegisters ) == D3D_OK )
        {
            for( int ii = 0; ii < sNumIntRegisters; ++ii )
            {
                AddRenderStateHeader( RT_PIXELSHADERCONSTANTI_BEGIN + ii, iDataIter, iDataSize );
                AddRenderStateData( &data[ii * 4], sizeof(int)*4, iDataIter, iDataSize );
            }
        }
    }

    void GetRenderState( IDirect3DDevice9* iD3DDevice, char*& iDataIter, unsigned int& iDataSize )
    {
        DWORD data;
        for( int ii = 0; ii < sNumCaptureRenderStates; ++ii )
        {
            iD3DDevice->GetRenderState( sCaptureRenderStates[ii], &data );
            AddRenderStateHeader( RT_RENDERSTATE_BEGIN+ii, iDataIter, iDataSize );
            AddRenderStateData( &data, sizeof(DWORD), iDataIter, iDataSize );
        }
    }

    void GetRenderTarget( IDirect3DDevice9* iD3DDevice, char*& iDataIter, unsigned int& iDataSize )
    {
        int numRTS = min( 4, sDX9Data.mCaps.NumSimultaneousRTs );
        for( int ii = 0; ii < numRTS; ++ii )
        {
            ScopedRelease<IDirect3DSurface9> surface;
            iD3DDevice->GetRenderTarget( ii, &surface.mPtr );
            AddRenderStateHeader( RT_RENDERTARGET_BEGIN + ii, iDataIter, iDataSize );
            AddRenderStateData( &surface.mPtr, sizeof(IDirect3DSurface9*), iDataIter, iDataSize );
        }
    }

    void GetSamplerState( IDirect3DDevice9* iD3DDevice, char*& iDataIter, unsigned int& iDataSize )
    {
        DWORD data;
        for( int ii = 0; ii < sNumSamplers; ++ii )
        {
            for( int jj = 0; jj < sNumCaptureSamplerStates; ++jj )
            {
                iD3DDevice->GetSamplerState( ii, sCaptureSamplerStates[jj], &data );
                AddRenderStateHeader( RT_SAMPLER0_BEGIN + (ii * sNumCaptureSamplerStates) + jj, iDataIter, iDataSize );
                AddRenderStateData( &data, sizeof(DWORD), iDataIter, iDataSize );
            }
        }

    }

    void GetScissorRect( IDirect3DDevice9* iD3DDevice, char*& iDataIter, unsigned int& iDataSize )
    {
        RECT rec;
        if( iD3DDevice->GetScissorRect( &rec ) == D3D_OK )
        {
            for( int ii = 0; ii < 4; ++ii )
            {
                AddRenderStateHeader( RT_SCISSORRECT_BEGIN + ii, iDataIter, iDataSize );
                AddRenderStateData( &(((LONG*)&rec)[ii]), sizeof(LONG), iDataIter, iDataSize );
            }
        }
    }

    void GetStreamSource( IDirect3DDevice9* iD3DDevice, char*& iDataIter, unsigned int& iDataSize )
    {
        StreamSourceData stream;
        int numStreams = min( 16, sDX9Data.mCaps.MaxStreams );

        // Enable this for pix debugging on windows.
        //sDX9Data.mCaps.NumSimultaneousRTs = 1;

        for( int ii = 0; ii < numStreams; ++ii )
        {
            iD3DDevice->GetStreamSource( ii, &stream.mBuffer, &stream.mOffsetInBytes,
                                         &stream.mStride );
            iD3DDevice->GetStreamSourceFreq( ii, &stream.mDivider );

            AddRenderStateHeader( RT_STREAMSOURCE_BEGIN + (ii*4) + 0, iDataIter, iDataSize );
            AddRenderStateData( &stream.mBuffer, sizeof(IDirect3DVertexBuffer9*), iDataIter, iDataSize );
            AddRenderStateHeader( RT_STREAMSOURCE_BEGIN + (ii*4) + 1, iDataIter, iDataSize );
            AddRenderStateData( &stream.mOffsetInBytes, sizeof(UINT), iDataIter, iDataSize );
            AddRenderStateHeader( RT_STREAMSOURCE_BEGIN + (ii*4) + 2, iDataIter, iDataSize );
            AddRenderStateData( &stream.mStride, sizeof(UINT), iDataIter, iDataSize );
            AddRenderStateHeader( RT_STREAMSOURCE_BEGIN + (ii*4) + 3, iDataIter, iDataSize );
            AddRenderStateData( &stream.mDivider, sizeof(UINT), iDataIter, iDataSize );
            
            if( stream.mBuffer )
                stream.mBuffer->Release();
        }
    }

    void GetTexture( IDirect3DDevice9* iD3DDevice, char*& iDataIter, unsigned int& iDataSize )
    {
        int numTextures = min( 16, sDX9Data.mCaps.MaxSimultaneousTextures );
        for( int ii = 0; ii < numTextures; ++ii )
        {
            ScopedRelease<IDirect3DBaseTexture9> texture;
            iD3DDevice->GetTexture( ii, &texture.mPtr );
            AddRenderStateHeader( RT_TEXTURE_BEGIN+ii, iDataIter, iDataSize );
            AddRenderStateData( &texture.mPtr, sizeof(IDirect3DBaseTexture9*), iDataIter, iDataSize );
        }
    }

    void GetVertexDeclaration( IDirect3DDevice9* iD3DDevice, char*& iDataIter, unsigned int& iDataSize )
    {
        ScopedRelease<IDirect3DVertexDeclaration9> decl;
        iD3DDevice->GetVertexDeclaration( &decl.mPtr );

        AddRenderStateHeader( RT_VERTEXDECLARATION, iDataIter, iDataSize );
        AddRenderStateData( &decl.mPtr, sizeof(IDirect3DVertexDeclaration9*), iDataIter, iDataSize );
    }

    void SetVertexDeclaration( IDirect3DDevice9* iD3DDevice, char* iData, unsigned int iDataSize )
    {
        assert( sizeof(IDirect3DVertexDeclaration9*) == iDataSize );
        iD3DDevice->SetVertexDeclaration( *(IDirect3DVertexDeclaration9**)iData );
    }

    void GetVertexShader( IDirect3DDevice9* iD3DDevice, char*& iDataIter, unsigned int& iDataSize )
    {
        ScopedRelease<IDirect3DVertexShader9> shader;
        HRESULT res = iD3DDevice->GetVertexShader( &shader.mPtr );

        AddRenderStateHeader( RT_VERTEXSHADER, iDataIter, iDataSize );
        AddRenderStateData( &shader.mPtr, sizeof(IDirect3DVertexShader9*), iDataIter, iDataSize );
    }

    void SetVertexShader( IDirect3DDevice9* iD3DDevice, char* iData, unsigned int iDataSize )
    {
        assert( sizeof(IDirect3DVertexShader9*) == iDataSize );
        iD3DDevice->SetVertexShader( *(IDirect3DVertexShader9**)iData );
    }

    void GetVertexShaderConstantB( IDirect3DDevice9* iD3DDevice, char*& iDataIter, unsigned int& iDataSize )
    {
        BOOL data[sNumBoolRegisters];
        if( iD3DDevice->GetVertexShaderConstantB( 0, data, sNumBoolRegisters ) == D3D_OK )
        {
            for( int ii = 0; ii < sNumBoolRegisters; ++ii )
            {
                AddRenderStateHeader( RT_VERTEXSHADERCONSTANTB_BEGIN + ii, iDataIter, iDataSize );
                AddRenderStateData( &data[ii], sizeof(BOOL), iDataIter, iDataSize );
            }
        }
    }

    void GetVertexShaderConstantF( IDirect3DDevice9* iD3DDevice, char*& iDataIter, unsigned int& iDataSize )
    {
        float data[4*256];
        if( iD3DDevice->GetVertexShaderConstantF( 0, data, 256 ) == D3D_OK )
        {
            for( int ii = 0; ii < 256; ++ii )
            {
                AddRenderStateHeader( RT_VERTEXSHADERCONSTANTF_BEGIN + ii, iDataIter, iDataSize );
                AddRenderStateData( &data[ii*4], 4*sizeof(float), iDataIter, iDataSize );
            }
        }
    }

    void GetVertexShaderConstantI( IDirect3DDevice9* iD3DDevice, char*& iDataIter, unsigned int& iDataSize )
    {
        int data[4*sNumBoolRegisters];
        if( iD3DDevice->GetVertexShaderConstantI( 0, data, sNumIntRegisters ) == D3D_OK )
        {
            for( int ii = 0; ii < sNumIntRegisters; ++ii )
            {
                AddRenderStateHeader( RT_VERTEXSHADERCONSTANTI_BEGIN + ii, iDataIter, iDataSize );
                AddRenderStateData( &data[ii*4], 4*sizeof(int), iDataIter, iDataSize );
            }
        }
    }

    void GetViewport( IDirect3DDevice9* iD3DDevice, char*& iDataIter, unsigned int& iDataSize )
    {
        D3DVIEWPORT9 vp;
        if( iD3DDevice->GetViewport( &vp ) == D3D_OK )
        {
            for( int ii = 0; ii < 6; ++ii )
            {
                AddRenderStateHeader( RT_VIEWPORT_BEGIN + ii, iDataIter, iDataSize );
                AddRenderStateData( &(((DWORD*)&vp)[ii]), sizeof(DWORD), iDataIter, iDataSize );
            }
        }
    }

    void SetRenderStateInternal( IDirect3DDevice9* iD3DDevice, Dissector::RenderState* rs, char* eventData,
        int eventSize, bool &clipDataDirty, bool &scissorDirty, bool &viewportDirty, bool &streamsDirty )
    {
        if( rs->mType >= RT_CLIPPLANE_BEGIN && rs->mType < RT_CLIPPLANE_END )
        {
            int num = rs->mType - RT_CLIPPLANE_BEGIN;
            assert( num < 16 );
            sDX9Data.mClipPlane[ num ] = *(float*)eventData;
            clipDataDirty = true;
        }
        else if( rs->mType >= RT_SAMPLER0_BEGIN && rs->mType < RT_SAMPLER7_END )
        {
            int num = rs->mType - RT_SAMPLER0_BEGIN;
            int sampler = num / sNumCaptureSamplerStates;
            int prop = num % sNumCaptureSamplerStates;

            iD3DDevice->SetSamplerState( sampler, sCaptureSamplerStates[prop], *(DWORD*)eventData );
        }
        else if( rs->mType >= RT_PIXELSHADERCONSTANTF_BEGIN && rs->mType < RT_PIXELSHADERCONSTANTF_END )
        {
            int num = rs->mType - RT_PIXELSHADERCONSTANTF_BEGIN;
            iD3DDevice->SetPixelShaderConstantF( num, (float*)eventData, 1 );
        }
        else if( rs->mType >= RT_PIXELSHADERCONSTANTB_BEGIN && rs->mType < RT_PIXELSHADERCONSTANTB_END )
        {
            int num = rs->mType - RT_PIXELSHADERCONSTANTB_BEGIN;
            iD3DDevice->SetPixelShaderConstantB( num, (BOOL*)eventData, 1 );
        }
        else if( rs->mType >= RT_PIXELSHADERCONSTANTI_BEGIN && rs->mType < RT_PIXELSHADERCONSTANTI_END )
        {
            int num = rs->mType - RT_PIXELSHADERCONSTANTI_BEGIN;
            iD3DDevice->SetPixelShaderConstantI( num, (int*)eventData, 1 );
        }
        else if( rs->mType >= RT_VERTEXSHADERCONSTANTF_BEGIN && rs->mType < RT_VERTEXSHADERCONSTANTF_END )
        {
            int num = rs->mType - RT_VERTEXSHADERCONSTANTF_BEGIN;
            iD3DDevice->SetVertexShaderConstantF( num, (float*)eventData, 1 );
        }
        else if( rs->mType >= RT_VERTEXSHADERCONSTANTB_BEGIN && rs->mType < RT_VERTEXSHADERCONSTANTB_END )
        {
            int num = rs->mType - RT_VERTEXSHADERCONSTANTB_BEGIN;
            iD3DDevice->SetVertexShaderConstantB( num, (BOOL*)eventData, 1 );
        }
        else if( rs->mType >= RT_VERTEXSHADERCONSTANTI_BEGIN && rs->mType < RT_VERTEXSHADERCONSTANTI_END )
        {
            int num = rs->mType - RT_VERTEXSHADERCONSTANTI_BEGIN;
            iD3DDevice->SetVertexShaderConstantI( num, (int*)eventData, 1 );
        }
        else if( rs->mType >= RT_RENDERTARGET_BEGIN && rs->mType < RT_RENDERTARGET_END )
        {
            int num = rs->mType - RT_RENDERTARGET_BEGIN;
            int numRTS = min( 4, sDX9Data.mCaps.NumSimultaneousRTs );
            if( num < numRTS )
            {
                iD3DDevice->SetRenderTarget( num, *(IDirect3DSurface9**)eventData );
            }
        }
        else if( rs->mType >= RT_TEXTURE_BEGIN && rs->mType < RT_TEXTURE_END )
        {
            int num = rs->mType - RT_TEXTURE_BEGIN;
            iD3DDevice->SetTexture( num, *(IDirect3DBaseTexture9**)eventData );
        }
        else if( rs->mType >= RT_RENDERSTATE_BEGIN && rs->mType < RT_RENDERSTATE_END )
        {
            int num = rs->mType - RT_RENDERSTATE_BEGIN;
            iD3DDevice->SetRenderState( sCaptureRenderStates[num], *(DWORD*)eventData );
        }
        else if( rs->mType >= RT_SCISSORRECT_BEGIN && rs->mType < RT_SCISSORRECT_END )
        {
            scissorDirty = true;
            int num = rs->mType - RT_SCISSORRECT_BEGIN;
            ((LONG*)&sDX9Data.mScissorRect)[num] = *(LONG*)eventData;
        }
        else if( rs->mType >= RT_STREAMSOURCE_BEGIN && rs->mType < RT_STREAMSOURCE_END )
        {
            int num = rs->mType - RT_STREAMSOURCE_BEGIN;
            int stream = num / 4;
            int val = num % 4;
            switch( val )
            {
            case( 0 ): sDX9Data.mStreamData[stream].mBuffer = *(IDirect3DVertexBuffer9**)eventData; break;
            case( 1 ): sDX9Data.mStreamData[stream].mOffsetInBytes = *(UINT*)eventData; break;
            case( 2 ): sDX9Data.mStreamData[stream].mStride = *(UINT*)eventData; break;
            case( 3 ): sDX9Data.mStreamData[stream].mDivider = *(UINT*)eventData; break;
            }

            streamsDirty = true;
        }
        else if( rs->mType >= RT_VIEWPORT_BEGIN && rs->mType < RT_VIEWPORT_END )
        {
            viewportDirty = true;
            int num = rs->mType - RT_VIEWPORT_BEGIN;
            ((DWORD*)&sDX9Data.mViewport)[num] = *(DWORD*)eventData;
        }

        switch( rs->mType )
        {
        case( RT_CLIPSTATUS ):
            SetClipStatus( iD3DDevice, eventData, eventSize ); break;

        case( RT_DEPTHSTENCILSURFACE ):
            SetDepthStencilSurface( iD3DDevice, eventData, eventSize ); break;

        case( RT_INDEXBUFFER ):
            SetIndices( iD3DDevice, eventData, eventSize ); break;

        case( RT_PIXELSHADER ):
            SetPixelShader( iD3DDevice, eventData, eventSize ); break;

        case( RT_VERTEXDECLARATION ):
            SetVertexDeclaration( iD3DDevice, eventData, eventSize ); break;

        case( RT_VERTEXSHADER ):
            SetVertexShader( iD3DDevice, eventData, eventSize ); break;

        default:
            break;
        }
    }

    void SetRenderStates( IDirect3DDevice9* iD3DDevice, const Dissector::DrawCallData* iDC )
    {
        char* dataIter = (char*)iDC->mFirstRenderState;
        char* dataEnd = &dataIter[iDC->mSizeRenderStateData];

        bool clipDataDirty = false;
        bool scissorDirty = false;
        bool viewportDirty = false;
        bool streamsDirty = false;

        bool samplerDirty[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
        while( dataIter < dataEnd )
        {
            Dissector::RenderState* rs = (Dissector::RenderState*)dataIter;
            char* eventData = dataIter + sizeof(Dissector::RenderState);
            int   eventSize = rs->mSize;

            dataIter += sizeof(Dissector::RenderState);
            dataIter += rs->mSize;
            SetRenderStateInternal( iD3DDevice, rs, eventData, eventSize, clipDataDirty, scissorDirty, 
                viewportDirty, streamsDirty );
        }

        Dissector::ToolDataItem* iter = iDC->mToolRenderStates;
        while( iter )
        {
            SetRenderStateInternal( iD3DDevice, &iter->stateData, ((char*)iter) + sizeof(Dissector::ToolDataItem),
                iter->stateData.mSize, clipDataDirty, scissorDirty, viewportDirty, streamsDirty );
            iter = iter->nextItem;
        }

        if( clipDataDirty )
        {
            iD3DDevice->SetClipPlane( 0, &sDX9Data.mClipPlane[0] );
            iD3DDevice->SetClipPlane( 1, &sDX9Data.mClipPlane[4] );
            iD3DDevice->SetClipPlane( 2, &sDX9Data.mClipPlane[8] );
            iD3DDevice->SetClipPlane( 3, &sDX9Data.mClipPlane[12] );
        }

        if( scissorDirty )
        {
            iD3DDevice->SetScissorRect( &sDX9Data.mScissorRect );
        }

        if( viewportDirty )
        {
            iD3DDevice->SetViewport( &sDX9Data.mViewport );
        }

        if( streamsDirty )
        {
            int numStreams = min( 16, sDX9Data.mCaps.MaxStreams );

            for( int ii = 0; ii < numStreams; ++ii )
            {
                iD3DDevice->SetStreamSource( ii, sDX9Data.mStreamData[ii].mBuffer,
                    sDX9Data.mStreamData[ii].mOffsetInBytes, sDX9Data.mStreamData[ii].mStride );
                iD3DDevice->SetStreamSourceFreq( ii, sDX9Data.mStreamData[ii].mDivider );
            }
        }
    }

    void GetRenderStates( void* iDevice, int iEventType, const void* iEventData, unsigned int iDataSize )
    {
        static const int bufferSize = 24 * 1024;
        char renderStateData[ bufferSize ];
        char* dataIter = renderStateData;        
        unsigned int dataSize = 0;
        IDirect3DDevice9* D3DDevice = (IDirect3DDevice9*)iDevice;

        GetPixelShader( D3DDevice, dataIter, dataSize );
        GetVertexShader( D3DDevice, dataIter, dataSize );
        GetClipPlane( D3DDevice, dataIter, dataSize );
        GetClipStatus( D3DDevice, dataIter, dataSize );
        GetDepthStencilSurface( D3DDevice, dataIter, dataSize );
        GetIndices( D3DDevice, dataIter, dataSize );
        GetPixelShaderConstantB( D3DDevice, dataIter, dataSize );
        GetPixelShaderConstantF( D3DDevice, dataIter, dataSize );
        GetPixelShaderConstantI( D3DDevice, dataIter, dataSize );
        GetRenderState( D3DDevice, dataIter, dataSize );
        GetRenderTarget( D3DDevice, dataIter, dataSize );
        GetSamplerState( D3DDevice, dataIter, dataSize );
        GetScissorRect( D3DDevice, dataIter, dataSize );
        GetStreamSource( D3DDevice, dataIter, dataSize );
        GetTexture( D3DDevice, dataIter, dataSize );
        GetVertexDeclaration( D3DDevice, dataIter, dataSize );
        GetVertexShaderConstantB( D3DDevice, dataIter, dataSize );
        GetVertexShaderConstantF( D3DDevice, dataIter, dataSize );
        GetVertexShaderConstantI( D3DDevice, dataIter, dataSize );
        GetViewport( D3DDevice, dataIter, dataSize );

        assert( dataSize < bufferSize );

        Dissector::AddRenderStateEntries( renderStateData, dataSize );
    }

};
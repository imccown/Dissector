#include "..\Dissector\Dissector.h"
#include <assert.h>
#include <d3d9.h>
#include <d3dx9.h>
#include "ShaderDebugDX9.h"
#include "..\Dissector\DissectorHelpers.h"
#include "DX9Tests.h"
#include "DX9Helpers.h"
#include "DX9Capture.h"

namespace DissectorDX9
{
    bool TestPixelsWrittenInternal( void* iDevice, const Dissector::DrawCallData& iData,
        Dissector::PixelLocation* iPixels, unsigned int iNumPixels, bool iSetRenderStates )
    {
        ScopedRelease<IDirect3DSurface9> instrTarget;
        ScopedRelease<IDirect3DPixelShader9> debugShader;
        ScopedFree<void> shaderData;
        ScopedFree<void> shaderDataPatched;

        HRESULT res;
        IDirect3DDevice9* D3DDevice = (IDirect3DDevice9*)iDevice;

        if( iSetRenderStates )
            SetRenderStates( D3DDevice, &iData );

        D3DSURFACE_DESC desc;
        {
            ScopedRelease<IDirect3DSurface9> rt;
            D3DDevice->GetRenderTarget( 0, &rt.mPtr );
            rt->GetDesc( &desc );
        }

        // Create a blank render target
        if( ( res = D3DDevice->CreateRenderTarget( desc.Width, desc.Height, D3DFMT_A32B32G32R32F,
                                       D3DMULTISAMPLE_NONE, 0, true, &instrTarget.mPtr, NULL ) ) != D3D_OK )
        {
            return false;
        }

        // Change pixel shader to output our magic value to o0 (so we get clips on the shader).
        ScopedRelease<IDirect3DPixelShader9> shader;
        if( D3DDevice->GetPixelShader( &shader.mPtr ) != D3D_OK || !shader )
        {
            return false;
        }

        UINT shaderSize = 0;
        unsigned int shaderDataPatchedSize = 0;

        ShaderDebugDX9::ShaderInfo info;

        shader->GetFunction( NULL, &shaderSize );
        shaderData = Dissector::MallocCallback( shaderSize );
        if( !shaderData )
        {
            return false;
        }
        shader->GetFunction( shaderData, &shaderSize );

        shaderDataPatched = Dissector::MallocCallback( shaderSize + ShaderDebugDX9::ShaderPatchSize );
        if( !shaderDataPatched )
        {
            return false;
        }

        info = ShaderDebugDX9::GetShaderInfo( shaderData, shaderSize, D3DDevice );

        unsigned char outputMask = 0;
        shaderDataPatchedSize = shaderSize + ShaderDebugDX9::ShaderPatchSize;
        if( !ShaderDebugDX9::PatchShaderForDebug( ShaderDebugDX9::LastInstruction, &info, 0, shaderData, shaderSize,
            shaderDataPatched, shaderDataPatchedSize, outputMask ) )
        {
            return false;
        }

        if( (D3DDevice->CreatePixelShader( (const DWORD*)shaderDataPatched.mPtr, &debugShader.mPtr )) != D3D_OK ||
            !debugShader )
        {
            return false;
        }

        // Disable these render states to ensure our debug output gets written.
        // Find some other way to debug these.
        D3DDevice->SetRenderState( D3DRS_ALPHATESTENABLE, FALSE );
        D3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
        D3DDevice->SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_RED |
            D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE );

        D3DDevice->SetPixelShader( debugShader );

        // set the render target to a float4
        D3DDevice->SetRenderTarget( 0, instrTarget );

        // Render to the new RT
        ExecuteEvent( D3DDevice, iData );

        // Test the chosen pixel(s) for the magic value
        D3DLOCKED_RECT lrect;
        instrTarget->LockRect( &lrect, NULL, NULL );
        bool rvalue = false;
        for( unsigned int ii = 0; ii < iNumPixels; ++ii )
        {
            float* pixelOffset = (float*)(&((char*)lrect.pBits)[ lrect.Pitch * iPixels[ii].mY + 16*iPixels[ii].mX ]);
            if( pixelOffset[0] == -1.f &&
                pixelOffset[1] == 2.f &&
                pixelOffset[2] == 1.f &&
                pixelOffset[3] == 0.f )
            {
                rvalue = true;
                break;
            }
        }

        return rvalue;
    }

    DWORD GetNumPixelsWritten( void* iDevice, const Dissector::DrawCallData& iData, bool iSetRenderStates )
    {
        IDirect3DDevice9* D3DDevice = (IDirect3DDevice9*)iDevice;

        if( iSetRenderStates )
            SetRenderStates( D3DDevice, &iData );

        DWORD numberOfPixelsDrawn;

        sDX9Data.mOcclusionQuery->Issue(D3DISSUE_BEGIN);
        ExecuteEvent( D3DDevice, iData );
        sDX9Data.mOcclusionQuery->Issue(D3DISSUE_END);

        while(S_FALSE == sDX9Data.mOcclusionQuery->GetData( &numberOfPixelsDrawn, sizeof(DWORD), D3DGETDATA_FLUSH ));

        return numberOfPixelsDrawn;
    }

    template< typename tType, bool iTris >
    float CheckForDegenerateIndicesTemplate( tType* iIndexPtr, int iFirstStride, int iStride, UINT iCount )
    {
        if( iCount == 0 )
            return 0.f;

        float rvalue = 0.f;

        tType* iter = iIndexPtr;
        rvalue = (iter[0] == iter[1] || iter[0] == iter[2] || (iTris && (iter[1] == iter[2]))) ? rvalue + 1.f : rvalue;
        iter += iFirstStride;

        for( UINT ii = 1; ii < iCount; ++ii, iter += iStride )
        {
            rvalue = (iter[0] == iter[1] || iter[0] == iter[2] || (iTris && (iter[1] == iter[2]))) ? rvalue + 1.f : rvalue;
        }

        rvalue /= float(iCount);

        return rvalue;
    }

    struct PrimitiveData
    {
        int firstStride;
        int stride;
        bool tris;
    };

    static PrimitiveData sPrimitiveData[7] = 
    {
        { 0, 0, false }, // invalid
        { 0, 0, false }, // D3DPT_POINTLIST
        { 2, 2, false }, // D3DPT_LINELIST
        { 1, 1, false }, // D3DPT_LINESTRIP
        { 3, 3, true  }, // D3DPT_TRIANGLELIST
        { 2, 1, true  }, // D3DPT_TRIANGLESTRIP
        { 2, 1, true  }, // D3DPT_TRIANGLEFAN
    };

    // return value is percentage of primtives that are degenerate.
    float CheckForDegenerateIndices( IDirect3DDevice9* D3DDevice, D3DPRIMITIVETYPE iPrimitiveType,
                                    UINT iPrimitiveCount )
    {
        ScopedRelease<IDirect3DIndexBuffer9> indexBuf;
        D3DDevice->GetIndices( &indexBuf.mPtr );

        D3DINDEXBUFFER_DESC desc;
        indexBuf->GetDesc( &desc );
        bool largeIndices = desc.Format == D3DFMT_INDEX32;

        unsigned int* indices;
        indexBuf->Lock( 0, 0, (void**)&indices, D3DLOCK_READONLY );

        int firstStride = sPrimitiveData[iPrimitiveType].firstStride;
        int stride = sPrimitiveData[iPrimitiveType].stride;
        bool tris = sPrimitiveData[iPrimitiveType].tris;

        if( tris )
        {
            if( largeIndices )
                return CheckForDegenerateIndicesTemplate< unsigned int, true >( (unsigned int*)indices, firstStride, stride, iPrimitiveCount );
            else
                return CheckForDegenerateIndicesTemplate< unsigned short, true >( (unsigned short*)indices, firstStride, stride, iPrimitiveCount );
        }
        else
        {
            if( largeIndices )
                return CheckForDegenerateIndicesTemplate< unsigned int, false >( (unsigned int*)indices, firstStride, stride, iPrimitiveCount );
            else
                return CheckForDegenerateIndicesTemplate< unsigned short, false >( (unsigned short*)indices, firstStride, stride, iPrimitiveCount );
        }

        indexBuf->Unlock();
        indexBuf->Release();
    }

    float CheckForDegenerateInputVerts( IDirect3DDevice9* D3DDevice, D3DPRIMITIVETYPE iPrimitiveType, UINT iPrimitiveCount,
                                        UINT iStartElement, bool iIndexed )
    {
        //TODO: Fix this.
        ScopedFree<void> data;
        UINT bufferSize;
        data = GetVertexShaderInputPositions( (void*)D3DDevice, iStartElement, iPrimitiveCount, bufferSize );
        return -1.f;
        /*
        if( !data.mPtr )
            return -1.f;

        char* vertData = (char*)data.mPtr;
        char* vertIter = vertData + pOffsetInBytes;
        char* vertEnd = vertData + bufferSize;

        int firstStride = sPrimitiveData[iPrimitiveType].firstStride;
        int vertStride = sPrimitiveData[iPrimitiveType].stride;
        bool tris = sPrimitiveData[iPrimitiveType].tris;

        float rvalue;
        if( iIndexed )
        {
            //TODO: Implement this.
        }
        else
        {
            vertData += iStartElement * streamStride;
            int curStride = firstStride;
            unsigned int numDegen = 0;
            for( UINT ii = 0; ii < iPrimitiveCount; ++ii )
            {
                char* vert0 = vertIter + pOffsetInBytes;
                char* vert1 = vertIter + streamStride + pOffsetInBytes;
                char* vert2 = vertIter + streamStride + streamStride + pOffsetInBytes;
                vertIter += streamStride * curStride;
                curStride = vertStride;

                bool degen = (memcmp( vert0, vert1, sizeInBytes ) == 0);
                degen     &= (memcmp( vert0, vert2, sizeInBytes ) == 0);
                degen     &= (memcmp( vert1, vert2, sizeInBytes ) == 0);
                if( degen )
                {
                    ++numDegen;
                }
            }

            rvalue = float(numDegen) / float(iPrimitiveCount);
        }

        return rvalue;
        */
    }

    DWORD CheckShaderClips( IDirect3DDevice9* D3DDevice, Dissector::DrawCallData* iDrawCall )
    {
        HRESULT res;
        UINT shaderSize = 0;
        UINT shaderDataPatchedSize = 0;
        ScopedFree<void> shaderData = GetPixelShaderByteCode( D3DDevice, shaderSize );

        ScopedRelease<IDirect3DPixelShader9> debugPixelShader;
        ScopedFree<void> shaderDataPatched;

        // Patch the shader to debug the instruction
        shaderDataPatched = Dissector::MallocCallback( shaderSize + ShaderDebugDX9::ShaderPatchSize );
        if( !shaderDataPatched )
        {
            return 0;
        }

        ShaderDebugDX9::ShaderInfo info = ShaderDebugDX9::GetShaderInfo( shaderData, shaderSize, D3DDevice );
        unsigned char dummyMask;
        if( !ShaderDebugDX9::PatchShaderForDebug( ShaderDebugDX9::LastInstruction, &info, ShaderDebugDX9::InstructionTypes::Clip,
            shaderData.mPtr, shaderSize, shaderDataPatched, shaderDataPatchedSize, dummyMask ) )
        {
            return 0;
        }

        if( (res = D3DDevice->CreatePixelShader( (const DWORD*)shaderDataPatched.mPtr, &debugPixelShader.mPtr )) !=
            D3D_OK || !debugPixelShader )
        {
            return 0;
        }

        D3DDevice->SetPixelShader( debugPixelShader );

        return GetNumPixelsWritten( (void*)D3DDevice, *iDrawCall, false );
    }

    DWORD CheckAlphaTest( IDirect3DDevice9* D3DDevice, Dissector::DrawCallData* iDrawCall )
    {
        D3DDevice->SetRenderState( D3DRS_ALPHATESTENABLE, FALSE );
        return GetNumPixelsWritten( (void*)D3DDevice, *iDrawCall, false );
    }

    DWORD CheckClipPlanes( IDirect3DDevice9* D3DDevice, Dissector::DrawCallData* iDrawCall )
    {
        D3DDevice->SetRenderState( D3DRS_CLIPPLANEENABLE, 0 );
        return GetNumPixelsWritten( (void*)D3DDevice, *iDrawCall, false );
    }

    DWORD CheckDepthTest( IDirect3DDevice9* D3DDevice, Dissector::DrawCallData* iDrawCall )
    {
        D3DDevice->SetRenderState( D3DRS_ZFUNC, D3DCMP_ALWAYS );
        return GetNumPixelsWritten( (void*)D3DDevice, *iDrawCall, false );
    }

    DWORD CheckStencilTest( IDirect3DDevice9* D3DDevice, Dissector::DrawCallData* iDrawCall )
    {
        D3DDevice->SetRenderState( D3DRS_STENCILENABLE, FALSE );
        return GetNumPixelsWritten( (void*)D3DDevice, *iDrawCall, false );
    }
};
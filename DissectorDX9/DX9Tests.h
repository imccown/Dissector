#pragma once
#include "DX9Internal.h"

namespace DissectorDX9
{
    bool TestPixelsWrittenInternal( void* iDevice, const Dissector::DrawCallData& iData, Dissector::PixelLocation* iPixels,
                                    unsigned int iNumPixels, bool iSetRenderStates );
    DWORD GetNumPixelsWritten( void* iDevice, const Dissector::DrawCallData& iData, bool iSetRenderStates );

    float CheckForDegenerateIndices( IDirect3DDevice9* D3DDevice, D3DPRIMITIVETYPE iPrimitiveType, UINT iPrimitiveCount );
    float CheckForDegenerateInputVerts( IDirect3DDevice9* D3DDevice, D3DPRIMITIVETYPE iPrimitiveType, UINT iPrimitiveCount,
                                        UINT iStartElement, bool iIndexed );

    // Returns number of pixels rendered when all clips are removed from a shader.
    DWORD CheckShaderClips( IDirect3DDevice9* D3DDevice, Dissector::DrawCallData* iDrawCall );
    DWORD CheckAlphaTest( IDirect3DDevice9* D3DDevice, Dissector::DrawCallData* iDrawCall );
    DWORD CheckClipPlanes( IDirect3DDevice9* D3DDevice, Dissector::DrawCallData* iDrawCall );
    DWORD CheckDepthTest( IDirect3DDevice9* D3DDevice, Dissector::DrawCallData* iDrawCall );
    DWORD CheckStencilTest( IDirect3DDevice9* D3DDevice, Dissector::DrawCallData* iDrawCall );
};
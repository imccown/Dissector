#pragma once
#include "DX9Internal.h"

namespace DissectorDX9
{
    void DrawFullScreenQuad( IDirect3DDevice9* iD3DDevice );
    void ResetDevice( IDirect3DDevice9* iD3DDevice );
    void ResetSampler( IDirect3DDevice9* iD3DDevice, unsigned int iSampler );

    void BeginFrame( void* iDevice );
    bool EndFrame( void* iDevice, bool iShowRT0 = false );
    void ResimulateEvent( void* iDevice, const Dissector::DrawCallData& iData, bool iRecordTiming,
                          Dissector::TimingData* oTiming );

    void ExecuteEvent( IDirect3DDevice9* iD3DDevice, const Dissector::DrawCallData& iData );
    Dissector::DrawCallData* ExecuteToEvent( void* iDevice, int iEventId ); // Call end frame after this.

    void* GetPixelShaderByteCode( IDirect3DDevice9* D3DDevice, UINT& oSize );
    void* GetVertexShaderByteCode( IDirect3DDevice9* D3DDevice, UINT& oSize );

    // returns true if the pixel was written and the output value is valid.
    bool  GetPixelShaderInstructionOutput( void* iDevice, const Dissector::PixelLocation& iPixel,
                                           Dissector::DrawCallData* iDrawCall, unsigned int iInstructionNum,
                                           bool iDontResetDevice, unsigned int iRemoveTypeMask, float* oValue );
    UINT* GetIndices( void* iDevice, UINT iStartElement, UINT iNumElements, UINT& oBufferSize );
    float* GetVertexShaderInputPositions( void* iDevice, UINT iStartElement, UINT iNumElements, UINT& oBufferSize );
    void* GetVertexShaderOutputPositions( void* iDevice );

    unsigned int GetFormatSize( D3DFORMAT iFormat );
    unsigned int GetFormatWriteMask( D3DFORMAT iFormat ); // bit mask of ARGB

    void ApplyColorWriteMask( DWORD iMask, float* ioColor );

    bool GetRenderTargetPixelColor( void* iDevice, unsigned int iRenderTargetNumber,
                                    const Dissector::PixelLocation& iPixel, float* oColor );

    bool GetBlendFactor( DWORD iBlend, DWORD iBlendFactor, const float* iDst, const float* iSrc, float* oFactor );

    bool ApplyD3DBlending( DWORD iSrcBlend, DWORD iDstBlend, DWORD iBlendOp, DWORD iSepAlpha,
                           DWORD iAlphaSrcBlend, DWORD iAlphaDstBlend, DWORD iBlendOpAlpha, DWORD iBlendFactor, 
                           const float* iBufferColor, const float* iColor, float* oColor );

    IDirect3DTexture9* GetDepthBufferTexture( IDirect3DSurface9* iSurface );
    IDirect3DSurface9* GetDepthBufferReplacement( IDirect3DDevice9* iD3DDevice, IDirect3DSurface9* iSurface );

    void RenderTextureToRT( IDirect3DDevice9* iD3DDevice, IDirect3DBaseTexture9* iTexture, IDirect3DSurface9* iRenderTarget,
        IDirect3DPixelShader9* overrideShader = NULL, bool iInFrame = false, float* extraConstants = NULL, int numConstants = 0 );

    UINT GetVertexCountFromPrimitiveCount( D3DPRIMITIVETYPE iPrimitiveType, UINT iNumElements );
    UINT GetPrimtiveCountFromVertexCount( D3DPRIMITIVETYPE iPrimitiveType, UINT iNumVertices );
    Dissector::PrimitiveType::Type GetDissectorPrimitiveType( D3DPRIMITIVETYPE iPrimitiveType );

    void AddCapturedAssetHandle( IUnknown* iAsset );
};

struct ScopedEndFrame
{
    ScopedEndFrame(IDirect3DDevice9* iPtr): mPtr( iPtr ){}
    ~ScopedEndFrame(){ if( mPtr ){ DissectorDX9::EndFrame( mPtr );} }

    IDirect3DDevice9* mPtr;
};


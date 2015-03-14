#pragma once
#include <d3d9.h>
#include <Dissector\Dissector.h>

namespace DissectorDX9
{
    void InternalCheckRTUsage( IDirect3DDevice9* iD3DDevice );

    enum EventTypes
    {
        ET_DRAWINDEXED = 0,
        ET_DRAW,
        ET_DRAWINDEXEDUP,
        ET_DRAWUP,
        ET_CLEAR,
        ET_ENDFRAME,            // Need this to make sure we capture all render states at the end of a frame
        ET_VERTEXLOCK,          // Needed to capture data if a vertex buffer is locked multiple times in a frame.
        ET_INDEXLOCK,
        ET_TEXTURELOCK,
        ET_SURFACELOCK,
        ET_CUBESURFACELOCK,
    };

    struct DrawBase
    {
        DrawBase( D3DPRIMITIVETYPE iPrimitiveType, UINT iPrimitiveCount, UINT iStartElement ):
            mPrimitiveType( iPrimitiveType ),
            mPrimitiveCount( iPrimitiveCount ),
            mStartElement( iStartElement )
        {
        }

        D3DPRIMITIVETYPE    mPrimitiveType;
        UINT                mPrimitiveCount;
        UINT                mStartElement;
    };

    struct DrawIndexedData : public DrawBase
    {
        INT                 mBaseVertexIndex;
        UINT                mMinIndex; 
        UINT                mNumVertices;

        DrawIndexedData( D3DPRIMITIVETYPE iPrimitiveType, INT iBaseVertexIndex, UINT iMinIndex,  UINT iNumVertices, 
                  UINT iStartIndex, UINT iPrimitiveCount ):
            DrawBase( iPrimitiveType, iPrimitiveCount, iStartIndex ),
            mBaseVertexIndex( iBaseVertexIndex ),
            mMinIndex( iMinIndex ),
            mNumVertices( iNumVertices )
            {
            }
    };

    struct DrawData : public DrawBase
    {
        DrawData( D3DPRIMITIVETYPE iPrimitiveType, UINT iStartVertex, UINT iPrimitiveCount ):
            DrawBase( iPrimitiveType, iPrimitiveCount, iStartVertex )
        {
        }
    };

    struct DrawIndexedUpData  : public DrawBase
    {
        UINT                mNumVertices;
        UINT                mNumIndices;
        D3DFORMAT           mIndexDataFormat;
        UINT                mStride;
    };

    struct DrawUpData : public DrawBase
    {
        UINT                mStride;
    };

    struct ClearData
    {
        ClearData( DWORD iFlags, D3DCOLOR iColor, float iZ, DWORD iStencil ):
            mFlags( iFlags ),
            mColor( iColor ),
            mZ( iZ ),
            mStencil( iStencil )
        {
        }

        DWORD       mFlags;
        D3DCOLOR    mColor;
        float       mZ;
        DWORD       mStencil;
    };

    struct PresentData
    {
        PresentData(){}
        PresentData( const RECT *pSourceRect, const RECT *pDestRect, HWND hDestWindowOverride, const RGNDATA *pDirtyRegion ) :
            mSourceRect( *pSourceRect ),
            mDestRect( *pDestRect ),
            mDestWindowOverride( hDestWindowOverride ),
            mDirtyRegion( *pDirtyRegion )
        {
        }

        RECT mSourceRect;
        RECT mDestRect;
        HWND mDestWindowOverride;
        RGNDATA mDirtyRegion;
    };

    struct BufferLockData
    {
        void* mBuffer;
        UINT  mDataSize;
        UINT  mOffsetToLock;
        UINT  mSizeToLock;
        DWORD mFlags;
    };

    struct SurfaceLockData
    {
        void* mBuffer;
        void* mParent;
        UINT  mFace;
        UINT  mLevel;
        UINT  mPitch;
        UINT  mHeight;
        RECT  mRect;
        DWORD mFlags;
    };

    inline HRESULT DrawIndexedPrimitive( IDirect3DDevice9* iD3DDevice, D3DPRIMITIVETYPE iType, INT iBaseVertexIndex,
                                  UINT iMinIndex,  UINT iNumVertices, UINT iStartIndex, UINT iPrimitiveCount )
    {
        if( Dissector::IsCapturing() )
        {
            DrawIndexedData dc( iType, iBaseVertexIndex, iMinIndex, iNumVertices, iStartIndex, iPrimitiveCount );
            Dissector::RegisterEvent( iD3DDevice, ET_DRAWINDEXED, (const char*)&dc, sizeof(DrawIndexedData) );
            InternalCheckRTUsage( iD3DDevice );
        }

        return iD3DDevice->DrawIndexedPrimitive( iType, iBaseVertexIndex, iMinIndex, iNumVertices, 
                                                 iStartIndex, iPrimitiveCount );
    }

    inline HRESULT DrawPrimitive( IDirect3DDevice9* iD3DDevice, D3DPRIMITIVETYPE iPrimitiveType, UINT iStartVertex, 
                           UINT iPrimitiveCount )
    {
        if( Dissector::IsCapturing() )
        {
            DrawData dc( iPrimitiveType, iStartVertex, iPrimitiveCount );
            Dissector::RegisterEvent( iD3DDevice, ET_DRAW, (const char*)&dc, sizeof(DrawData) );
            InternalCheckRTUsage( iD3DDevice );
        }

        return  iD3DDevice->DrawPrimitive( iPrimitiveType, iStartVertex, iPrimitiveCount );
    }

    inline HRESULT Clear( IDirect3DDevice9* iD3DDevice, DWORD iCount,  const D3DRECT *iRects,  
                          DWORD iFlags, D3DCOLOR iColor, float iZ, DWORD iStencil )
    {
        if( Dissector::IsCapturing() )
        {
            ClearData dc( iFlags, iColor, iZ, iStencil );
            Dissector::RegisterEvent( iD3DDevice, ET_CLEAR, (const char*)&dc, sizeof(ClearData) );
            InternalCheckRTUsage( iD3DDevice );
        }

        return iD3DDevice->Clear( iCount, iRects, iFlags, iColor, iZ, iStencil );
    }

    HRESULT DrawPrimitiveUP( IDirect3DDevice9* iD3DDevice, D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount,
                             const void *pVertexStreamZeroData, UINT VertexStreamZeroStride );

    HRESULT DrawIndexedPrimitiveUP( IDirect3DDevice9* iD3DDevice, D3DPRIMITIVETYPE PrimitiveType,
                                    UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount,
                                    const void *pIndexData, D3DFORMAT IndexDataFormat, 
                                    const void *pVertexStreamZeroData,UINT VertexStreamZeroStride );

    void VertexUnlock( IDirect3DDevice9* iD3DDevice, IDirect3DVertexBuffer9* iBuffer, UINT iDataSize, UINT OffsetToLock,
                           UINT SizeToLock, void* iData, DWORD Flags );

    void IndexUnlock( IDirect3DDevice9* iD3DDevice, IDirect3DIndexBuffer9* iBuffer, UINT iDataSize, UINT OffsetToLock,
                           UINT SizeToLock, void* iData, DWORD Flags );

    void SurfaceUnlock( IDirect3DDevice9* iD3DDevice, IDirect3DSurface9* iSurface, D3DLOCKED_RECT *pLockedRect,
                            const RECT &pRect, DWORD iFlags );

    void Texture2DUnlock( IDirect3DDevice9* iD3DDevice, IDirect3DTexture9* iTexture, UINT iLevel,
                              D3DLOCKED_RECT *pLockedRect, const RECT &pRect, DWORD iFlags );

    void TextureCubeUnlock( IDirect3DDevice9* iD3DDevice, IDirect3DCubeTexture9* iTexture, UINT Face, UINT iLevel,
                              D3DLOCKED_RECT *pLockedRect, const RECT &pRect, DWORD iFlags );


    // Setup
    void Initialize( IDirect3DDevice9* iD3DDevice );
    void Shutdown();
    void SetUIPumpFunction( bool (*iPumpFunction)() );
};
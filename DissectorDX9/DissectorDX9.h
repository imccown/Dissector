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

    // Calling this is only neccessary if you set render states between your last draw call and present that you need next frame.
    //HRESULT Present( IDirect3DDevice9* iD3DDevice, const RECT *pSourceRect, const RECT *pDestRect,
    //    HWND hDestWindowOverride, const RGNDATA *pDirtyRegion )
    //{
    //    if( Dissector::IsCapturing() )
    //    {
    //        PresentData pd( pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion );
    //        Dissector::RegisterEvent( iD3DDevice, ET_PRESENT, (const char*)&pd, sizeof(PresentData) );
    //        InternalCheckRTUsage( iD3DDevice );
    //    }

    //    return iD3DDevice->Present( pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion );
    //}


    // Setup
    void Initialize( IDirect3DDevice9* iD3DDevice );
    void Shutdown();
    void SetUIPumpFunction( bool (*iPumpFunction)() );
};
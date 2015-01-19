#pragma once
#include <assert.h>
#include "..\Dissector\Dissector.h"
#include "..\Dissector\DissectorHelpers.h"
#include <d3d9.h>
#include <d3dx9.h>

#define SAFE_RELEASE( iObject ) if( iObject ){ iObject->Release(); iObject = NULL; }

template< typename tType >
struct ScopedRelease
{
    ScopedRelease(): mPtr( NULL ){}
    ScopedRelease(tType* iPtr): mPtr( iPtr ){}
    ~ScopedRelease(){ if( mPtr ){ mPtr->Release(); mPtr = NULL; } }
    const ScopedRelease& operator=( tType* iPtr ){ mPtr = iPtr; return *this; }
    tType* operator->(){ return mPtr; }
    operator tType*(){ return mPtr; }

    tType* mPtr;
};

struct StreamSourceData
{
    IDirect3DVertexBuffer9 *mBuffer;
    UINT mOffsetInBytes;
    UINT mStride;
    UINT mDivider;
};

struct DepthBufferReplacement
{
    DepthBufferReplacement() {}
    ~DepthBufferReplacement()
    {
        if( mReplacement )
            mReplacement->Release();
    }
    IDirect3DSurface9* mOriginal;
    IDirect3DSurface9* mReplacement;
    IDirect3DTexture9* mTexture;
    DepthBufferReplacement* mNext;
};

struct RenderTargetData
{
    IDirect3DSurface9* mSurface;
    IDirect3DSurface9* mSurfaceCopy;
    RenderTargetData*  mNext;
};

struct DX9Data
{
    DX9Data() : mInitialized( 0 ) {}
    unsigned int            mInitialized;
    D3DCAPS9                mCaps;
    char                    mUPDataBuffer[2048];
    bool                    (*mPumpFunction)();

    IDirect3DQuery9*        mTimingQuery;

    IDirect3DQuery9*        mOcclusionQuery;
    IDirect3DQuery9*        mBandwidthTimingsQuery;
    IDirect3DQuery9*        mCacheUtilizationQuery;
    IDirect3DQuery9*        mPipelineTimingsQuery;
    IDirect3DQuery9*        mVcacheQuery;
    IDirect3DQuery9*        mVertexStatsQuery;
    IDirect3DQuery9*        mVertexTimingsQuery;
    IDirect3DQuery9*        mPixelTimingsQuery;

    float                   mTimingFrequency;
    IDirect3DPixelShader9*  mSingleColorShader;

    // Cached Render Data
    float                   mClipPlane[16];
    RECT                    mScissorRect;
    D3DVIEWPORT9            mViewport;
    StreamSourceData        mStreamData[16];

    // Draw Fullscreen Quad
    IDirect3DVertexBuffer9* mFullscreenQuadVerts;
    IDirect3DVertexShader9* mPosTexVS;
    IDirect3DPixelShader9*  mTexPS;
    IDirect3DPixelShader9*  mVSDebugPS;

    // Vertex Formats
    LPDIRECT3DVERTEXDECLARATION9    mPosVert;

    bool                        mNoDepthReplacements;

    // Depth Buffer Overrides (replaced with lockable formats)
    DepthBufferReplacement*     mDepthBuffers;

    // Copies of initial render targets
    RenderTargetData*           mRTCopies;

    IUnknown**                  mAssetHandles;
    unsigned int                mAssetHandlesCount;
    unsigned int                mAssetHandlesSize;
};

struct RegisterData
{
    enum
    {
        FLAG_ISFLOAT = 0x01,
        FLAG_ISVALID = 0x02,
    };

    unsigned short mOpNum;
    unsigned short mInstructionNum;
    unsigned char  mRegisterMask; // 4 bits, WZYX 
    unsigned char  mFlags;
    unsigned char  mPad[2];
    union
    {
        float mFloat[4];
        unsigned int   mInt[4];
    };
};

enum Visualizers
{
    ENUM_ZBUFFER = Dissector::RSTF_VISUALIZE_ENUM_BEGIN,
    ENUM_FILLMODE,
    ENUM_BLEND,
    ENUM_BLENDOP,
    ENUM_CULL,
    ENUM_CMP,
    ENUM_STENCILOP,
    ENUM_SAMPLER,
    ENUM_FILTER,
};

const int sNumIntRegisters = 16;
const int sNumBoolRegisters = 16;

const int sNumCaptureRenderStates = 53;
const int sNumSamplers = 8;
const int sNumCaptureSamplerStates = 12;
extern const D3DRENDERSTATETYPE sCaptureRenderStates[sNumCaptureRenderStates];
extern const D3DSAMPLERSTATETYPE sCaptureSamplerStates[sNumCaptureSamplerStates];

enum RenderStateTypes
{
    RT_INDEXBUFFER = 0,
    RT_PIXELSHADER,
    RT_DEPTHSTENCILSURFACE,
    RT_CLIPSTATUS,
    RT_VERTEXDECLARATION,
    RT_VERTEXSHADER,

    RT_CLIPPLANE_BEGIN,
    RT_CLIPPLANE_END = RT_CLIPPLANE_BEGIN + 16,

    RT_PIXELSHADERCONSTANTB_BEGIN,
    RT_PIXELSHADERCONSTANTB_END = RT_PIXELSHADERCONSTANTB_BEGIN + 16,

    RT_PIXELSHADERCONSTANTF_BEGIN,
    RT_PIXELSHADERCONSTANTF_END = RT_PIXELSHADERCONSTANTF_BEGIN + 224,

    RT_PIXELSHADERCONSTANTI_BEGIN,
    RT_PIXELSHADERCONSTANTI_END = RT_PIXELSHADERCONSTANTI_BEGIN + 16,

    RT_RENDERSTATE_BEGIN,
    RT_RS_ZENABLE = RT_RENDERSTATE_BEGIN,
    RT_RS_FILLMODE,
    RT_RS_ZWRITEENABLE,
    RT_RS_ALPHATESTENABLE,
    RT_RS_LASTPIXEL,
    RT_RS_SRCBLEND,
    RT_RS_DESTBLEND,
    RT_RS_CULLMODE,
    RT_RS_ZFUNC,
    RT_RS_ALPHAREF,
    RT_RS_ALPHAFUNC,
    RT_RS_ALPHABLENDENABLE,
    RT_RS_STENCILENABLE,
    RT_RS_STENCILFAIL,
    RT_RS_STENCILZFAIL,
    RT_RS_STENCILPASS,
    RT_RS_STENCILFUNC,
    RT_RS_STENCILREF,
    RT_RS_STENCILMASK,
    RT_RS_STENCILWRITEMASK,
    RT_RS_TEXTUREFACTOR,
    RT_RS_CLIPPING,
    RT_RS_CLIPPLANEENABLE,
    RT_RS_POINTSIZE,
    RT_RS_POINTSIZE_MIN,
    RT_RS_POINTSPRITEENABLE,
    RT_RS_POINTSCALEENABLE,
    RT_RS_POINTSCALE_A,
    RT_RS_POINTSCALE_B,
    RT_RS_POINTSCALE_C,
    RT_RS_MULTISAMPLEANTIALIAS,
    RT_RS_MULTISAMPLEMASK,
    RT_RS_POINTSIZE_MAX,
    RT_RS_COLORWRITEENABLE,
    RT_RS_BLENDOP,
    RT_RS_SCISSORTESTENABLE,
    RT_RS_SLOPESCALEDEPTHBIAS,
    RT_RS_ANTIALIASEDLINEENABLE,
    RT_RS_TWOSIDEDSTENCILMODE,
    RT_RS_CCW_STENCILFAIL,
    RT_RS_CCW_STENCILZFAIL,
    RT_RS_CCW_STENCILPASS,
    RT_RS_CCW_STENCILFUNC,
    RT_RS_COLORWRITEENABLE1,
    RT_RS_COLORWRITEENABLE2,
    RT_RS_COLORWRITEENABLE3,
    RT_RS_BLENDFACTOR,
    RT_RS_SRGBWRITEENABLE,
    RT_RS_DEPTHBIAS,
    RT_RS_SEPARATEALPHABLENDENABLE,
    RT_RS_SRCBLENDALPHA,
    RT_RS_DESTBLENDALPHA,
    RT_RS_BLENDOPALPHA,

    RT_RENDERSTATE_END = RT_RS_BLENDOPALPHA,

    RT_RENDERTARGET_BEGIN,
    RT_RENDERTARGET_END = RT_RENDERTARGET_BEGIN + 4,

    RT_SCISSORRECT_BEGIN,
    RT_SCISSORRECT_END = RT_SCISSORRECT_BEGIN + 4,

    RT_STREAMSOURCE_BEGIN,
    RT_STREAMSOURCE_END = RT_STREAMSOURCE_BEGIN + (16*4),

    RT_TEXTURE_BEGIN,
    RT_TEXTURE_END = RT_TEXTURE_BEGIN + 16,

    RT_VERTEXSHADERCONSTANTB_BEGIN,
    RT_VERTEXSHADERCONSTANTB_END = RT_VERTEXSHADERCONSTANTB_BEGIN + 16,
    RT_VERTEXSHADERCONSTANTF_BEGIN,
    RT_VERTEXSHADERCONSTANTF_END = RT_VERTEXSHADERCONSTANTF_BEGIN + 256,
    RT_VERTEXSHADERCONSTANTI_BEGIN,
    RT_VERTEXSHADERCONSTANTI_END = RT_VERTEXSHADERCONSTANTI_BEGIN + 16,

    RT_VIEWPORT_BEGIN,
    RT_VIEWPORT_END = RT_VIEWPORT_BEGIN + 6,

    RT_SAMPLER0_BEGIN,
    RT_SAMPLER1_BEGIN = RT_SAMPLER0_BEGIN + sNumCaptureSamplerStates,
    RT_SAMPLER2_BEGIN = RT_SAMPLER1_BEGIN + sNumCaptureSamplerStates,
    RT_SAMPLER3_BEGIN = RT_SAMPLER2_BEGIN + sNumCaptureSamplerStates,
    RT_SAMPLER4_BEGIN = RT_SAMPLER3_BEGIN + sNumCaptureSamplerStates,
    RT_SAMPLER5_BEGIN = RT_SAMPLER4_BEGIN + sNumCaptureSamplerStates,
    RT_SAMPLER6_BEGIN = RT_SAMPLER5_BEGIN + sNumCaptureSamplerStates,
    RT_SAMPLER7_BEGIN = RT_SAMPLER6_BEGIN + sNumCaptureSamplerStates,
    RT_SAMPLER7_END   = RT_SAMPLER7_BEGIN + sNumCaptureSamplerStates,
};

extern DX9Data sDX9Data;

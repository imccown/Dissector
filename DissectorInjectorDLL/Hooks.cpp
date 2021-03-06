#include "DissectorDX9\DissectorDX9.h"
#include "DissectorWinSock\DissectorWinSock.h"
#include "mhook\mhook-lib\mhook.h"
#include <stdio.h>
#include <assert.h>

static ULONG gDeviceBaseRefCount = 0;
static HWND gHWND;
static WNDPROC gSavedWinProc = NULL;
static bool gKeepRunning = true;
static IDirect3D9* sD3D = NULL;

interface IDirect3DDevice9Hooked;

#define LOGGING

bool gLoggingEnabled = false;
bool gLogOneFrame = false;
static DWORD last0samp = 0;

#ifdef LOGGING
#define FORCE_LOG_OUTPUT
static FILE* fOutput = NULL;
    void Log( char* iFormat, ... )
    {
        if( !gLoggingEnabled && !gLogOneFrame )
            return;
        // Slow to open and close file constantly, but I need it to flush.
#ifdef FORCE_LOG_OUTPUT
        fopen_s( &fOutput, "D:\\temp\\log.txt", "a" );
#else
        if( !fOutput ) 
        {
            fopen_s( &fOutput, "D:\\temp\\log.txt", "a" );
        }
#endif

        if( !fOutput) return;

        char buffer[1024];

        va_list args;

        va_start(args,iFormat);

        int return_status = 0;
        return_status = _vsnprintf_s( buffer, sizeof(buffer), iFormat, args );

        va_end(args);

        fwrite( buffer, strlen(buffer), 1, fOutput );
#ifdef FORCE_LOG_OUTPUT
        fclose( fOutput );
#endif
    }
#else
    void Log( char* iFormat, ... )
    {
    }
#endif

// -----------------------------------------------------------------------------
// Override of windows functions to make the render window work correctly
// -----------------------------------------------------------------------------
bool PumpFunction()
{
    MSG msg_Message;

    if(PeekMessage(&msg_Message,gHWND,0,0,PM_REMOVE))    
    {
        DispatchMessage(&msg_Message);
    }

    return gKeepRunning;
}

LRESULT CALLBACK WinProcOverride(HWND hWnd,UINT uint_Message,WPARAM wParam,LPARAM lParam)
{
    switch(uint_Message)
    {
    case WM_CLOSE:
    case WM_DESTROY:
    case WM_QUIT:
        {
            gKeepRunning = false;
        } break;
    }

    LRESULT res = CallWindowProc(gSavedWinProc, hWnd,uint_Message,wParam,lParam);
    if( !gKeepRunning )
    {
        SetWindowLongPtr( hWnd, GWLP_WNDPROC, (LONG_PTR)gSavedWinProc );
        gSavedWinProc = NULL;
    }
    return res;
}

void SlaveEnded()
{
    if( gSavedWinProc )
    {
        SetWindowLongPtr(gHWND, GWLP_WNDPROC, (LONG_PTR)gSavedWinProc);
    }
}

// -----------------------------------------------------------------------------
// Hooking Pix calls to get group information
// -----------------------------------------------------------------------------
int (WINAPI *D3DPERF_BeginEventUnhooked)( D3DCOLOR col, LPCWSTR wszName ) = NULL;
int (WINAPI *D3DPERF_EndEventUnhooked)( void ) = NULL;

int WINAPI D3DPERF_BeginEventHooked( D3DCOLOR col, LPCWSTR wszName )
{
    char dest[512];
    wcstombs_s( NULL, dest, wszName, sizeof(dest) );
    Dissector::StartDrawCallGroup( dest );

    return D3DPERF_BeginEventUnhooked( col, wszName );
}

int WINAPI D3DPERF_EndEventHooked( void )
{
    Dissector::EndDrawCallGroup();
    return D3DPERF_EndEventUnhooked();
}

// -----------------------------------------------------------------------------
// Present function that is used by swap chain and device presents
// -----------------------------------------------------------------------------
void PresentInternal( IDirect3DDevice9* iDevice, HRESULT res )
{
    DWORD val;
    iDevice->GetSamplerState( 0, D3DSAMP_ADDRESSU, &val );

    if( gLogOneFrame )
    {
        gLogOneFrame = false;
        fclose( fOutput );
        fOutput = NULL;
    }

    if( Dissector::IsCapturing() )
    {
        Dissector::EndFrame( iDevice );
        DissectorDX9::SetUIPumpFunction( PumpFunction );
        gKeepRunning = true;
        gSavedWinProc = (WNDPROC)GetWindowLongPtr( gHWND, GWLP_WNDPROC );
        if( gSavedWinProc )
        {
            SetWindowLongPtr( gHWND, GWLP_WNDPROC, (LONG_PTR)&WinProcOverride );
        }

        Dissector::SetSlaveEndingCallback( SlaveEnded );
        Dissector::StartSlave( iDevice );
    }
    else if( SUCCEEDED( res ) && Dissector::ToolWantsToCapture() )
    {
        Dissector::TriggerCapture();
        Dissector::BeginFrame( iDevice );
    }

    Dissector::FrameUpdate( iDevice );
    DissectorWinsock::FrameUpdate();
}

// -----------------------------------------------------------------------------
// Hooking vertex/index buffers to get data from mid-frame locks
// -----------------------------------------------------------------------------
interface IDirect3DVertexBuffer9Hooked : public IDirect3DVertexBuffer9
{
    IDirect3DVertexBuffer9* mBuffer;
    IDirect3DDevice9Hooked* mDeviceHooked;
    IDirect3DDevice9* mDevice;
    void* mLockData;
    UINT  mOffset;
    UINT  mSizeToLockParam;
    UINT  mBufferSize;
    DWORD mFlags;

    IDirect3DVertexBuffer9Hooked( IDirect3DVertexBuffer9* iBuffer, IDirect3DDevice9Hooked* iDeviceHooked, IDirect3DDevice9* iDevice )
    {
        mBuffer = iBuffer;
        mDevice = iDevice;
        mDeviceHooked = iDeviceHooked;
        mLockData = NULL;
        mOffset = 0;
        mSizeToLockParam = 0;
        mBufferSize = 0;
        mFlags = 0;
    }

    STDMETHOD(Lock)(THIS_ UINT OffsetToLock,UINT SizeToLock,void** ppbData,DWORD Flags)
    {
        HRESULT res = mBuffer->Lock( OffsetToLock, SizeToLock, ppbData, Flags );
        if( SUCCEEDED( res ) && Dissector::IsCapturing() )
        {
            mLockData = *ppbData;
            mOffset = OffsetToLock;
            mSizeToLockParam = SizeToLock;
            mFlags = Flags;

            if( OffsetToLock == 0 && SizeToLock == 0 )
            {
                D3DVERTEXBUFFER_DESC desc;
                mBuffer->GetDesc( &desc );
                mBufferSize = desc.Size;
            }
            else
            {
                mBufferSize = SizeToLock;
            }
        }

        return res;
    }

    STDMETHOD(Unlock)(THIS)
    {
        if( Dissector::IsCapturing() && (mFlags & D3DLOCK_READONLY) == 0 )
        {
            DissectorDX9::VertexUnlock( mDevice, mBuffer, mBufferSize, mOffset, mSizeToLockParam, mLockData, mFlags );
        }
        return mBuffer->Unlock();
    }

    STDMETHOD_(ULONG,Release)(THIS)
    {
        ULONG res = mBuffer->Release();
        if( res == 0 )
            delete this;

        return res;
    }

    STDMETHOD(GetDevice)(THIS_ IDirect3DDevice9** ppDevice)
    {
        HRESULT res = mBuffer->GetDevice( ppDevice );
        if( S_OK == res )
        {
            if( *ppDevice == mDevice )
                *ppDevice = (IDirect3DDevice9*)mDeviceHooked;
        }

        return res;
    }

    //------------------------------------------------------------------------------------------------------------------
    // Boiler-plate pass through
    //------------------------------------------------------------------------------------------------------------------
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj)
    {
        return mBuffer->QueryInterface( riid, ppvObj );
    }

    STDMETHOD_(ULONG,AddRef)(THIS)
    {
        return mBuffer->AddRef();
    }

    /*** IDirect3DResource9 methods ***/
    STDMETHOD(SetPrivateData)(THIS_ REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags)
    {
        return mBuffer->SetPrivateData( refguid, pData, SizeOfData, Flags );
    }

    STDMETHOD(GetPrivateData)(THIS_ REFGUID refguid,void* pData,DWORD* pSizeOfData)
    {
        return mBuffer->GetPrivateData( refguid, pData, pSizeOfData );
    }

    STDMETHOD(FreePrivateData)(THIS_ REFGUID refguid)
    {
        return mBuffer->FreePrivateData( refguid );
    }

    STDMETHOD_(DWORD, SetPriority)(THIS_ DWORD PriorityNew)
    {
        return mBuffer->SetPriority( PriorityNew );
    }

    STDMETHOD_(DWORD, GetPriority)(THIS)
    {
        return mBuffer->GetPriority();
    }

    STDMETHOD_(void, PreLoad)(THIS)
    {
        return mBuffer->PreLoad();
    }

    STDMETHOD_(D3DRESOURCETYPE, GetType)(THIS)
    {
        return mBuffer->GetType();
    }

    STDMETHOD(GetDesc)(THIS_ D3DVERTEXBUFFER_DESC *pDesc)
    {
        return mBuffer->GetDesc( pDesc );
    }
};

interface IDirect3DIndexBuffer9Hooked : public IDirect3DIndexBuffer9
{
    IDirect3DIndexBuffer9* mBuffer;
    IDirect3DDevice9Hooked* mDeviceHooked;
    IDirect3DDevice9* mDevice;
    void* mLockData;
    UINT  mOffset;
    UINT  mSizeToLockParam;
    UINT  mBufferSize;
    DWORD mFlags;

    IDirect3DIndexBuffer9Hooked( IDirect3DIndexBuffer9* iBuffer, IDirect3DDevice9Hooked* iDeviceHooked, IDirect3DDevice9* iDevice )
    {
        mBuffer = iBuffer;
        mDevice = iDevice;
        mDeviceHooked = iDeviceHooked;
        mLockData = NULL;
        mOffset = 0;
        mSizeToLockParam = 0;
        mBufferSize = 0;
        mFlags = 0;
    }


    STDMETHOD(Lock)(THIS_ UINT OffsetToLock,UINT SizeToLock,void** ppbData,DWORD Flags)
    {
        HRESULT res = mBuffer->Lock( OffsetToLock, SizeToLock, ppbData, Flags );
        if( SUCCEEDED( res ) && Dissector::IsCapturing() )
        {
            mLockData = *ppbData;
            mOffset = OffsetToLock;
            mSizeToLockParam = SizeToLock;
            mFlags = Flags;

            if( OffsetToLock == 0 && SizeToLock == 0 )
            {
                D3DINDEXBUFFER_DESC desc;
                mBuffer->GetDesc( &desc );
                mBufferSize = desc.Size;
            }
            else
            {
                mBufferSize = SizeToLock;
            }
        }

        return res;
    }

    STDMETHOD(Unlock)(THIS)
    {
        if( Dissector::IsCapturing() && (mFlags & D3DLOCK_READONLY) == 0 )
        {
            DissectorDX9::IndexUnlock( mDevice, mBuffer, mBufferSize, mOffset, mSizeToLockParam, mLockData, mFlags );
        }
        return mBuffer->Unlock();
    }

    STDMETHOD_(ULONG,Release)(THIS)
    {
        ULONG res = mBuffer->Release();
        if( res == 0 )
            delete this;

        return res;
    }

    STDMETHOD(GetDevice)(THIS_ IDirect3DDevice9** ppDevice)
    {
        HRESULT res = mBuffer->GetDevice( ppDevice );
        if( S_OK == res )
        {
            if( *ppDevice == mDevice )
                *ppDevice = (IDirect3DDevice9*)mDeviceHooked;
        }

        return res;
    }

    //------------------------------------------------------------------------------------------------------------------
    // Boiler-plate pass through
    //------------------------------------------------------------------------------------------------------------------
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj)
    {
        return mBuffer->QueryInterface( riid, ppvObj );
    }
    STDMETHOD_(ULONG,AddRef)(THIS)
    {
        return mBuffer->AddRef();
    }

    /*** IDirect3DResource9 methods ***/
    STDMETHOD(SetPrivateData)(THIS_ REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags)
    {
        return mBuffer->SetPrivateData( refguid, pData, SizeOfData, Flags );
    }
    STDMETHOD(GetPrivateData)(THIS_ REFGUID refguid,void* pData,DWORD* pSizeOfData)
    {
        return mBuffer->GetPrivateData( refguid, pData, pSizeOfData );
    }
    STDMETHOD(FreePrivateData)(THIS_ REFGUID refguid)
    {
        return mBuffer->FreePrivateData(refguid);
    }
    STDMETHOD_(DWORD, SetPriority)(THIS_ DWORD PriorityNew)
    {
        return mBuffer->SetPriority( PriorityNew );
    }
    STDMETHOD_(DWORD, GetPriority)(THIS)
    {
        return mBuffer->GetPriority();
    }
    STDMETHOD_(void, PreLoad)(THIS)
    {
        return mBuffer->PreLoad();
    }
    STDMETHOD_(D3DRESOURCETYPE, GetType)(THIS)
    {
        return mBuffer->GetType();
    }
    STDMETHOD(GetDesc)(THIS_ D3DINDEXBUFFER_DESC *pDesc)
    {
        return mBuffer->GetDesc( pDesc );
    }
};


// -----------------------------------------------------------------------------
// Hooking Texture Cube and Surface to get lock information
// -----------------------------------------------------------------------------
interface IDirect3DSurface9Hooked : public IDirect3DSurface9
{
    IDirect3DSurface9* mSurface;
    IDirect3DDevice9Hooked* mDeviceHooked;
    IDirect3DDevice9*  mDevice;
    D3DLOCKED_RECT*    mLockedRect;
    RECT               mRect;
    DWORD              mFlags;

    IDirect3DSurface9Hooked( IDirect3DSurface9* iSurface, IDirect3DDevice9Hooked* iDeviceHooked, IDirect3DDevice9* iDevice )
    {
        mSurface = iSurface;
        mDevice = iDevice;
        mDeviceHooked = iDeviceHooked;
        mLockedRect = NULL;
        mFlags = 0;
    }

    STDMETHOD_(ULONG,Release)(THIS)
    {
        ULONG res = mSurface->Release();
        if( res == 0 )
            delete this;

        return res;
    }

    STDMETHOD(LockRect)(THIS_ D3DLOCKED_RECT* pLockedRect,CONST RECT* pRect,DWORD Flags)
    {
        HRESULT res = mSurface->LockRect( pLockedRect, pRect, Flags );
        if( SUCCEEDED( res ) && Dissector::IsCapturing() )
        {
            mLockedRect = pLockedRect;
            mFlags = Flags;

            if( pRect )
            {
                mRect = *pRect;
            }
            else 
            {
                D3DSURFACE_DESC desc;
                mSurface->GetDesc( &desc );

                mRect.left = 0;
                mRect.top = 0;
                mRect.right = desc.Width;
                mRect.bottom = desc.Height;
            }
        }

        return res;
    }

    STDMETHOD(UnlockRect)(THIS)
    {
        if( Dissector::IsCapturing() && (mFlags & D3DLOCK_READONLY) == 0 )
        {
            DissectorDX9::SurfaceUnlock( mDevice, mSurface, mLockedRect, mRect, mFlags );
        }
        return mSurface->UnlockRect();
    }

    STDMETHOD(GetDevice)(THIS_ IDirect3DDevice9** ppDevice)
    {
        HRESULT res = mSurface->GetDevice( ppDevice );
        if( S_OK == res )
        {
            if( *ppDevice == mDevice )
                *ppDevice = (IDirect3DDevice9*)mDeviceHooked;
        }

        return res;
    }

    //------------------------------------------------------------------------------------------------------------------
    // Boiler-plate pass through
    //------------------------------------------------------------------------------------------------------------------
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj)
    {
        return mSurface->QueryInterface( riid, ppvObj );
    }
    STDMETHOD_(ULONG,AddRef)(THIS)
    {
        return mSurface->AddRef();
    }

    /*** IDirect3DResource9 methods ***/
    STDMETHOD(SetPrivateData)(THIS_ REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags)
    {
        return mSurface->SetPrivateData( refguid, pData, SizeOfData, Flags );
    }
    STDMETHOD(GetPrivateData)(THIS_ REFGUID refguid,void* pData,DWORD* pSizeOfData)
    {
        return mSurface->GetPrivateData( refguid, pData, pSizeOfData );
    }
    STDMETHOD(FreePrivateData)(THIS_ REFGUID refguid)
    {
        return mSurface->FreePrivateData( refguid );
    }
    STDMETHOD_(DWORD, SetPriority)(THIS_ DWORD PriorityNew)
    {
        return mSurface->SetPriority( PriorityNew );
    }
    STDMETHOD_(DWORD, GetPriority)(THIS)
    {
        return mSurface->GetPriority();
    }
    STDMETHOD_(void, PreLoad)(THIS)
    {
        return mSurface->PreLoad();
    }
    STDMETHOD_(D3DRESOURCETYPE, GetType)(THIS)
    {
        return mSurface->GetType();
    }
    STDMETHOD(GetContainer)(THIS_ REFIID riid,void** ppContainer)
    {
        return mSurface->GetContainer( riid, ppContainer );
    }
    STDMETHOD(GetDesc)(THIS_ D3DSURFACE_DESC *pDesc)
    {
        return mSurface->GetDesc( pDesc );
    }
    STDMETHOD(GetDC)(THIS_ HDC *phdc)
    {
        return mSurface->GetDC( phdc );
    }
    STDMETHOD(ReleaseDC)(THIS_ HDC hdc)
    {
        return mSurface->ReleaseDC( hdc );
    }
};

interface IDirect3DTexture9Hooked : public IDirect3DTexture9
{
    IDirect3DTexture9*        mTexture;
    IDirect3DDevice9Hooked*   mDeviceHooked;
    IDirect3DDevice9*         mDevice;
    struct LockData
    {
        UINT                      mLevel;
        D3DLOCKED_RECT*           mLockedRect;
        RECT                      mRect;
        DWORD                     mFlags;
        LockData*                 mNext;
    };

    LockData*                 mLocks;

    IDirect3DTexture9Hooked( IDirect3DTexture9* iTexture, IDirect3DDevice9Hooked* iDeviceHooked, IDirect3DDevice9* iDevice )
    {
        mTexture = iTexture;
        mDeviceHooked = iDeviceHooked;
        mDevice = iDevice;
        mLocks = NULL;
    }

    STDMETHOD_(ULONG,Release)(THIS)
    {
        ULONG res = mTexture->Release();
        if( res == 0 )
            delete this;

        return res;
    }

    STDMETHOD(GetSurfaceLevel)(THIS_ UINT Level,IDirect3DSurface9** ppSurfaceLevel)
    {
        HRESULT res = mTexture->GetSurfaceLevel( Level, ppSurfaceLevel );
        if( SUCCEEDED( res ) && *ppSurfaceLevel )
        {
            *ppSurfaceLevel = new IDirect3DSurface9Hooked( *ppSurfaceLevel, mDeviceHooked, mDevice );
        }

        return res;
    }

    STDMETHOD(LockRect)(THIS_ UINT Level,D3DLOCKED_RECT* pLockedRect,CONST RECT* pRect,DWORD Flags)
    {
        HRESULT res = mTexture->LockRect( Level, pLockedRect, pRect, Flags );
        if( SUCCEEDED( res ) && Dissector::IsCapturing() )
        {
            LockData* ld = new LockData;
            ld->mLevel = Level;
            ld->mLockedRect = pLockedRect;
            ld->mFlags = Flags;
            ld->mNext = mLocks;

            if( pRect )
            {
                ld->mRect = *pRect;
            }
            else
            {
                IDirect3DSurface9* surf;
                if( S_OK == mTexture->GetSurfaceLevel( 0, &surf ) )
                {
                    D3DSURFACE_DESC desc;
                    surf->GetDesc( &desc );

                    ld->mRect.left = ld->mRect.top = 0;
                    ld->mRect.right = desc.Width;
                    ld->mRect.bottom = desc.Height;
                }
                else
                {
                    ld->mRect.left = ld->mRect.right = ld->mRect.top = ld->mRect.bottom = 0;
                }
            }

            mLocks = ld;
        }

        return res;
    }

    STDMETHOD(UnlockRect)(THIS_ UINT Level)
    {
        if( Dissector::IsCapturing() )
        {
            LockData* iter = mLocks;
            while( iter ) {
                if( iter->mLevel == Level ) {
                    DissectorDX9::Texture2DUnlock( mDevice, mTexture, Level, iter->mLockedRect, iter->mRect, iter->mFlags );
                    break;
                }
                iter = iter->mNext;
            }
        }
        return mTexture->UnlockRect( Level );
    }

    STDMETHOD(GetDevice)(THIS_ IDirect3DDevice9** ppDevice)
    {
        HRESULT res = mTexture->GetDevice( ppDevice );
        if( S_OK == res )
        {
            if( *ppDevice == mDevice )
                *ppDevice = (IDirect3DDevice9*)mDeviceHooked;
        }

        return res;
    }

    //------------------------------------------------------------------------------------------------------------------
    // Boiler-plate pass through
    //------------------------------------------------------------------------------------------------------------------
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj)
    {
        return mTexture->QueryInterface( riid, ppvObj );
    }
    STDMETHOD_(ULONG,AddRef)(THIS)
    {
        return mTexture->AddRef();
    }

    /*** IDirect3DBaseTexture9 methods ***/
    STDMETHOD(SetPrivateData)(THIS_ REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags)
    {
        return mTexture->SetPrivateData( refguid, pData, SizeOfData, Flags );
    }
    STDMETHOD(GetPrivateData)(THIS_ REFGUID refguid,void* pData,DWORD* pSizeOfData)
    {
        return mTexture->GetPrivateData( refguid, pData, pSizeOfData );
    }
    STDMETHOD(FreePrivateData)(THIS_ REFGUID refguid)
    {
        return mTexture->FreePrivateData( refguid );
    }
    STDMETHOD_(DWORD, SetPriority)(THIS_ DWORD PriorityNew)
    {
        return mTexture->SetPriority( PriorityNew );
    }
    STDMETHOD_(DWORD, GetPriority)(THIS)
    {
        return mTexture->GetPriority();
    }
    STDMETHOD_(void, PreLoad)(THIS)
    {
        return mTexture->PreLoad();
    }
    STDMETHOD_(D3DRESOURCETYPE, GetType)(THIS)
    {
        return mTexture->GetType();
    }
    STDMETHOD_(DWORD, SetLOD)(THIS_ DWORD LODNew)
    {
        return mTexture->SetLOD( LODNew );
    }
    STDMETHOD_(DWORD, GetLOD)(THIS)
    {
        return mTexture->GetLOD();
    }
    STDMETHOD_(DWORD, GetLevelCount)(THIS)
    {
        return mTexture->GetLevelCount();
    }
    STDMETHOD(SetAutoGenFilterType)(THIS_ D3DTEXTUREFILTERTYPE FilterType)
    {
        return mTexture->SetAutoGenFilterType( FilterType );
    }
    STDMETHOD_(D3DTEXTUREFILTERTYPE, GetAutoGenFilterType)(THIS)
    {
        return mTexture->GetAutoGenFilterType();
    }
    STDMETHOD_(void, GenerateMipSubLevels)(THIS)
    {
        return mTexture->GenerateMipSubLevels();
    }
    STDMETHOD(GetLevelDesc)(THIS_ UINT Level,D3DSURFACE_DESC *pDesc)
    {
        return mTexture->GetLevelDesc( Level, pDesc );
    }
    STDMETHOD(AddDirtyRect)(THIS_ CONST RECT* pDirtyRect)
    {
        return mTexture->AddDirtyRect( pDirtyRect );
    }
};

interface IDirect3DCubeTexture9Hooked : public IDirect3DCubeTexture9
{
    IDirect3DCubeTexture9*  mCube;
    IDirect3DDevice9Hooked* mDeviceHooked;
    IDirect3DDevice9*       mDevice;

    struct LockData
    {
        D3DCUBEMAP_FACES    mFace;
        UINT                mLevel;
        D3DLOCKED_RECT*     mLockedRect;
        RECT                mRect;
        DWORD               mFlags;
        LockData*           mNext;
    };
    LockData*               mLocks;

    IDirect3DCubeTexture9Hooked( IDirect3DCubeTexture9* iCube, IDirect3DDevice9Hooked* iDeviceHooked, IDirect3DDevice9* iDevice )
    {
        mCube = iCube;
        mDeviceHooked = iDeviceHooked;
        mDevice = iDevice;
        mLocks = NULL;
    }

    STDMETHOD_(ULONG,Release)(THIS)
    {
        ULONG res = mCube->Release();
        if( res == 0 )
            delete this;

        return res;
    }

    STDMETHOD(GetCubeMapSurface)(THIS_ D3DCUBEMAP_FACES FaceType,UINT Level,IDirect3DSurface9** ppCubeMapSurface)
    {
        HRESULT res = mCube->GetCubeMapSurface( FaceType, Level, ppCubeMapSurface );
        if( SUCCEEDED( res ) && *ppCubeMapSurface )
        {
            *ppCubeMapSurface = new IDirect3DSurface9Hooked( *ppCubeMapSurface, mDeviceHooked, mDevice );
        }

        return res;
    }

    STDMETHOD(LockRect)(THIS_ D3DCUBEMAP_FACES FaceType,UINT Level,D3DLOCKED_RECT* pLockedRect,CONST RECT* pRect,DWORD Flags)
    {
        HRESULT res = mCube->LockRect( FaceType, Level, pLockedRect, pRect, Flags );
        if( SUCCEEDED( res ) && pLockedRect )        
        {
            LockData* ld = new LockData;
            ld->mFace = FaceType;
            ld->mLevel = Level;
            ld->mLockedRect = pLockedRect;
            ld->mFlags = Flags;
            ld->mNext = mLocks;

            if( pRect )
            {
                ld->mRect = *pRect;
            }
            else
            {
                IDirect3DSurface9* surf;
                if( S_OK == mCube->GetCubeMapSurface( FaceType, Level, &surf ) )
                {
                    D3DSURFACE_DESC desc;
                    surf->GetDesc( &desc );

                    ld->mRect.left = ld->mRect.top = 0;
                    ld->mRect.right = desc.Width;
                    ld->mRect.bottom = desc.Height;
                }
                else
                {
                    ld->mRect.left = ld->mRect.right = ld->mRect.top = ld->mRect.bottom = 0;
                }
            }

            mLocks = ld;
        }

        return res;
    }

    STDMETHOD(UnlockRect)(THIS_ D3DCUBEMAP_FACES FaceType,UINT Level)
    {
        if( Dissector::IsCapturing() )
        {
            LockData* iter = mLocks;
            while( iter ) {
                if( iter->mFace == FaceType && iter->mLevel == Level ) {
                    DissectorDX9::TextureCubeUnlock( mDevice, mCube, iter->mFace, iter->mLevel, iter->mLockedRect, iter->mRect, iter->mFlags );
                    break;
                }
                iter = iter->mNext;
            }
        }

        return mCube->UnlockRect( FaceType, Level );
    }

    STDMETHOD(GetDevice)(THIS_ IDirect3DDevice9** ppDevice)
    {
        HRESULT res = mCube->GetDevice( ppDevice );
        if( S_OK == res )
        {
            if( *ppDevice == mDevice )
                *ppDevice = (IDirect3DDevice9*)mDeviceHooked;
        }

        return res;
    }

    //------------------------------------------------------------------------------------------------------------------
    // Boiler-plate pass through
    //------------------------------------------------------------------------------------------------------------------
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj)
    {
        return mCube->QueryInterface( riid, ppvObj );
    }
    STDMETHOD_(ULONG,AddRef)(THIS)
    {
        return mCube->AddRef();
    }

    /*** IDirect3DBaseTexture9 methods ***/
    STDMETHOD(SetPrivateData)(THIS_ REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags)
    {
        return mCube->SetPrivateData( refguid, pData, SizeOfData, Flags );
    }
    STDMETHOD(GetPrivateData)(THIS_ REFGUID refguid,void* pData,DWORD* pSizeOfData)
    {
        return mCube->GetPrivateData( refguid, pData, pSizeOfData );
    }
    STDMETHOD(FreePrivateData)(THIS_ REFGUID refguid)
    {
        return mCube->FreePrivateData( refguid );
    }
    STDMETHOD_(DWORD, SetPriority)(THIS_ DWORD PriorityNew)
    {
        return mCube->SetPriority( PriorityNew );
    }
    STDMETHOD_(DWORD, GetPriority)(THIS)
    {
        return mCube->GetPriority();
    }
    STDMETHOD_(void, PreLoad)(THIS)
    {
        return mCube->PreLoad();
    }
    STDMETHOD_(D3DRESOURCETYPE, GetType)(THIS)
    {
        return mCube->GetType();
    }
    STDMETHOD_(DWORD, SetLOD)(THIS_ DWORD LODNew)
    {
        return mCube->SetLOD( LODNew );
    }
    STDMETHOD_(DWORD, GetLOD)(THIS)
    {
        return mCube->GetLOD();
    }
    STDMETHOD_(DWORD, GetLevelCount)(THIS)
    {
        return mCube->GetLevelCount();
    }
    STDMETHOD(SetAutoGenFilterType)(THIS_ D3DTEXTUREFILTERTYPE FilterType)
    {
        return mCube->SetAutoGenFilterType( FilterType );
    }
    STDMETHOD_(D3DTEXTUREFILTERTYPE, GetAutoGenFilterType)(THIS)
    {
        return mCube->GetAutoGenFilterType();
    }
    STDMETHOD_(void, GenerateMipSubLevels)(THIS)
    {
        return mCube->GenerateMipSubLevels();
    }
    STDMETHOD(GetLevelDesc)(THIS_ UINT Level,D3DSURFACE_DESC *pDesc)
    {
        return mCube->GetLevelDesc( Level, pDesc );
    }
    STDMETHOD(AddDirtyRect)(THIS_ D3DCUBEMAP_FACES FaceType,CONST RECT* pDirtyRect)
    {
        return mCube->AddDirtyRect( FaceType, pDirtyRect );
    }
};

static IDirect3DTexture9Hooked      sTexhooked( NULL, NULL, NULL );
static IDirect3DCubeTexture9Hooked  sCubehooked( NULL, NULL, NULL );
static IDirect3DVertexBuffer9Hooked sVertexHooked( NULL, NULL, NULL );
static IDirect3DIndexBuffer9Hooked  sIndexHooked( NULL, NULL, NULL );
static IDirect3DSurface9Hooked      sSurfaceHooked( NULL, NULL, NULL );

IDirect3DSurface9* ConvertToUnhooked( IDirect3DSurface9* iBuf )
{
   if( !iBuf )
        return NULL;

    // First 4 (or 8 for 64bit) bytes are the virtual function pointer, if those don't match the hooked type then somehow the app
    // has gotten a hold of a non-hooked texture (probably D3DX stuff), so just pass it through.
    if( *(void**)iBuf == *(void**)&sSurfaceHooked )
    {
        IDirect3DSurface9Hooked* hook = (IDirect3DSurface9Hooked*)iBuf;
        return hook ? hook->mSurface : NULL;
    }

    return iBuf;
}

IDirect3DVertexBuffer9* ConvertToUnhooked( IDirect3DVertexBuffer9* iBuf )
{
    if( !iBuf )
        return NULL;

    // First 4 (or 8 for 64bit) bytes are the virtual function pointer, if those don't match the hooked type then somehow the app
    // has gotten a hold of a non-hooked texture. This should technically never happen but having this function lets you detect that it did.
    if( *(void**)iBuf == *(void**)&sVertexHooked )
    {
        IDirect3DVertexBuffer9Hooked* hook = (IDirect3DVertexBuffer9Hooked*)iBuf;
        return hook ? hook->mBuffer : NULL;
    }

    return iBuf;
}

IDirect3DIndexBuffer9* ConvertToUnhooked( IDirect3DIndexBuffer9* iBuf )
{
    if( !iBuf )
        return NULL;

    // First 4 (or 8 for 64bit) bytes are the virtual function pointer, if those don't match the hooked type then somehow the app
    // has gotten a hold of a non-hooked texture. This should technically never happen but having this function lets you detect that it did.
    if( *(void**)iBuf == *(void**)&sIndexHooked )
    {
        IDirect3DIndexBuffer9Hooked* hook = (IDirect3DIndexBuffer9Hooked*)iBuf;
        return hook ? hook->mBuffer : NULL;
    }

    return iBuf;
}

IDirect3DTexture9* ConvertToUnhooked( IDirect3DTexture9* iTex )
{
    if( !iTex )
        return NULL;

    // First 4 (or 8 for 64bit) bytes are the virtual function pointer, if those don't match the hooked type then somehow the app
    // has gotten a hold of a non-hooked texture. This should technically never happen but having this function lets you detect that it did.
    if( *(void**)iTex == *(void**)&sTexhooked )
    {
        IDirect3DTexture9Hooked* hook = (IDirect3DTexture9Hooked*)iTex;
        return hook ? hook->mTexture : NULL;
    }

    return iTex;
}

IDirect3DCubeTexture9* ConvertToUnhooked( IDirect3DCubeTexture9* iTex )
{
    if( !iTex )
        return NULL;

    static IDirect3DTexture9Hooked texhooked( NULL, NULL, NULL );
    static IDirect3DCubeTexture9Hooked cubehooked( NULL, NULL, NULL );

    // First 4 (or 8 for 64bit) bytes are the virtual function pointer, if those don't match the hooked type then somehow the app
    // has gotten a hold of a non-hooked texture. This should technically never happen but having this function lets you detect that it did.
    if( *(void**)iTex == *(void**)&sCubehooked )
    {
        IDirect3DCubeTexture9Hooked* hook = (IDirect3DCubeTexture9Hooked*)iTex;
        return hook ? hook->mCube : NULL;
    }

    return iTex;
}

IDirect3DBaseTexture9* ConvertToUnhooked( IDirect3DBaseTexture9* iTex )
{
    if( !iTex )
        return NULL;

    static IDirect3DTexture9Hooked texhooked( NULL, NULL, NULL );
    static IDirect3DCubeTexture9Hooked cubehooked( NULL, NULL, NULL );

    // First 4 (or 8 for 64bit) bytes are the virtual function pointer, if those don't match the hooked type then somehow the app
    // has gotten a hold of a non-hooked texture. This should technically never happen but having this function lets you detect that it did.
    if( *(void**)iTex == *(void**)&sTexhooked )
    {
        IDirect3DTexture9Hooked* hook = (IDirect3DTexture9Hooked*)iTex;
        return hook ? hook->mTexture : NULL;
    }
    else if( *(void**)iTex == *(void**)&sCubehooked )
    {
        IDirect3DCubeTexture9Hooked* hook = (IDirect3DCubeTexture9Hooked*)iTex;
        return hook ? hook->mCube : NULL;
    }

    return iTex;
}

// -----------------------------------------------------------------------------
// Hooking swap chain to get presents
// -----------------------------------------------------------------------------
interface IDirect3DSwapChain9Hooked : public IDirect3DSwapChain9
{
    IDirect3DDevice9*    mDevice;
    IDirect3DSwapChain9* mSwapChain;
    IDirect3DDevice9Hooked* mHookedDevice;

    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj)
    {
        HRESULT res = mSwapChain->QueryInterface( riid, ppvObj );
        Log( "%08x SwapChain->QueryInterface: res = %x\n", this, res );
        return res;
    }

    STDMETHOD_(ULONG,AddRef)(THIS)
    {
        ULONG res = mSwapChain->AddRef();
        Log( "%08x SwapChain->AddRef: res = %u\n", this, res );
        return res;
    }

    STDMETHOD_(ULONG,Release)(THIS)
    {
        ULONG res = mSwapChain->Release();
        if( res == 0 )
            delete this;

        Log( "%08x SwapChain->Release: res = %u\n", this, res );
        return res;
    }

    /*** IDirect3DSwapChain9 methods ***/
    STDMETHOD(Present)(THIS_ CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion,DWORD dwFlags)
    {
        HRESULT res = mSwapChain->Present( pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags );
        PresentInternal( mDevice, res );
        Log( "%08x SwapChain->Present: res = %x\n", this, res );
        return res;
    }

    STDMETHOD(GetFrontBufferData)(THIS_ IDirect3DSurface9* pDestSurface)
    {
        IDirect3DSurface9Hooked* hookdst = (IDirect3DSurface9Hooked*)pDestSurface;

        HRESULT res = mSwapChain->GetFrontBufferData( hookdst ? hookdst->mSurface : NULL );
        Log( "%08x SwapChain->GetFrontBufferData: res = %x\n", this, res );
        return res;
    }

    STDMETHOD(GetBackBuffer)(THIS_ UINT iBackBuffer,D3DBACKBUFFER_TYPE Type,IDirect3DSurface9** ppBackBuffer)
    {
        HRESULT res = mSwapChain->GetBackBuffer( iBackBuffer, Type, ppBackBuffer );
        if( SUCCEEDED( res ) && *ppBackBuffer )
        {
            *ppBackBuffer = new IDirect3DSurface9Hooked( *ppBackBuffer, mHookedDevice, mDevice );
        }

        Log( "%08x SwapChain->GetBackBuffer: res = %x\n", this, res );
        return res;
    }

    STDMETHOD(GetRasterStatus)(THIS_ D3DRASTER_STATUS* pRasterStatus)
    {
        HRESULT res = mSwapChain->GetRasterStatus( pRasterStatus );
        Log( "%08x SwapChain->GetRasterStatus: res = %x\n", this, res );
        return res;
    }

    STDMETHOD(GetDisplayMode)(THIS_ D3DDISPLAYMODE* pMode)
    {
        HRESULT res = mSwapChain->GetDisplayMode( pMode );
        Log( "%08x SwapChain->GetDisplayMode: res = %x\n", this, res );
        return res;
    }

    STDMETHOD(GetDevice)(THIS_ IDirect3DDevice9** ppDevice)
    {
        if( mHookedDevice )
        {
            *ppDevice = (IDirect3DDevice9*)mHookedDevice;
            mDevice->AddRef();
            Log( "%08x SwapChain->GetDevice: Overrode\n", this );
            return S_OK;
        }
        else
        {
            HRESULT res = mSwapChain->GetDevice( ppDevice );
            Log( "%08x SwapChain->GetDevice: No device. Returning base. res = %x\n", this, res );
            return res;
        }
    }

    STDMETHOD(GetPresentParameters)(THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters)
    {
        HRESULT res = mSwapChain->GetPresentParameters( pPresentationParameters );
        Log( "%08x SwapChain->GetPresentParameters: res = %x\n", this, res );
        return res;
    }
};

// -----------------------------------------------------------------------------
// Hooking Device to get draw call information
// -----------------------------------------------------------------------------
interface IDirect3DDevice9Hooked : public IDirect3DDevice9
{
    IDirect3DDevice9Hooked( IDirect3DDevice9* iDevice )
    {
        mDevice = iDevice;
        for( unsigned int ii = 0; ii < 16; ++ii ) 
        {
            mStreamSources[ii] = NULL;
        }
    }

    IDirect3DDevice9* mDevice;
    IDirect3DVertexBuffer9Hooked* mStreamSources[16];
    IDirect3DIndexBuffer9*  mIndexSource;

    //------------------------------------------------------------------------------------------------------------------
    // Overridden functions
    //------------------------------------------------------------------------------------------------------------------
    STDMETHOD(Clear)(THIS_ DWORD Count,CONST D3DRECT* pRects,DWORD Flags,D3DCOLOR Color,float Z,DWORD Stencil)
    {
        HRESULT res = DissectorDX9::Clear( mDevice, Count, pRects, Flags, Color, Z, Stencil );
        Log( "%08x Clear: res = %x Count = %u, pRects = %x Flags = %x, Color = %x, Z = %f; Stencil = %u\n", this, res, Count, pRects, Flags, Color, Z, Stencil );
        return res;
    }

    STDMETHOD(DrawPrimitive)(THIS_ D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount)
    {
        HRESULT res = DissectorDX9::DrawPrimitive(mDevice,PrimitiveType,StartVertex,PrimitiveCount);
        Log( "%08x DrawPrimitive: res = %x\n", this, res );
        return res;
    }

    STDMETHOD(DrawIndexedPrimitive)(THIS_ D3DPRIMITIVETYPE type,INT BaseVertexIndex,UINT MinVertexIndex,UINT NumVertices,
        UINT startIndex,UINT primCount)
    {
        HRESULT res =  DissectorDX9::DrawIndexedPrimitive(mDevice,type,BaseVertexIndex,MinVertexIndex,NumVertices,startIndex,primCount);
        Log( "%08x DrawIndexedPrimitive: res = %x\n", this, res );
        return res;

    }

    STDMETHOD(DrawPrimitiveUP)(THIS_ D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,CONST void* pVertexStreamZeroData,
        UINT VertexStreamZeroStride)
    {
        HRESULT res =  DissectorDX9::DrawPrimitiveUP(mDevice,PrimitiveType, PrimitiveCount,pVertexStreamZeroData,VertexStreamZeroStride);
        Log( "%08x DrawPrimitiveUP: res = %x\n", this, res );
        return res;

    }

    STDMETHOD(DrawIndexedPrimitiveUP)(THIS_ D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,UINT NumVertices,
        UINT PrimitiveCount,CONST void* pIndexData,D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride)
    {
        HRESULT res = DissectorDX9::DrawIndexedPrimitiveUP(mDevice,PrimitiveType,MinVertexIndex,NumVertices,PrimitiveCount,pIndexData,
            IndexDataFormat,pVertexStreamZeroData,VertexStreamZeroStride);
        Log( "%08x DrawIndexedPrimitiveUP: res = %x\n", this, res );
        return res;

    }

    STDMETHOD(Present)(THIS_ CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion)
    {
        HRESULT res = mDevice->Present( pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion );
        PresentInternal( mDevice, res );

        Log( "%08x Present: res = %x\n", this, res );
        return res;
    }

    STDMETHOD_(ULONG,AddRef)(THIS)
    {
        ULONG res = mDevice->AddRef() - gDeviceBaseRefCount;
        Log( "%08x AddRef: res = %u\n", this, res );
        return res;

    }

    STDMETHOD_(ULONG,Release)(THIS)
    {
        ULONG val = mDevice->Release();
        if( val <= gDeviceBaseRefCount )
        {
            DissectorDX9::Shutdown();
            return 0;
        }
        val -= gDeviceBaseRefCount;
        Log( "%08x Release: %u\n", val );
        return val;
    }

    STDMETHOD(CreateVertexBuffer)(THIS_ UINT Length,DWORD Usage,DWORD FVF,D3DPOOL Pool,IDirect3DVertexBuffer9** ppVertexBuffer,HANDLE* pSharedHandle)
    {
        
        HRESULT res = mDevice->CreateVertexBuffer( Length, Usage, FVF, Pool, ppVertexBuffer, pSharedHandle );
        if( SUCCEEDED( res ) )
        {
            IDirect3DVertexBuffer9Hooked* hooked = new IDirect3DVertexBuffer9Hooked( *ppVertexBuffer, this, mDevice );
            if( hooked )
            {
                *ppVertexBuffer = hooked;
            }
        }

        Log( "%08x CreateVertexBuffer: res = %x\n", this, res );
        return res;
    }

    STDMETHOD(CreateIndexBuffer)(THIS_ UINT Length,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DIndexBuffer9** ppIndexBuffer,HANDLE* pSharedHandle)
    {
        HRESULT res = mDevice->CreateIndexBuffer( Length, Usage, Format, Pool, ppIndexBuffer, pSharedHandle );
        if( SUCCEEDED( res ) )
        {
            IDirect3DIndexBuffer9Hooked* hooked = new IDirect3DIndexBuffer9Hooked( *ppIndexBuffer, this, mDevice );
            if( hooked )
            {
                *ppIndexBuffer = hooked;
            }
        }

        Log( "%08x CreateIndexBuffer: res = %x\n", this, res );
        return res;

    }

    STDMETHOD(SetStreamSource)(THIS_ UINT StreamNumber,IDirect3DVertexBuffer9* pStreamData,UINT OffsetInBytes,UINT Stride)
    {
        IDirect3DVertexBuffer9Hooked* hooked = (IDirect3DVertexBuffer9Hooked*)pStreamData;
        mStreamSources[StreamNumber] = hooked;
        HRESULT res = mDevice->SetStreamSource(StreamNumber,ConvertToUnhooked( pStreamData ),OffsetInBytes,Stride);
        Log( "%08x SetStreamSource: res = %x\n", this, res );
        return res;
    }

    STDMETHOD(GetStreamSource)(THIS_ UINT StreamNumber,IDirect3DVertexBuffer9** ppStreamData,UINT* pOffsetInBytes,UINT* pStride)
    {
        HRESULT res = mDevice->GetStreamSource(StreamNumber,ppStreamData,pOffsetInBytes,pStride);
        if( SUCCEEDED( res ) )
        {
            if( mStreamSources[StreamNumber] && mStreamSources[StreamNumber]->mBuffer == *ppStreamData )
            {
                *ppStreamData = mStreamSources[StreamNumber];
            }
        }
        Log( "%08x GetStreamSource: res = %x\n", this, res );
        return res;
    }

    STDMETHOD(SetIndices)(THIS_ IDirect3DIndexBuffer9* pIndexData)
    {
        HRESULT res = mDevice->SetIndices( ConvertToUnhooked( pIndexData ) );
        if( SUCCEEDED( res ) )
            mIndexSource = pIndexData;

        Log( "%08x SetIndices: res = %x\n", this, res );
        return res;

    }

    STDMETHOD(GetIndices)(THIS_ IDirect3DIndexBuffer9** ppIndexData)
    {
        HRESULT res = mDevice->GetIndices(ppIndexData);
        if( SUCCEEDED( res ) )
            *ppIndexData = mIndexSource;

        Log( "%08x GetIndices: res = %x\n", this, res );
        return res;
    }

    STDMETHOD(SetCursorProperties)(THIS_ UINT XHotSpot,UINT YHotSpot,IDirect3DSurface9* pCursorBitmap)
    {
        IDirect3DSurface9Hooked* hook = (IDirect3DSurface9Hooked*)pCursorBitmap;
        HRESULT res = mDevice->SetCursorProperties( XHotSpot, YHotSpot, hook ? hook->mSurface : NULL );
        Log( "%08x SetCursorProperties: res = %x\n", this, res );
        return res;

    }

    STDMETHOD(CreateAdditionalSwapChain)(THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DSwapChain9** pSwapChain)
    {
        HRESULT res = mDevice->CreateAdditionalSwapChain( pPresentationParameters, pSwapChain );
        IDirect3DSwapChain9Hooked* hooked = new IDirect3DSwapChain9Hooked;
        hooked->mDevice = mDevice;
        hooked->mSwapChain = *pSwapChain;
        hooked->mHookedDevice = this;
        *pSwapChain = hooked;
        Log( "%08x CreateAdditionalSwapChain: res = %x\n", this, res );
        return res;
    }


    STDMETHOD(GetSwapChain)(THIS_ UINT iSwapChain,IDirect3DSwapChain9** pSwapChain)
    {
        HRESULT res = mDevice->GetSwapChain( iSwapChain, pSwapChain );
        IDirect3DSwapChain9Hooked* hooked = new IDirect3DSwapChain9Hooked;
        hooked->mDevice = mDevice;
        hooked->mSwapChain = *pSwapChain;
        hooked->mHookedDevice = this;
        *pSwapChain = hooked;
        Log( "%08x GetSwapChain: res = %x\n", this, res );
        return res;
    }    

    STDMETHOD(GetBackBuffer)(THIS_ UINT iSwapChain,UINT iBackBuffer,D3DBACKBUFFER_TYPE Type,IDirect3DSurface9** ppBackBuffer)
    {
        HRESULT res = mDevice->GetBackBuffer( iSwapChain, iBackBuffer, Type, ppBackBuffer );
        if( SUCCEEDED( res ) && *ppBackBuffer )
        {
            *ppBackBuffer = new IDirect3DSurface9Hooked( *ppBackBuffer, this, mDevice );
        }
        Log( "%08x GetBackBuffer: res = %x\n", this, res );
        return res;

    }

    STDMETHOD(CreateTexture)(THIS_ UINT Width,UINT Height,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DTexture9** ppTexture,HANDLE* pSharedHandle)
    {
        HRESULT res = mDevice->CreateTexture( Width, Height, Levels, Usage, Format, Pool, ppTexture, pSharedHandle);
        if( SUCCEEDED( res ) && ppTexture )
        {
            *ppTexture = new IDirect3DTexture9Hooked( *ppTexture, this, mDevice );
        }
        Log( "%08x CreateTexture: ret = %x res = %x\n", this, *ppTexture, res );
        return res;

    }
    STDMETHOD(CreateCubeTexture)(THIS_ UINT EdgeLength,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DCubeTexture9** ppCubeTexture,HANDLE* pSharedHandle)
    {
        HRESULT res = mDevice->CreateCubeTexture( EdgeLength, Levels, Usage, Format, Pool, ppCubeTexture, pSharedHandle );
        if( SUCCEEDED( res ) && *ppCubeTexture )
        {
            *ppCubeTexture = new IDirect3DCubeTexture9Hooked( *ppCubeTexture, this, mDevice );
        }
        Log( "%08x CreateCubeTexture: ret = %x res = %x\n", this, *ppCubeTexture, res );
        return res;

    }

    STDMETHOD(CreateRenderTarget)(THIS_ UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Lockable,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle)
    {
        HRESULT res = mDevice->CreateRenderTarget( Width, Height, Format, MultiSample, MultisampleQuality, Lockable, ppSurface, pSharedHandle );
        if( SUCCEEDED( res ) && *ppSurface )
        {
            *ppSurface = new IDirect3DSurface9Hooked( *ppSurface, this, mDevice );
        }

        Log( "%08x CreateRenderTarget: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(CreateDepthStencilSurface)(THIS_ UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Discard,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle)
    {
        HRESULT res = mDevice->CreateDepthStencilSurface( Width, Height, Format, MultiSample, MultisampleQuality, Discard, ppSurface, pSharedHandle );
        if( SUCCEEDED( res ) && *ppSurface )
        {
            *ppSurface = new IDirect3DSurface9Hooked( *ppSurface, this, mDevice );
        }
        Log( "%08x CreateDepthStencilSurface: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(UpdateSurface)(THIS_ IDirect3DSurface9* pSourceSurface,CONST RECT* pSourceRect,IDirect3DSurface9* pDestinationSurface,CONST POINT* pDestPoint)
    {
        IDirect3DSurface9Hooked* hooksrc = (IDirect3DSurface9Hooked*)pSourceSurface;
        IDirect3DSurface9Hooked* hookdst = (IDirect3DSurface9Hooked*)pDestinationSurface;

        HRESULT res = mDevice->UpdateSurface( hooksrc ? hooksrc->mSurface : NULL, pSourceRect, hookdst ? hookdst->mSurface : NULL, pDestPoint );
        Log( "%08x UpdateSurface: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(UpdateTexture)(THIS_ IDirect3DBaseTexture9* pSourceTexture,IDirect3DBaseTexture9* pDestinationTexture)
    {
        IDirect3DTexture9Hooked* hooksrc = (IDirect3DTexture9Hooked*)pSourceTexture;
        IDirect3DTexture9Hooked* hookdst = (IDirect3DTexture9Hooked*)pDestinationTexture;

        HRESULT res = mDevice->UpdateTexture( ConvertToUnhooked( pSourceTexture ), ConvertToUnhooked( pDestinationTexture ) );
        Log( "%08x UpdateTexture: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(GetRenderTargetData)(THIS_ IDirect3DSurface9* pRenderTarget,IDirect3DSurface9* pDestSurface)
    {
        IDirect3DSurface9Hooked* hookrt  = (IDirect3DSurface9Hooked*)pRenderTarget;
        IDirect3DSurface9Hooked* hookdst = (IDirect3DSurface9Hooked*)pDestSurface;
        HRESULT res = mDevice->GetRenderTargetData( hookrt ? hookrt->mSurface : NULL, hookdst ? hookdst->mSurface : NULL );
        Log( "%08x GetRenderTargetData: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(GetFrontBufferData)(THIS_ UINT iSwapChain,IDirect3DSurface9* pDestSurface)
    {
        IDirect3DSurface9Hooked* hookdst = (IDirect3DSurface9Hooked*)pDestSurface;
        HRESULT res = mDevice->GetFrontBufferData( iSwapChain, hookdst ? hookdst->mSurface : NULL );
        Log( "%08x GetFrontBufferData: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(StretchRect)(THIS_ IDirect3DSurface9* pSourceSurface,CONST RECT* pSourceRect,IDirect3DSurface9* pDestSurface,CONST RECT* pDestRect,D3DTEXTUREFILTERTYPE Filter)
    {
        IDirect3DSurface9Hooked* hooksrc = (IDirect3DSurface9Hooked*)pSourceSurface;
        IDirect3DSurface9Hooked* hookdst = (IDirect3DSurface9Hooked*)pDestSurface;

        HRESULT res = mDevice->StretchRect( hooksrc ? hooksrc->mSurface : NULL, pSourceRect, hookdst ? hookdst->mSurface : NULL, pDestRect, Filter );
        Log( "%08x StretchRect: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(ColorFill)(THIS_ IDirect3DSurface9* pSurface,CONST RECT* pRect,D3DCOLOR color)
    {
        IDirect3DSurface9Hooked* hook = (IDirect3DSurface9Hooked*)pSurface;

        HRESULT res = mDevice->ColorFill( hook ? hook->mSurface : NULL, pRect, color );
        Log( "%08x ColorFill: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(CreateOffscreenPlainSurface)(THIS_ UINT Width,UINT Height,D3DFORMAT Format,D3DPOOL Pool,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle)
    {
        HRESULT res = mDevice->CreateOffscreenPlainSurface( Width, Height, Format, Pool, ppSurface, pSharedHandle );
        if( SUCCEEDED( res ) && *ppSurface )
        {
            *ppSurface = new IDirect3DSurface9Hooked( *ppSurface, this, mDevice );
        }

        Log( "%08x CreateOffscreenPlainSurface: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(SetRenderTarget)(THIS_ DWORD RenderTargetIndex,IDirect3DSurface9* pRenderTarget)
    {
        IDirect3DSurface9Hooked* hook = (IDirect3DSurface9Hooked*)pRenderTarget;

        HRESULT res = mDevice->SetRenderTarget( RenderTargetIndex, ConvertToUnhooked( hook ) );
        Log( "%08x SetRenderTarget: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(GetRenderTarget)(THIS_ DWORD RenderTargetIndex,IDirect3DSurface9** ppRenderTarget)
    {
        HRESULT res = mDevice->GetRenderTarget( RenderTargetIndex, ppRenderTarget );
        if( SUCCEEDED( res ) && *ppRenderTarget )
        {
            *ppRenderTarget = new IDirect3DSurface9Hooked( *ppRenderTarget, this, mDevice );
        }
        Log( "%08x GetRenderTarget: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(SetDepthStencilSurface)(THIS_ IDirect3DSurface9* pNewZStencil)
    {
        IDirect3DSurface9Hooked* hook = (IDirect3DSurface9Hooked*)pNewZStencil;
        HRESULT res = mDevice->SetDepthStencilSurface( ConvertToUnhooked( hook ) );
        Log( "%08x SetDepthStencilSurface: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(GetDepthStencilSurface)(THIS_ IDirect3DSurface9** ppZStencilSurface)
    {
        HRESULT res = mDevice->GetDepthStencilSurface( ppZStencilSurface );
        if( SUCCEEDED( res ) && *ppZStencilSurface )
        {
            *ppZStencilSurface = new IDirect3DSurface9Hooked( *ppZStencilSurface, this, mDevice );
        }

        Log( "%08x GetDepthStencilSurface: res = %x\n", this, res );
        return res;

    }

    STDMETHOD(SetTexture)(THIS_ DWORD Stage,IDirect3DBaseTexture9* pTexture)
    {
        HRESULT res = mDevice->SetTexture(Stage, ConvertToUnhooked( pTexture ) );
        Log( "%08x SetTexture: tex = %x res = %x\n", this, pTexture, res );
        return res;

    }

    STDMETHOD(GetTexture)(THIS_ DWORD Stage,IDirect3DBaseTexture9** ppTexture)
    {
        HRESULT res = mDevice->GetTexture(Stage,ppTexture);
        Log( "%08x GetTexture: res = %x\n", this, res );
        return res;
    }

    //------------------------------------------------------------------------------------------------------------------
    // Boiler-plate pass through
    //------------------------------------------------------------------------------------------------------------------
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj)
    {
        HRESULT res = mDevice->QueryInterface( riid, ppvObj );
        Log( "%08x QueryInterface: res = %x\n", this, res );
        return res;

    }

    /*** IDirect3DDevice9 methods ***/
    STDMETHOD(TestCooperativeLevel)(THIS)
    {
        HRESULT res = mDevice->TestCooperativeLevel();
        Log( "%08x TestCooperativeLevel: res = %x\n", this, res );
        return res;

    }
    STDMETHOD_(UINT, GetAvailableTextureMem)(THIS)
    {
        HRESULT res = mDevice->GetAvailableTextureMem();
        Log( "%08x GetAvailableTextureMem: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(EvictManagedResources)(THIS)
    {
        HRESULT res = mDevice->EvictManagedResources();
        Log( "%08x EvictManagedResources: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(GetDirect3D)(THIS_ IDirect3D9** ppD3D9)
    {
        if( sD3D )
        {
            Log( "%08x GetDirect3D: Overrode\n" );
            *ppD3D9 = sD3D;
            sD3D->AddRef();
            return S_OK;
        }
        else
        {
            HRESULT res = mDevice->GetDirect3D( ppD3D9 );
            Log( "%08x GetDirect3D: res = %x\n", this, res );
            return res;
        }

    }
    STDMETHOD(GetDeviceCaps)(THIS_ D3DCAPS9* pCaps)
    {
        HRESULT res = mDevice->GetDeviceCaps( pCaps );
        Log( "%08x GetDeviceCaps: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(GetDisplayMode)(THIS_ UINT iSwapChain,D3DDISPLAYMODE* pMode)
    {
        HRESULT res = mDevice->GetDisplayMode( iSwapChain, pMode );
        Log( "%08x GetDisplayMode: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(GetCreationParameters)(THIS_ D3DDEVICE_CREATION_PARAMETERS *pParameters)
    {
        HRESULT res = mDevice->GetCreationParameters( pParameters );
        Log( "%08x GetCreationParameters: res = %x\n", this, res );
        return res;

    }
    STDMETHOD_(void, SetCursorPosition)(THIS_ int X,int Y,DWORD Flags)
    {
        mDevice->SetCursorPosition( X, Y, Flags );
        Log( "%08x SetCursorPosition\n" );

    }
    STDMETHOD_(BOOL, ShowCursor)(THIS_ BOOL bShow)
    {
        BOOL res = mDevice->ShowCursor( bShow );
        Log( "%08x ShowCursor: res = %u\n", this, res );
        return res;

    }

    STDMETHOD_(UINT, GetNumberOfSwapChains)(THIS)
    {
        HRESULT res = mDevice->GetNumberOfSwapChains();
        Log( "%08x GetNumberOfSwapChains: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(Reset)(THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters)
    {
        HRESULT res = mDevice->Reset( pPresentationParameters );
        Log( "%08x Reset: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(GetRasterStatus)(THIS_ UINT iSwapChain,D3DRASTER_STATUS* pRasterStatus)
    {
        HRESULT res = mDevice->GetRasterStatus( iSwapChain, pRasterStatus );
        Log( "%08x GetRasterStatus: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(SetDialogBoxMode)(THIS_ BOOL bEnableDialogs)
    {
        HRESULT res = mDevice->SetDialogBoxMode( bEnableDialogs );
        Log( "%08x SetDialogBoxMode: res = %x\n", this, res );
        return res;

    }
    STDMETHOD_(void, SetGammaRamp)(THIS_ UINT iSwapChain,DWORD Flags,CONST D3DGAMMARAMP* pRamp)
    {
        mDevice->SetGammaRamp( iSwapChain, Flags, pRamp );
        Log( "%08x SetGammaRamp: \n" );
    }
    STDMETHOD_(void, GetGammaRamp)(THIS_ UINT iSwapChain,D3DGAMMARAMP* pRamp)
    {
        mDevice->GetGammaRamp( iSwapChain, pRamp );
        Log( "%08x GetGammaRamp: \n" );
    }
    STDMETHOD(CreateVolumeTexture)(THIS_ UINT Width,UINT Height,UINT Depth,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DVolumeTexture9** ppVolumeTexture,HANDLE* pSharedHandle)
    {
        HRESULT res = mDevice->CreateVolumeTexture( Width, Height, Depth, Levels, Usage, Format, Pool, ppVolumeTexture, pSharedHandle );
        Log( "%08x CreateVolumeTexture: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(BeginScene)(THIS)
    {
        HRESULT res = mDevice->BeginScene();
        Log( "%08x BeginScene: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(EndScene)(THIS)
    {
        HRESULT res = mDevice->EndScene();
        Log( "%08x EndScene: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(SetTransform)(THIS_ D3DTRANSFORMSTATETYPE State,CONST D3DMATRIX* pMatrix)
    {
        HRESULT res = mDevice->SetTransform( State, pMatrix );
        Log( "%08x SetTransform: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(GetTransform)(THIS_ D3DTRANSFORMSTATETYPE State,D3DMATRIX* pMatrix)
    {
        HRESULT res = mDevice->GetTransform( State, pMatrix );
        Log( "%08x GetTransform: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(MultiplyTransform)(THIS_ D3DTRANSFORMSTATETYPE State,CONST D3DMATRIX* pMatrix )
    {
        HRESULT res = mDevice->MultiplyTransform( State, pMatrix );
        Log( "%08x MultiplyTransform: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(SetViewport)(THIS_ CONST D3DVIEWPORT9* pViewport)
    {
        HRESULT res = mDevice->SetViewport( pViewport );
        Log( "%08x SetViewport: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(GetViewport)(THIS_ D3DVIEWPORT9* pViewport)
    {
        HRESULT res = mDevice->GetViewport( pViewport );
        Log( "%08x GetViewport: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(SetMaterial)(THIS_ CONST D3DMATERIAL9* pMaterial)
    {
        HRESULT res = mDevice->SetMaterial( pMaterial );
        Log( "%08x SetMaterial: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(GetMaterial)(THIS_ D3DMATERIAL9* pMaterial)
    {
        HRESULT res = mDevice->GetMaterial( pMaterial );
        Log( "%08x GetMaterial: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(SetLight)(THIS_ DWORD Index,CONST D3DLIGHT9* iLight)
    {
        HRESULT res = mDevice->SetLight( Index, iLight );
        Log( "%08x SetLight: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(GetLight)(THIS_ DWORD Index,D3DLIGHT9* iLight )
    {
        HRESULT res = mDevice->GetLight( Index, iLight );
        Log( "%08x GetLight: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(LightEnable)(THIS_ DWORD Index,BOOL Enable)
    {
        HRESULT res = mDevice->LightEnable( Index, Enable );
        Log( "%08x LightEnable: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(GetLightEnable)(THIS_ DWORD Index,BOOL* pEnable)
    {
        HRESULT res = mDevice->GetLightEnable( Index, pEnable );
        Log( "%08x GetLightEnable: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(SetClipPlane)(THIS_ DWORD Index,CONST float* pPlane)
    {
        HRESULT res = mDevice->SetClipPlane( Index, pPlane );
        Log( "%08x SetClipPlane: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(GetClipPlane)(THIS_ DWORD Index,float* pPlane)
    {
        HRESULT res = mDevice->GetClipPlane( Index, pPlane );
        Log( "%08x GetClipPlane: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(SetRenderState)(THIS_ D3DRENDERSTATETYPE State,DWORD Value)
    {
        HRESULT res = mDevice->SetRenderState( State, Value );
        Log( "%08x SetRenderState: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(GetRenderState)(THIS_ D3DRENDERSTATETYPE State,DWORD* pValue)
    {
        HRESULT res = mDevice->GetRenderState( State, pValue );
        Log( "%08x GetRenderState: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(CreateStateBlock)(THIS_ D3DSTATEBLOCKTYPE Type,IDirect3DStateBlock9** ppSB)
    {
        HRESULT res = mDevice->CreateStateBlock( Type, ppSB );
        Log( "%08x CreateStateBlock: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(BeginStateBlock)(THIS)
    {
        HRESULT res = mDevice->BeginStateBlock();
        Log( "%08x BeginStateBlock: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(EndStateBlock)(THIS_ IDirect3DStateBlock9** ppSB)
    {
        HRESULT res = mDevice->EndStateBlock(ppSB);
        Log( "%08x EndStateBlock: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(SetClipStatus)(THIS_ CONST D3DCLIPSTATUS9* pClipStatus)
    {
        HRESULT res = mDevice->SetClipStatus(pClipStatus);
        Log( "%08x SetClipStatus: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(GetClipStatus)(THIS_ D3DCLIPSTATUS9* pClipStatus)
    {
        HRESULT res = mDevice->GetClipStatus(pClipStatus);
        Log( "%08x GetClipStatus: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(GetTextureStageState)(THIS_ DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD* pValue)
    {
        HRESULT res = mDevice->GetTextureStageState(Stage,Type,pValue);
        Log( "%08x GetTextureStageState: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(SetTextureStageState)(THIS_ DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD Value)
    {
        HRESULT res = mDevice->SetTextureStageState(Stage,Type,Value);
        Log( "%08x SetTextureStageState: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(GetSamplerState)(THIS_ DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD* pValue)
    {
        HRESULT res = mDevice->GetSamplerState(Sampler,Type,pValue);
        Log( "%08x GetSamplerState: res = %x\n", this, res );
        return res;

    }

    STDMETHOD(SetSamplerState)(THIS_ DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD Value)
    {
        if( Sampler == 0 && Type == D3DSAMP_ADDRESSU )
            last0samp = Value;

        HRESULT res = mDevice->SetSamplerState(Sampler,Type,Value);
        Log( "%08x SetSamplerState: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(ValidateDevice)(THIS_ DWORD* pNumPasses)
    {
        HRESULT res = mDevice->ValidateDevice(pNumPasses);
        Log( "%08x ValidateDevice: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(SetPaletteEntries)(THIS_ UINT PaletteNumber,CONST PALETTEENTRY* pEntries)
    {
        HRESULT res = mDevice->SetPaletteEntries(PaletteNumber,pEntries);
        Log( "%08x SetPaletteEntries: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(GetPaletteEntries)(THIS_ UINT PaletteNumber,PALETTEENTRY* pEntries)
    {
        HRESULT res = mDevice->GetPaletteEntries(PaletteNumber,pEntries);
        Log( "%08x GetPaletteEntries: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(SetCurrentTexturePalette)(THIS_ UINT PaletteNumber)
    {
        HRESULT res = mDevice->SetCurrentTexturePalette(PaletteNumber);
        Log( "%08x SetCurrentTexturePalette: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(GetCurrentTexturePalette)(THIS_ UINT *PaletteNumber)
    {
        HRESULT res = mDevice->GetCurrentTexturePalette(PaletteNumber);
        Log( "%08x GetCurrentTexturePalette: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(SetScissorRect)(THIS_ CONST RECT* pRect)
    {
        HRESULT res = mDevice->SetScissorRect(pRect);
        Log( "%08x SetScissorRect: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(GetScissorRect)(THIS_ RECT* pRect)
    {
        HRESULT res = mDevice->GetScissorRect(pRect);
        Log( "%08x GetScissorRect: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(SetSoftwareVertexProcessing)(THIS_ BOOL bSoftware)
    {
        HRESULT res = mDevice->SetSoftwareVertexProcessing(bSoftware);
        Log( "%08x SetSoftwareVertexProcessing: res = %x\n", this, res );
        return res;

    }
    STDMETHOD_(BOOL, GetSoftwareVertexProcessing)(THIS)
    {
        HRESULT res = mDevice->GetSoftwareVertexProcessing();
        Log( "%08x GetSoftwareVertexProcessing: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(SetNPatchMode)(THIS_ float nSegments)
    {
        HRESULT res = mDevice->SetNPatchMode(nSegments);
        Log( "%08x SetNPatchMode: res = %x\n", this, res );
        return res;

    }
    STDMETHOD_(float, GetNPatchMode)(THIS)
    {
        float res = mDevice->GetNPatchMode();
        Log( "%08x GetNPatchMode: res = %f\n", this, res );
        return res;

    }
    STDMETHOD(ProcessVertices)(THIS_ UINT SrcStartIndex,UINT DestIndex,UINT VertexCount,IDirect3DVertexBuffer9* pDestBuffer,IDirect3DVertexDeclaration9* pVertexDecl,DWORD Flags)
    {
        HRESULT res = mDevice->ProcessVertices(SrcStartIndex,DestIndex,VertexCount,pDestBuffer,pVertexDecl,Flags);
        Log( "%08x ProcessVertices: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(CreateVertexDeclaration)(THIS_ CONST D3DVERTEXELEMENT9* pVertexElements,IDirect3DVertexDeclaration9** ppDecl)
    {
        HRESULT res = mDevice->CreateVertexDeclaration(pVertexElements,ppDecl);
        Log( "%08x CreateVertexDeclaration: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(SetVertexDeclaration)(THIS_ IDirect3DVertexDeclaration9* pDecl)
    {
        HRESULT res = mDevice->SetVertexDeclaration(pDecl);
        Log( "%08x SetVertexDeclaration: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(GetVertexDeclaration)(THIS_ IDirect3DVertexDeclaration9** ppDecl)
    {
        HRESULT res = mDevice->GetVertexDeclaration(ppDecl);
        Log( "%08x GetVertexDeclaration: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(SetFVF)(THIS_ DWORD FVF)
    {
        HRESULT res = mDevice->SetFVF(FVF);
        Log( "%08x SetFVF: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(GetFVF)(THIS_ DWORD* pFVF)
    {
        HRESULT res = mDevice->GetFVF(pFVF);
        Log( "%08x GetFVF: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(CreateVertexShader)(THIS_ CONST DWORD* pFunction,IDirect3DVertexShader9** ppShader)
    {
        HRESULT res = mDevice->CreateVertexShader(pFunction,ppShader);
        Log( "%08x CreateVertexShader: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(SetVertexShader)(THIS_ IDirect3DVertexShader9* pShader)
    {
        HRESULT res = mDevice->SetVertexShader(pShader);
        Log( "%08x SetVertexShader: pShader = %x res = %x\n", this, pShader, res );
        return res;
    }
    STDMETHOD(GetVertexShader)(THIS_ IDirect3DVertexShader9** ppShader)
    {
        HRESULT res = mDevice->GetVertexShader(ppShader);
        Log( "%08x GetVertexShader: res = %x\n", this, res );
        return res;
    }
    STDMETHOD(SetVertexShaderConstantF)(THIS_ UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount)
    {
        HRESULT res = mDevice->SetVertexShaderConstantF(StartRegister,pConstantData,Vector4fCount);
        Log( "%08x SetVertexShaderConstantF: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(GetVertexShaderConstantF)(THIS_ UINT StartRegister,float* pConstantData,UINT Vector4fCount)
    {
        HRESULT res = mDevice->GetVertexShaderConstantF(StartRegister,pConstantData,Vector4fCount);
        Log( "%08x GetVertexShaderConstantF: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(SetVertexShaderConstantI)(THIS_ UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount)
    {
        HRESULT res = mDevice->SetVertexShaderConstantI(StartRegister,pConstantData,Vector4iCount);
        Log( "%08x SetVertexShaderConstantI: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(GetVertexShaderConstantI)(THIS_ UINT StartRegister,int* pConstantData,UINT Vector4iCount)
    {
        HRESULT res = mDevice->GetVertexShaderConstantI(StartRegister,pConstantData,Vector4iCount);
        Log( "%08x GetVertexShaderConstantI: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(SetVertexShaderConstantB)(THIS_ UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount)
    {
        HRESULT res = mDevice->SetVertexShaderConstantB(StartRegister,pConstantData,BoolCount);
        Log( "%08x SetVertexShaderConstantB: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(GetVertexShaderConstantB)(THIS_ UINT StartRegister,BOOL* pConstantData,UINT BoolCount)
    {
        HRESULT res = mDevice->GetVertexShaderConstantB(StartRegister,pConstantData,BoolCount);
        Log( "%08x GetVertexShaderConstantB: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(SetStreamSourceFreq)(THIS_ UINT StreamNumber,UINT Setting)
    {
        HRESULT res = mDevice->SetStreamSourceFreq(StreamNumber,Setting);
        Log( "%08x SetStreamSourceFreq: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(GetStreamSourceFreq)(THIS_ UINT StreamNumber,UINT* pSetting)
    {
        HRESULT res = mDevice->GetStreamSourceFreq(StreamNumber,pSetting);
        Log( "%08x GetStreamSourceFreq: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(CreatePixelShader)(THIS_ CONST DWORD* pFunction,IDirect3DPixelShader9** ppShader)
    {
        HRESULT res = mDevice->CreatePixelShader(pFunction,ppShader);
        Log( "%08x CreatePixelShader: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(SetPixelShader)(THIS_ IDirect3DPixelShader9* pShader)
    {
        HRESULT res = mDevice->SetPixelShader(pShader);
        Log( "%08x SetPixelShader: pShader = %x, res = %x\n", this, pShader, res );
        return res;

    }
    STDMETHOD(GetPixelShader)(THIS_ IDirect3DPixelShader9** ppShader)
    {
        HRESULT res = mDevice->GetPixelShader(ppShader);
        Log( "%08x GetPixelShader: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(SetPixelShaderConstantF)(THIS_ UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount)
    {
        HRESULT res = mDevice->SetPixelShaderConstantF(StartRegister,pConstantData,Vector4fCount);
        Log( "%08x SetPixelShaderConstantF: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(GetPixelShaderConstantF)(THIS_ UINT StartRegister,float* pConstantData,UINT Vector4fCount)
    {
        HRESULT res = mDevice->GetPixelShaderConstantF(StartRegister,pConstantData,Vector4fCount);
        Log( "%08x GetPixelShaderConstantF: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(SetPixelShaderConstantI)(THIS_ UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount)
    {
        HRESULT res = mDevice->SetPixelShaderConstantI(StartRegister,pConstantData,Vector4iCount);
        Log( "%08x SetPixelShaderConstantI: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(GetPixelShaderConstantI)(THIS_ UINT StartRegister,int* pConstantData,UINT Vector4iCount)
    {
        HRESULT res = mDevice->GetPixelShaderConstantI(StartRegister,pConstantData,Vector4iCount);
        Log( "%08x GetPixelShaderConstantI: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(SetPixelShaderConstantB)(THIS_ UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount)
    {
        HRESULT res = mDevice->SetPixelShaderConstantB(StartRegister,pConstantData,BoolCount);
        Log( "%08x SetPixelShaderConstantB: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(GetPixelShaderConstantB)(THIS_ UINT StartRegister,BOOL* pConstantData,UINT BoolCount)
    {
        HRESULT res = mDevice->GetPixelShaderConstantB(StartRegister,pConstantData,BoolCount);
        Log( "%08x GetPixelShaderConstantB: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(DrawRectPatch)(THIS_ UINT Handle,CONST float* pNumSegs,CONST D3DRECTPATCH_INFO* pRectPatchInfo)
    {
        HRESULT res = mDevice->DrawRectPatch(Handle,pNumSegs,pRectPatchInfo);
        Log( "%08x DrawRectPatch: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(DrawTriPatch)(THIS_ UINT Handle,CONST float* pNumSegs,CONST D3DTRIPATCH_INFO* pTriPatchInfo)
    {
        HRESULT res = mDevice->DrawTriPatch(Handle,pNumSegs,pTriPatchInfo);
        Log( "%08x DrawTriPatch: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(DeletePatch)(THIS_ UINT Handle)
    {
        HRESULT res = mDevice->DeletePatch(Handle);
        Log( "%08x DeletePatch: res = %x\n", this, res );
        return res;

    }
    STDMETHOD(CreateQuery)(THIS_ D3DQUERYTYPE Type,IDirect3DQuery9** ppQuery)    
    {
        HRESULT res = mDevice->CreateQuery(Type,ppQuery);
        Log( "%08x CreateQuery: res = %x\n", this, res );
        return res;

    }
};

// Need to hook the D3D interface so we can intercept the create device call.
interface IDirect3D9Hooked : public IDirect3D9 {
public:
	IDirect3D9Hooked(IDirect3D9 *ppIDirect3D9) {
		mD3D = ppIDirect3D9;
	}
	
	IDirect3D9 *mD3D;

    //------------------------------------------------------------------------------------------------------------------
    // Overridden functions
    //------------------------------------------------------------------------------------------------------------------
    STDMETHOD(CreateDevice)(UINT Adapter,D3DDEVTYPE DeviceType,HWND hFocusWindow,DWORD BehaviorFlags,
        D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DDevice9** ppReturnedDeviceInterface)
    {
        //BehaviorFlags &= ~D3DCREATE_PUREDEVICE;
        gHWND = hFocusWindow;
        HRESULT res = mD3D->CreateDevice( Adapter, DeviceType, hFocusWindow, BehaviorFlags, 
            pPresentationParameters, ppReturnedDeviceInterface );

        if( *ppReturnedDeviceInterface )
        {
            DissectorDX9::Initialize( *ppReturnedDeviceInterface );
            DissectorDX9::SetUIPumpFunction( PumpFunction );

            IDirect3DDevice9Hooked* ret = new IDirect3DDevice9Hooked( *ppReturnedDeviceInterface );
            (*ppReturnedDeviceInterface)->AddRef();
            gDeviceBaseRefCount = (*ppReturnedDeviceInterface)->Release() - 1;
            assert( gDeviceBaseRefCount == 0 && "This has to be 0 or Reset() won't work properly." );
            *ppReturnedDeviceInterface = ret;

        }
        Log( "%08x CreateDevice: res %x, Adapter %u, DeviceType %x, hFocusWindow %x, BehaviorFlags %u,\
                \nBackBufferWidth %u, BackBufferHeight %u, BackBufferFormat %x, BackBufferCount %x,\
                \nMultiSampleType %x, MultiSampleQuality %x, SwapEffect %x, hDeviceWindow %x,\
                \nWindowed %x, EnableAutoDepthStencil %x, AutoDepthStencilFormat %x, Flags %x,\
                \nFullScreen_RefreshRateInHz %x, PresentationInterval %x\n",
                this, res, Adapter, DeviceType, hFocusWindow, BehaviorFlags,
                pPresentationParameters->BackBufferWidth, pPresentationParameters->BackBufferHeight, pPresentationParameters->BackBufferFormat, pPresentationParameters->BackBufferCount,
                pPresentationParameters->MultiSampleType, pPresentationParameters->MultiSampleQuality, pPresentationParameters->SwapEffect, pPresentationParameters->hDeviceWindow,
                pPresentationParameters->Windowed, pPresentationParameters->EnableAutoDepthStencil, pPresentationParameters->AutoDepthStencilFormat, pPresentationParameters->Flags,
                pPresentationParameters->FullScreen_RefreshRateInHz, pPresentationParameters->PresentationInterval );

        return res;
    }

    //------------------------------------------------------------------------------------------------------------------
    // Boiler-plate pass through
    //------------------------------------------------------------------------------------------------------------------
    STDMETHOD(QueryInterface)(REFIID riid, void** ppvObj)
    {
        HRESULT res = mD3D->QueryInterface( riid, ppvObj );
        Log( "%08x QueryInterface: res %x\n", this, res );
        return res;
    }

    STDMETHOD_(ULONG,AddRef)()
    {
        ULONG res = mD3D->AddRef();
        Log( "%08x D3D->AddRef: res %u\n", this, res );
        return res;
    }

    STDMETHOD_(ULONG,Release)()
    {
        ULONG res = mD3D->Release();
        if( res == 0 && sD3D == mD3D )
            sD3D = NULL;

        Log( "%08x D3D->Release: res %u\n", this, res );
        return res;
    }

	STDMETHOD(RegisterSoftwareDevice)(void* pInitializeFunction)
    {
        HRESULT res = mD3D->RegisterSoftwareDevice( pInitializeFunction );
        Log( "%08x RegisterSoftwareDevice: res %x\n", this, res );
        return res;
    }

    STDMETHOD_(UINT, GetAdapterCount)()
    {
        HRESULT res = mD3D->GetAdapterCount();
        Log( "%08x GetAdapterCount: res %x\n", this, res );
        return res;
    }

    STDMETHOD(GetAdapterIdentifier)(UINT Adapter,DWORD Flags,D3DADAPTER_IDENTIFIER9* pIdentifier)
    {
        HRESULT res = mD3D->GetAdapterIdentifier( Adapter, Flags, pIdentifier );
        Log( "%08x GetAdapterIdentifier: res %x, Adapter %u, Flags %x\n", this, res, Adapter, Flags );
        return res;
    }

    STDMETHOD_(UINT, GetAdapterModeCount)(UINT Adapter,D3DFORMAT Format)
    {
        HRESULT res = mD3D->GetAdapterModeCount( Adapter, Format );
        Log( "%08x GetAdapterModeCount: res %x, Adapter %u, Format %x\n", this, res, Adapter, Format );
        return res;
    }

    STDMETHOD(EnumAdapterModes)(UINT Adapter,D3DFORMAT Format,UINT Mode,D3DDISPLAYMODE* pMode)
    {
        HRESULT res = mD3D->EnumAdapterModes( Adapter, Format, Mode, pMode );
        Log( "%08x EnumAdapterModes: res %x Adapter %u, Format %x, Mode %u \n", this, res, Adapter, Format, Mode );
        return res;
    }

    STDMETHOD(GetAdapterDisplayMode)(UINT Adapter,D3DDISPLAYMODE* pMode)
    {
        HRESULT res = mD3D->GetAdapterDisplayMode( Adapter, pMode );
        Log( "%08x GetAdapterDisplayMode: res %x Adapter %u\n", this, res, Adapter );
        return res;
    }

    STDMETHOD(CheckDeviceType)(UINT Adapter,D3DDEVTYPE DevType,D3DFORMAT AdapterFormat,D3DFORMAT BackBufferFormat,BOOL bWindowed)
    {
        HRESULT res = mD3D->CheckDeviceType( Adapter, DevType, AdapterFormat, BackBufferFormat, bWindowed );
        Log( "%08x CheckDeviceType: res %x, Adapter %u, DevType %x, AdapterFormat %x, BackBufferFormat %x, bWindowed %u\n", this, res, Adapter, DevType, AdapterFormat, BackBufferFormat, bWindowed );
        return res;
    }

    STDMETHOD(CheckDeviceFormat)(UINT Adapter,D3DDEVTYPE DeviceType,D3DFORMAT AdapterFormat,DWORD Usage,
        D3DRESOURCETYPE RType,D3DFORMAT CheckFormat)
    {
        HRESULT res = mD3D->CheckDeviceFormat( Adapter, DeviceType, AdapterFormat, Usage, RType, CheckFormat );
        Log( "%08x CheckDeviceFormat: res %x, Adapter %u, DeviceType %x, AdapterFormat %x, Usage %x, RType %x, CheckFormat %x\n", this, res, Adapter, DeviceType, AdapterFormat, Usage, RType, CheckFormat );
        return res;
    }

    STDMETHOD(CheckDeviceMultiSampleType)(UINT Adapter,D3DDEVTYPE DeviceType,D3DFORMAT SurfaceFormat,BOOL Windowed,
        D3DMULTISAMPLE_TYPE MultiSampleType,DWORD* pQualityLevels)
    {
        HRESULT res = mD3D->CheckDeviceMultiSampleType( Adapter, DeviceType, SurfaceFormat, Windowed, MultiSampleType, pQualityLevels );
        Log( "%08x CheckDeviceMultiSampleType: res %x, Adapter %u, DeviceType %x, SurfaceFormat %x, Windowed %u, MultiSampleType %x, pQualityLevels %x\n", this, res, Adapter, DeviceType, SurfaceFormat, Windowed, MultiSampleType, pQualityLevels );
        return res;
    }

    STDMETHOD(CheckDepthStencilMatch)(UINT Adapter,D3DDEVTYPE DeviceType,D3DFORMAT AdapterFormat,D3DFORMAT RenderTargetFormat,
        D3DFORMAT DepthStencilFormat)
    {
        HRESULT res = mD3D->CheckDepthStencilMatch( Adapter, DeviceType, AdapterFormat, RenderTargetFormat, DepthStencilFormat );
        Log( "%08x CheckDepthStencilMatch: res %x, Adapter %u, DeviceType %x, AdapterFormat %x, RenderTargetFormat %x, DepthStencilFormat %x\n", this, res, Adapter, DeviceType, AdapterFormat, RenderTargetFormat, DepthStencilFormat );
        return res;
    }

    STDMETHOD(CheckDeviceFormatConversion)(UINT Adapter,D3DDEVTYPE DeviceType,D3DFORMAT SourceFormat,D3DFORMAT TargetFormat)
    {
        HRESULT res = mD3D->CheckDeviceFormatConversion( Adapter, DeviceType, SourceFormat, TargetFormat );
        Log( "%08x CheckDeviceFormatConversion: res %x, Adapter %u, DeviceType %x, SourceFormat %x, TargetFormat %x\n", this, res, Adapter, DeviceType, SourceFormat, TargetFormat );
        return res;
    }

    STDMETHOD(GetDeviceCaps)(UINT Adapter,D3DDEVTYPE DeviceType,D3DCAPS9* pCaps)
    {
        HRESULT res = mD3D->GetDeviceCaps( Adapter, DeviceType, pCaps );
        Log( "%08x GetDeviceCaps: res %x, Adapter %u, DeviceType %x,, pCaps %x,\n", this, res, Adapter, DeviceType, pCaps );
        return res;
    }

    STDMETHOD_(HMONITOR, GetAdapterMonitor)(UINT Adapter)
    {
        HMONITOR res = mD3D->GetAdapterMonitor( Adapter );
        Log( "%08x GetAdapterMonitor: res %x, Adapter %u\n", this, res, Adapter );
        return res;
    }
};


IDirect3D9* (WINAPI *Direct3DCreate9Unhooked)(UINT SDKVersion);
IDirect3D9* WINAPI Direct3DCreate9Hooked(UINT SDKVersion)
{
    IDirect3D9* dev = Direct3DCreate9Unhooked( SDKVersion );
    if( dev )
    {
        sD3D = dev;
        IDirect3D9Hooked* ret = new IDirect3D9Hooked( dev );
        return ret;
    }

    return dev;
}

HRESULT (WINAPI *Direct3DCreate9ExUnhooked)(UINT SDKVersion, IDirect3D9Ex **ppD3D);
HRESULT WINAPI Direct3DCreate9ExHooked(UINT SDKVersion, IDirect3D9Ex **ppD3D)
{
    IDirect3D9Ex* dev; 
    HRESULT res = Direct3DCreate9ExUnhooked( SDKVersion, &dev );
    if( S_OK == res && dev )
    {
        sD3D = dev;
        IDirect3D9Hooked* ret = new IDirect3D9Hooked( dev );
        return res;
    }

    return res;
}

void* GetFunctionAddress(const char *module, const char *function)
{
	HMODULE mod = GetModuleHandleA(module);
	if(mod == 0)
		return NULL;

	return (void *)GetProcAddress(mod, function);
}

BOOL HookFunction(const char *function, const char *module_name, void* destination_function_ptr,
                  void** unhooked_function_ptr )
{
	*unhooked_function_ptr = GetFunctionAddress(module_name, function);

	if(*unhooked_function_ptr == NULL)
		return false;

    return Mhook_SetHook(unhooked_function_ptr, destination_function_ptr);
}

void SetupHooks()
{
    HookFunction( "Direct3DCreate9", "d3d9.dll", Direct3DCreate9Hooked, (void**)&Direct3DCreate9Unhooked );
    HookFunction( "Direct3DCreate9Ex", "d3d9.dll", Direct3DCreate9ExHooked, (void**)&Direct3DCreate9ExUnhooked );
    HookFunction( "D3DPERF_BeginEvent", "d3d9.dll", D3DPERF_BeginEventHooked, (void**)&D3DPERF_BeginEventUnhooked );
    HookFunction( "D3DPERF_EndEvent", "d3d9.dll", D3DPERF_EndEventHooked, (void**)&D3DPERF_EndEventUnhooked );
}

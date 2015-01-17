#include "DLLExports.h"
#include "Dissector\Dissector.h"
#include "DissectorDX9\DissectorDX9.h"
#include "DissectorWinSock\DissectorWinSock.h"

void SetupHooks();

char sDissectorMemory[ 32 * 1024 * 1024 ];
extern "C" DECLDLL void __cdecl InitializeInjection()
{
    SetupHooks();
    OutputDebugString( L"Initializing Dissector\n" );
    Dissector::Initialize( sDissectorMemory, sizeof( sDissectorMemory ) );
    Dissector::SetMallocCallback( malloc );
    Dissector::SetFreeCallback( free );
    DissectorWinsock::Initialize();

    // TODO: Shutdown properly!
}

extern "C" DECLDLL bool __cdecl DoNothing()
{
    static int a = 1;
    return a == 1;
}
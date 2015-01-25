#include "DLLExports.h"
#include "Dissector\Dissector.h"
#include "DissectorDX9\DissectorDX9.h"
#include "DissectorWinSock\DissectorWinSock.h"

void SetupHooks();

extern "C" DECLDLL void __cdecl InitializeInjection()
{
    SetupHooks();
    OutputDebugString( L"Initializing Dissector\n" );
    Dissector::SetMallocCallback( malloc );
    Dissector::SetFreeCallback( free );

    Dissector::Initialize();
    DissectorWinsock::Initialize();

    // TODO: Shutdown properly!
}

extern "C" DECLDLL bool __cdecl DoNothing()
{
    static int a = 1;
    return a == 1;
}
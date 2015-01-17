// 
// Application that injects hooks in to another process and communicates with the DissectorGUI
//

#include "stdafx.h"
#include "Injection.h"
#include "DissectorInjectorDLL\DLLExports.h"

int wmain(int argc, WCHAR* argv[])
{
    DoNothing();

	STARTUPINFOW startinfo;
	PROCESS_INFORMATION procinfo;
    memset( &startinfo, 0, sizeof(startinfo) );
    memset( &procinfo, 0, sizeof(procinfo) );

    WCHAR blank[1] = { 0 };
    WCHAR* exe = blank, *dir = NULL, *args = blank;
    bool waitForDebugger = false;

    // -e "D:\code\D3dSamples\C++\Direct3D\Bin\x86\HDRCubeMap.exe" -d "D:\code\D3dSamples\C++\Direct3D\Bin\x86" -a "-w"
    for( int ii = 1; ii < argc; ++ii )
    {
        if( _wcsicmp( L"-e", argv[ii] ) == 0 )
        {
            if( (ii+1) < argc )
            {
                exe = argv[++ii];
            }
        }
        if( _wcsicmp( L"-d", argv[ii] ) == 0 )
        {
            if( (ii+1) < argc )
            {
                dir = argv[++ii];
            }
        }
        if( _wcsicmp( L"-w", argv[ii] ) == 0 )
        {
            waitForDebugger = true;
        }
        if( _wcsicmp( L"-a", argv[ii] ) == 0 )
        {
            if( (ii+1) < argc )
            {
                args = argv[++ii];
            }
        }
    }

    BOOL ret = CreateProcess(exe, args, NULL, NULL, FALSE,
                             CREATE_SUSPENDED, NULL, dir, &startinfo, &procinfo);


    if( !ret )
        return -1;

    InjectIntoProcess( procinfo.dwProcessId, false, waitForDebugger ? 60 : 0 );
    ResumeThread( procinfo.hThread );

	return 0;
}

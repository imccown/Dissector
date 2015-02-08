// 
// Application that injects hooks in to another process and communicates with the DissectorGUI
//

#include "stdafx.h"
#include "Injection.h"
#include "DissectorInjectorDLL\DLLExports.h"
#include <string>

int wmain(int argc, WCHAR* argv[])
{
    DoNothing(); // Necessary for some reason. I think it's because it calls in to the DLL. Also possibly magic.

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

    InjectReturns injectres = InjectIntoProcess(procinfo.dwProcessId, false, waitForDebugger ? 60 : 0);
    switch (injectres)
    {
    case(eInjectSuccess) : ResumeThread(procinfo.hThread); break;
    case(eInjectFailedBitMismatch) :
    {
        std::wstring exeName = argv[0];
#ifdef WIN64
        exeName = exeName.substr(0, exeName.length() - 8);
        exeName += L".exe";
#else
        exeName = exeName.substr(0, exeName.length() - 4);
        exeName += L"_x64.exe";
#endif
        std::wstring commandLine;
        for (WCHAR** iter = &argv[1], **end = &argv[argc]; iter < end; ++iter)
        {
            commandLine += ' ';
            commandLine += '"';
            commandLine += *iter;
            commandLine += '"';
        }

        BOOL ret = CreateProcess(exeName.c_str(), (LPWSTR)commandLine.c_str(), NULL, NULL, FALSE,
            CREATE_NO_WINDOW, NULL, dir, &startinfo, &procinfo);

    }break;
    default: break;
    }

	return 0;
}

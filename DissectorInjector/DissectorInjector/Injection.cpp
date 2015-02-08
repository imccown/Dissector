// Lifted These functions from RenderDoc. Some modifications. Thanks Crytek!
/******************************************************************************
 * The MIT License (MIT)
 * 
 * Copyright (c) 2014 Crytek
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#include "stdafx.h"
#include <tlhelp32.h> 
#include <stdio.h>
#include "Injection.h"

#ifndef _UINTPTR_T_DEFINED
#ifdef  _WIN64
typedef unsigned __int64    uintptr_t;
#else
typedef _W64 unsigned int   uintptr_t;
#endif
#define _UINTPTR_T_DEFINED
#endif

#if defined(WIN64)
static const char* dllName = "dissectorinjectordll_x64.dll";
static const WCHAR* dllNameL = L"dissectorinjectordll_x64.dll";
#else
static const char* dllName = "dissectorinjectordll.dll";
static const WCHAR* dllNameL = L"dissectorinjectordll.dll";
#endif


#define RDCEraseMem(a, b) memset(a, 0, b)
#define RDCEraseEl(a) memset(&a, 0, sizeof(a))

void InjectDLL(HANDLE hProcess, WCHAR* libName)
{
	WCHAR dllPath[MAX_PATH + 1] = {0};
	wcscpy_s(dllPath, libName);

	static HMODULE kernel32 = GetModuleHandle(L"Kernel32");

	void *remoteMem = VirtualAllocEx(hProcess, NULL, sizeof(dllPath), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	WriteProcessMemory(hProcess, remoteMem, (void *)dllPath, sizeof(dllPath), NULL);

	HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)GetProcAddress(kernel32, "LoadLibraryW"), remoteMem, 0, NULL);
	WaitForSingleObject(hThread, INFINITE);

	CloseHandle(hThread);
	VirtualFreeEx(hProcess, remoteMem, sizeof(dllPath), MEM_RELEASE); 
}

uintptr_t FindRemoteDLL(DWORD pid, const WCHAR* libName)
{
	HANDLE hModuleSnap = INVALID_HANDLE_VALUE;

	// up to 10 retries
	for(int i=0; i < 10; i++)
	{
		hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid); 

		if(hModuleSnap == INVALID_HANDLE_VALUE) 
		{
			DWORD err = GetLastError();

			printf("CreateToolhelp32Snapshot(%u) -> 0x%08x\n", pid, err);

			// retry if error is ERROR_BAD_LENGTH
			if(err == ERROR_BAD_LENGTH)
				continue;
		} 

		// didn't retry, or succeeded
		break;
	}

	if(hModuleSnap == INVALID_HANDLE_VALUE) 
	{
		printf("Couldn't create toolhelp dump of modules in process %u\n", pid);
		return 0;
	} 
 
  MODULEENTRY32 me32; 
	RDCEraseEl(me32);
  me32.dwSize = sizeof(MODULEENTRY32); 
 
	BOOL success = Module32First(hModuleSnap, &me32);

  if(success == FALSE) 
  { 
		DWORD err = GetLastError();

		printf("Couldn't get first module in process %u: 0x%08x\n", pid, err);
    CloseHandle(hModuleSnap);
		return 0;
  }

	uintptr_t ret = 0;

	int numModules = 0;
 
  do
  {
		wchar_t modnameLower[MAX_MODULE_NAME32+1];
		RDCEraseEl(modnameLower);
		wcsncpy_s(modnameLower, me32.szModule, MAX_MODULE_NAME32);

		wchar_t *wc = &modnameLower[0];
		while(*wc)
		{
			*wc = towlower(*wc);
			wc++;
		}

		numModules++;

		if(wcsstr(modnameLower, libName))
		{
			ret = (uintptr_t)me32.modBaseAddr;
		}
  } while(ret == 0 && Module32Next(hModuleSnap, &me32)); 

	if(ret == 0)
	{
		printf("Couldn't find module '%ls' among %d modules\n", libName, numModules);
	}

  CloseHandle(hModuleSnap); 

	return ret;
}

void InjectFunctionCall(HANDLE hProcess, uintptr_t renderdoc_remote, const char *funcName, void *data, const size_t dataLen)
{
	if(dataLen == 0)
	{
		printf("Invalid function call injection attempt");
		return;
	}

	printf("Injecting call to %hs", funcName);
	

    HMODULE renderdoc_local = GetModuleHandleA(dllName);
	
	uintptr_t func_local = (uintptr_t)GetProcAddress(renderdoc_local, funcName);

	// we've found SetCaptureOptions in our local instance of the module, now calculate the offset and so get the function
	// in the remote module (which might be loaded at a different base address
	uintptr_t func_remote = func_local + renderdoc_remote - (uintptr_t)renderdoc_local;
	
	void *remoteMem = VirtualAllocEx(hProcess, NULL, dataLen, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	SIZE_T numWritten;
	WriteProcessMemory(hProcess, remoteMem, data, dataLen, &numWritten);
	
	HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)func_remote, remoteMem, 0, NULL);
	WaitForSingleObject(hThread, INFINITE);

	ReadProcessMemory(hProcess, remoteMem, data, dataLen, &numWritten);

	CloseHandle(hThread);
	VirtualFreeEx(hProcess, remoteMem, dataLen, MEM_RELEASE);
}

InjectReturns InjectIntoProcess(unsigned int pid, bool waitForExit, unsigned int waitForDebugger)
{
	HANDLE hProcess = OpenProcess( PROCESS_CREATE_THREAD | 
		PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
        PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_TERMINATE | SYNCHRONIZE,
		FALSE, pid );

	if(waitForDebugger > 0)
	{
		printf("Waiting for debugger attach to %lu", pid);
		unsigned int timeout = 0;

		BOOL debuggerAttached = FALSE;

		while(!debuggerAttached)
		{
			CheckRemoteDebuggerPresent(hProcess, &debuggerAttached);

			Sleep(10);
			timeout += 10;

			if(timeout > waitForDebugger*1000)
				break;
		}

		if(debuggerAttached)
			printf("Debugger attach detected after %.2f s", float(timeout)/1000.0f);
		else
			printf("Timed out waiting for debugger, gave up after %u s", waitForDebugger);
	}

	//RDCLOG("Injecting renderdoc into process %lu", pid);
	
	wchar_t dllpath[MAX_PATH] = {0};
    GetModuleFileNameW(GetModuleHandleA(dllName), &dllpath[0], MAX_PATH - 1);

	BOOL isWow64 = FALSE;
	BOOL success = IsWow64Process(hProcess, &isWow64); // This actually returns if the application bitness is different than the OS bitness

	if(!success)
	{
		printf("Couldn't determine bitness of process\n");
		CloseHandle(hProcess);
        return eInjectFailed;
	}

#ifdef WIN64
    if (isWow64)
    {
        TerminateProcess(hProcess, 0);
        return eInjectFailedBitMismatch;
    }
#else
    if (!isWow64)
    {
        TerminateProcess(hProcess, 0);
        return eInjectFailedBitMismatch;
    }
#endif
	// misc
	InjectDLL(hProcess, L"kernel32.dll");

	// D3D11
	InjectDLL(hProcess, L"d3d9.dll");

    InjectDLL(hProcess, dllpath);
    
    uintptr_t loc = FindRemoteDLL(pid, dllNameL);
	
	unsigned int remoteident = 0;

	if(loc == 0)
	{
		printf("Can't locate dissectorinjectordll.dll in remote PID %d", pid);
	}
	else
	{
		// safe to cast away the const as we know these functions don't modify the parameters
		InjectFunctionCall(hProcess, loc, "InitializeInjection", &remoteident, sizeof(remoteident));
	}

	if(waitForExit)
		WaitForSingleObject(hProcess, INFINITE);

	CloseHandle(hProcess);

	return eInjectSuccess;
}
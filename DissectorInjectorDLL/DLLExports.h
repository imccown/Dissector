#pragma once

#ifdef _WINDLL
#define DECLDLL __declspec(dllexport)
#else
#define DECLDLL __declspec(dllimport)
#endif

extern "C" DECLDLL void __cdecl InitializeInjection();
extern "C" DECLDLL bool __cdecl DoNothing();

#include <Windows.h>
#include "forwards_kernel32.h"

#define EXPORT extern "C"

#pragma comment(linker,"/export:GlobalAlloc=_DetourGlobalAlloc@8")
EXPORT HGLOBAL WINAPI DetourGlobalAlloc(UINT uFlags, SIZE_T dwBytes)
{
	HGLOBAL result = GlobalAlloc(uFlags, dwBytes);
	if (result)
	{
		unsigned long protect[2];
		VirtualProtect(result, dwBytes, PAGE_EXECUTE_READWRITE, &protect[0]);
	}
	return result;
}

#pragma comment(linker,"/export:GlobalReAlloc=_DetourGlobalReAlloc@12")
EXPORT HGLOBAL WINAPI DetourGlobalReAlloc(HGLOBAL hMem, SIZE_T dwBytes,
	UINT uFlags)
{
	HGLOBAL result = GlobalReAlloc(hMem, dwBytes, uFlags);
	if (result)
	{
		unsigned long protect[2];
		VirtualProtect(result, dwBytes, PAGE_EXECUTE_READWRITE, &protect[0]);
	}
	return result;
}
/*
#pragma comment(linker,"/export:LocalAlloc=_DetourLocalAlloc@8")
EXPORT HLOCAL WINAPI DetourLocalAlloc(UINT uFlags, SIZE_T uBytes)
{
	LPVOID result = LocalAlloc(uFlags, uBytes);
	if (result)
	{
		unsigned long protect[2];
		VirtualProtect(result, uBytes, PAGE_EXECUTE_READWRITE, &protect[0]);
	}
	return result;
}

#pragma comment(linker,"/export:LocalReAlloc=_DetourLocalReAlloc@12")
EXPORT HLOCAL WINAPI DetourLocalReAlloc(HLOCAL hMem, SIZE_T uBytes, UINT uFlags)
{
	LPVOID result = LocalReAlloc(hMem, uBytes, uFlags);
	if (result)
	{
		unsigned long protect[2];
		VirtualProtect(result, uBytes, PAGE_EXECUTE_READWRITE, &protect[0]);
	}
	return result;
}*/

#pragma comment(linker,"/export:VirtualAlloc=_DetourVirtualAlloc@16")
EXPORT LPVOID WINAPI DetourVirtualAlloc(LPVOID lpAddress, SIZE_T dwSize,
	DWORD flAllocationType, DWORD flProtect)
{
	LPVOID result = VirtualAlloc(lpAddress, dwSize, flAllocationType, flProtect);
	if (result)
	{
		unsigned long protect[2];
		VirtualProtect(result, dwSize, PAGE_EXECUTE_READWRITE, &protect[0]);
	}
	return result;
}

BOOL APIENTRY DllMain(HMODULE hModule,
											DWORD  ul_reason_for_call,
											LPVOID lpReserved)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH )
	{
		DisableThreadLibraryCalls(hModule);
	}
	return 1;
}
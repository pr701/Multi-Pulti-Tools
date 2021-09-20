#include <Windows.h>
#include <tchar.h>
//#include "forwards_ddraw.h"

#include "fullscreen.h"

#define EXPORT extern "C"
#define DDERR_GENERIC 0x80004005L

typedef enum _PROCESS_DPI_AWARENESS {
	PROCESS_DPI_UNAWARE = 0,
	PROCESS_SYSTEM_DPI_AWARE = 1,
	PROCESS_PER_MONITOR_DPI_AWARE = 2
} PROCESS_DPI_AWARENESS;

// main hook
typedef HRESULT(WINAPI *DirectDrawCreate_t)(GUID *lpGUID, void *lplpDD, IUnknown *pUnkOuter);
// Win 8.1 >
typedef HRESULT(WINAPI *SetProcessDpiAwareness_t)(PROCESS_DPI_AWARENESS);
// Vista >
typedef BOOL(WINAPI *SetProcessDPIAware_t)(void);

static SetProcessDpiAwareness_t pSetProcessDpiAwareness = NULL;
static SetProcessDPIAware_t pSetProcessDPIAware = NULL;
static DirectDrawCreate_t pDirectDrawCreate = NULL;
static DWORD g_EnableFullscreen = 0;

#pragma comment(linker,"/export:DirectDrawCreate=_DetourDirectDrawCreate@12")
EXPORT HRESULT WINAPI DetourDirectDrawCreate(GUID *lpGUID, void *lplpDD, IUnknown *pUnkOuter)
{
	if (pDirectDrawCreate)
		return pDirectDrawCreate(lpGUID, lplpDD, pUnkOuter);
	return DDERR_GENERIC;
}

void ProcessDpiAwareness()
{
	HMODULE hModule = GetModuleHandleA("user32.dll");
	if (hModule)
	{
		pSetProcessDpiAwareness = (SetProcessDpiAwareness_t)GetProcAddress(hModule, "SetProcessDpiAwarenessInternal");
		if (pSetProcessDpiAwareness)
		{
			pSetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
		}
		else
		{
			// load vista - win8
			pSetProcessDPIAware = (SetProcessDPIAware_t)GetProcAddress(hModule, "SetProcessDPIAware");
			if (pSetProcessDPIAware)
				pSetProcessDPIAware();
		}
	}
}

bool GetPathToModule(wchar_t* wzPath, size_t uLen)
{
	if (!wzPath)
		return false;

	HMODULE hModule;
	*wzPath = 0;
	if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(wchar_t*)GetPathToModule, &hModule))
		return false;

	if (!GetModuleFileNameW(hModule, wzPath, uLen - 1))
		return false;

	size_t len = lstrlenW(wzPath);
	for (size_t i = len - 1; i > 0; --i)
		if (wzPath[i] == '\\')
		{
			wzPath[i + 1] = 0;
			break;
		}
	return true;
}

void Configurate()
{
	// Load system lib
	HMODULE hModule = LoadLibraryExA("ddraw.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (hModule)
		pDirectDrawCreate = (DirectDrawCreate_t)GetProcAddress(hModule, "DirectDrawCreate");

	// Fix DPI
	ProcessDpiAwareness();

	wchar_t wzPath[MAX_PATH * 2];
	if (GetPathToModule(wzPath, ARRAYSIZE(wzPath)))
	{
		SetCurrentDirectoryW(wzPath);

		lstrcatW(wzPath, L"Setup.ini");
		g_EnableFullscreen = GetPrivateProfileIntW(L"Param", L"Fullscreen", 0, wzPath);
	}
}

BOOL APIENTRY DllMain(HMODULE hModule,
					DWORD  ul_reason_for_call,
					LPVOID lpReserved)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH )
	{
		DisableThreadLibraryCalls(hModule);

		Configurate();

		if (g_EnableFullscreen) Fullscreen::StartThread();
	}
	if (ul_reason_for_call == DLL_PROCESS_DETACH)
	{
		if (g_EnableFullscreen) Fullscreen::StopThread();
	}

	return 1;
}
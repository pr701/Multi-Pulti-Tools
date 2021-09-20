/*
 Modified part of dxwrapper project by elishacloud
 https://github.com/elishacloud/dxwrapper
*/

#include "fullscreen.h"

namespace Fullscreen
{
	// Declare constants
	static LONG g_LoopSleepTime = 120;
	static LONG g_WindowSleepTime = 500;

	static LONG MinWindowWidth = 4;			// Minimum window width for valid window check
	static LONG MinWindowHeight = 4;			// Minimum window height for valid window check
	static LONG WindowDelta = 40;				// Delta between window size and screensize for fullscreen check
	static DWORD TerminationCount = 10;		// Minimum number of loops to check for termination
	static DWORD TerminationWaitTime = 2000;	// Minimum time to wait for termination (LoopSleepTime * NumberOfLoops)

	struct window_layer
	{
		HWND hwnd = nullptr;
		bool IsMain = false;
		bool IsFullScreen = false;
	};

	struct handle_data
	{
		DWORD process_id = 0;
		HWND best_handle = nullptr;
		DWORD LayerNumber = 0;
		window_layer Windows[MAX_PATH];
		bool AutoDetect = true;
		bool Debug = false;
	};

	// Overload operators
	bool operator==(const RECT& a, const RECT& b)
	{
		return (a.bottom == b.bottom && a.left == b.left && a.right == b.right && a.top == b.top);
	}

	bool operator!=(const RECT& a, const RECT& b)
	{
		return (a.bottom != b.bottom || a.left != b.left || a.right != b.right || a.top != b.top);
	}

	// Declare structures
	struct screen_res
	{
		LONG Width = 0;
		LONG Height = 0;

		screen_res& operator=(const screen_res& a)
		{
			Width = a.Width;
			Height = a.Height;
			return *this;
		}

		bool operator==(const screen_res& a) const
		{
			return (Width == a.Width && Height == a.Height);
		}

		bool operator!=(const screen_res& a) const
		{
			return (Width != a.Width || Height != a.Height);
		}
	};

	struct window_update
	{
		HWND hwnd = nullptr;
		HWND ChildHwnd = nullptr;
		RECT rect;
		screen_res ScreenSize;

		window_update& operator=(const window_update& a)
		{
			hwnd = a.hwnd;
			ChildHwnd = a.ChildHwnd;
			rect.bottom = a.rect.bottom;
			rect.left = a.rect.left;
			rect.right = a.rect.right;
			rect.top = a.rect.top;
			ScreenSize = a.ScreenSize;
			return *this;
		}

		bool operator==(const window_update& a) const
		{
			return (hwnd == a.hwnd && ChildHwnd == a.ChildHwnd && rect == a.rect && ScreenSize == a.ScreenSize);
		}

		bool operator!=(const window_update& a) const
		{
			return (hwnd != a.hwnd || ChildHwnd != a.ChildHwnd || rect != a.rect || ScreenSize != a.ScreenSize);
		}
	};

	// Declare variables
	bool g_StopThreadFlag = false;
	bool g_ThreadRunningFlag = false;
	HANDLE g_hThread = nullptr;
	DWORD g_dwThreadID = 0;

	// Gets the screen size from a wnd handle
	void GetScreenSize(HWND& hwnd, screen_res& Res, MONITORINFO& mi)
	{
		GetMonitorInfo(MonitorFromWindow((IsWindow(hwnd)) ? hwnd : GetDesktopWindow(), MONITOR_DEFAULTTONEAREST), &mi);
		Res.Width = mi.rcMonitor.right - mi.rcMonitor.left;
		Res.Height = mi.rcMonitor.bottom - mi.rcMonitor.top;
	}

	void GetScreenSize(HWND hwnd, LONG &screenWidth, LONG &screenHeight)
	{
		MONITORINFO mi = {};
		mi.cbSize = sizeof(MONITORINFO);
		Fullscreen::screen_res Res;
		Fullscreen::GetScreenSize(hwnd, Res, mi);
		screenWidth = Res.Width;
		screenHeight = Res.Height;
	}

	void GetScreenSize(HWND hwnd, DWORD &screenWidth, DWORD &screenHeight)
	{
		LONG Width, Height;
		GetScreenSize(hwnd, Width, Height);
		screenWidth = Width;
		screenHeight = Height;
	}

	// Check is fullscreen
	bool IsWindowFullScreen(screen_res WindowSize, screen_res ScreenSize)
	{
		return abs(ScreenSize.Width - WindowSize.Width) <= WindowDelta ||		// Window width matches screen width
			abs(ScreenSize.Height - WindowSize.Height) <= WindowDelta;			// Window height matches screen height
	}

	// Check is not fullscreen
	bool IsWindowNotFullScreen(screen_res WindowSize, screen_res ScreenSize)
	{
		return (ScreenSize.Width - WindowSize.Width) > WindowDelta ||			// Window width does not match screen width
			(ScreenSize.Height - WindowSize.Height) > WindowDelta;				// Window height does not match screen height
	}

	bool IsMainWindow(HWND hwnd)
	{
		return GetWindow(hwnd, GW_OWNER) == (HWND)0 && IsWindowVisible(hwnd);
	}

	bool IsWindowTooSmall(screen_res WindowSize)
	{
		return WindowSize.Width < MinWindowWidth || WindowSize.Height < MinWindowHeight;
	}


	// Gets the window size from a handle
	void GetWindowSize(HWND& hwnd, screen_res& Res, RECT& rect)
	{
		GetWindowRect(hwnd, &rect);
		Res.Width = abs(rect.right - rect.left);
		Res.Height = abs(rect.bottom - rect.top);
	}

	// Enums all windows and returns the handle to the active window
	BOOL CALLBACK EnumWindowsCallback(HWND hwnd, LPARAM lParam)
	{
		// Get variables from call back
		handle_data& data = *(handle_data*)lParam;

		// Skip windows that are from a different process ID
		DWORD process_id;
		GetWindowThreadProcessId(hwnd, &process_id);
		if (data.process_id != process_id)
		{
			return true;
		}

		// Skip compatibility class windows
		char CompatibilityClass[80] = { 0 };
		GetClassNameA(hwnd, CompatibilityClass, ARRAYSIZE(CompatibilityClass));
		if (strcmp(CompatibilityClass, "CompatWindowDesktopReplacement") == 0) // Compatibility class windows
		{
			return true;
		}

		// Skip windows of zero size
		RECT rect = { sizeof(rect) };
		screen_res WindowSize;
		GetWindowSize(hwnd, WindowSize, rect);
		if (WindowSize.Height == 0 && WindowSize.Width == 0)
		{
			return true;
		}

		// AutoDetect to search for main and fullscreen windows
		if (data.AutoDetect)
		{
			// Declare vars
			MONITORINFO mi = { sizeof(mi) };
			screen_res ScreenSize;

			// Get window and monitor information
			GetScreenSize(hwnd, ScreenSize, mi);

			// Store window layer information
			++data.LayerNumber;
			data.Windows[data.LayerNumber].hwnd = hwnd;
			data.Windows[data.LayerNumber].IsFullScreen = IsWindowFullScreen(WindowSize, ScreenSize);
			data.Windows[data.LayerNumber].IsMain = IsMainWindow(hwnd);

			// Check if the window is the best window
			if (data.Windows[data.LayerNumber].IsFullScreen && data.Windows[data.LayerNumber].IsMain)
			{
				// Match found returning value
				data.best_handle = hwnd;
				return false;
			}
		}
		else
		{
			//data.best_handle = hwnd;
			//return false;
		}
		// Return to loop again
		return true;
	}

	HWND FindMainWindow(DWORD process_id, bool AutoDetect, bool Debug)
	{
		// Set variables
		HWND WindowsHandle = nullptr;
		handle_data data;
		data.best_handle = nullptr;
		data.process_id = process_id;
		data.AutoDetect = AutoDetect;
		data.LayerNumber = 0;
		data.Debug = Debug;

		// Gets all window layers and looks for a main window that is fullscreen
		EnumWindows(EnumWindowsCallback, (LPARAM)&data);
		WindowsHandle = data.best_handle;

		// If no main fullscreen window found then check for other windows
		if (!WindowsHandle)
		{
			for (DWORD x = 1; x <= data.LayerNumber; x++)
			{
				// Return the first fullscreen window layer
				if (data.Windows[x].IsFullScreen)
				{
					return data.Windows[x].hwnd;
				}
				// If no fullscreen layer then return the first 'main' window
				if (!WindowsHandle && data.Windows[x].IsMain)
				{
					WindowsHandle = data.Windows[x].hwnd;
				}
			}
		}

		// Get first window handle if no handle has been found yet
		if (!WindowsHandle && data.LayerNumber > 0)
		{
			WindowsHandle = data.Windows[1].hwnd;
		}

		// Return the best handle
		return WindowsHandle;
	}

	// Check with resolution is best
	LONG GetBestResolution(screen_res& ScreenRes, LONG xWidth, LONG xHeight)
	{
		//Set vars
		DEVMODE dm = { 0 };
		dm.dmSize = sizeof(dm);
		LONG diff = 40000;
		LONG NewDiff = 0;
		ScreenRes.Width = 0;
		ScreenRes.Height = 0;

		// Get closest resolution
		for (DWORD iModeNum = 0; EnumDisplaySettings(nullptr, iModeNum, &dm) != 0; iModeNum++)
		{
			NewDiff = abs((LONG)dm.dmPelsWidth - xWidth) + abs((LONG)dm.dmPelsHeight - xHeight);
			if (NewDiff < diff)
			{
				diff = NewDiff;
				ScreenRes.Width = (LONG)dm.dmPelsWidth;
				ScreenRes.Height = (LONG)dm.dmPelsHeight;
			}
		}
		return diff;
	}

	// Sets the resolution of the screen
	void SetScreenResolution(LONG xWidth, LONG xHeight)
	{
		DEVMODE newSettings;
		ZeroMemory(&newSettings, sizeof(newSettings));
		if (EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &newSettings) != 0)
		{
			newSettings.dmPelsWidth = xWidth;
			newSettings.dmPelsHeight = xHeight;
			newSettings.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
			ChangeDisplaySettings(&newSettings, CDS_FULLSCREEN);
		}
	}

	// Verifies input and sets screen res to the values sent
	void SetScreen(screen_res ScreenRes)
	{
		// Verify stored values are large enough
		if (!IsWindowTooSmall(ScreenRes))
		{
			// Get the best screen resolution
			GetBestResolution(ScreenRes, ScreenRes.Width, ScreenRes.Height);

			// Set screen to new resolution
			SetScreenResolution(ScreenRes.Width, ScreenRes.Height);
		}
	}

	// Sets the window to fullscreen
	void SetFullScreen(HWND& hwnd, const MONITORINFO& mi)
	{
		// Attach to window thread
		DWORD h_ThreadID = GetWindowThreadProcessId(hwnd, nullptr);
		AttachThreadInput(InterlockedCompareExchange(&g_dwThreadID, 0, 0), h_ThreadID, true);

		// Try restoring the window to normal
		PostMessage(hwnd, WM_SYSCOMMAND, SW_SHOWNORMAL, 0);

		// Window placement helps ensure the window can be seen (sometimes windows appear as minimized)
		WINDOWPLACEMENT wp;
		wp.length = sizeof(wp);
		GetWindowPlacement(hwnd, &wp);
		wp.showCmd = SW_MAXIMIZE | SW_RESTORE | SW_SHOW | SW_SHOWMAXIMIZED | SW_SHOWNORMAL;
		wp.flags = WPF_RESTORETOMAXIMIZED;
		SetWindowPlacement(hwnd, &wp);

		// Set window style
		DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
		SetWindowLong(hwnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);

		// Set window to fullscreen
		SetWindowPos(hwnd, HWND_TOP,
			mi.rcMonitor.left, mi.rcMonitor.top,
			mi.rcMonitor.right - mi.rcMonitor.left,
			mi.rcMonitor.bottom - mi.rcMonitor.top,
			SWP_ASYNCWINDOWPOS | SWP_NOSENDCHANGING | SWP_FRAMECHANGED);

		// Set window to forground
		SetForegroundWindow(hwnd);

		// Dettach from window thread
		AttachThreadInput(InterlockedCompareExchange(&g_dwThreadID, 0, 0), h_ThreadID, false);

		// Set focus and activate
		SetFocus(hwnd);
		SetActiveWindow(hwnd);
	}

	void ResetScreen()
	{
		// Reset screen settings
		char Ramp[3 * 256 * 2] = { 0 };
		HDC hDC = GetDC(nullptr);
		GetDeviceGammaRamp(hDC, &Ramp[0]);
		Sleep(0);
		SetDeviceGammaRamp(hDC, &Ramp[0]);
		ReleaseDC(nullptr, hDC);
		Sleep(0);
		ChangeDisplaySettings(nullptr, 0);
	}

	// Get child window handle
	BOOL CALLBACK EnumChildWindowsProc(HWND hwnd, LPARAM lParam)
	{
		HWND &data = *(HWND*)lParam;
		data = hwnd;
		return true;
	}

	void FullscreenCallback()
	{
		// Declare vars
		window_update CurrentLoop;
		window_update PreviousLoop;
		window_update LastFullscreenLoop;
		screen_res WindowSize;
		MONITORINFO mi = { sizeof(mi) };
		char ClassName[80] = { 0 };
		bool ChangeDetectedFlag = false;
		bool NoChangeFromLastRunFlag = false;
		bool HasNoMenu = false;
		bool IsNotFullScreenFlag = false;

		// Get process ID
		DWORD processId = GetCurrentProcessId();

		// Short sleep to allow other items to load
		Sleep(100);

		// Start main fullscreen loop
		while (!g_StopThreadFlag)
		{
			// Starting loop
			// Get window hwnd for specific layer
			CurrentLoop.hwnd = FindMainWindow(processId, true, false);

			// Start Fullscreen method
			if (IsWindow(CurrentLoop.hwnd))
			{
				// Get window child hwnd
				EnumChildWindows(CurrentLoop.hwnd, EnumChildWindowsProc, (LPARAM)&CurrentLoop.ChildHwnd);

				// Get window and monitor information
				GetClassNameA(CurrentLoop.hwnd, ClassName, ARRAYSIZE(ClassName));
				GetWindowSize(CurrentLoop.hwnd, WindowSize, CurrentLoop.rect);
				GetScreenSize(CurrentLoop.hwnd, CurrentLoop.ScreenSize, mi);

				// Check if window is not fullscreen
				IsNotFullScreenFlag = IsWindowNotFullScreen(WindowSize, CurrentLoop.ScreenSize);

				// Check if change is detected
				ChangeDetectedFlag = (IsNotFullScreenFlag ||		// Check if it is not fullscreen
					CurrentLoop != LastFullscreenLoop);				// Check if the window or screen details have changed

				// Check if there is no change from the last run
				NoChangeFromLastRunFlag = (CurrentLoop == PreviousLoop);

				// Change detected in screen resolution or window
				if (ChangeDetectedFlag && NoChangeFromLastRunFlag && IsWindow(CurrentLoop.hwnd))
				{
					// Check if window is not too small
					if (!IsWindowTooSmall(WindowSize))
					{
						// Update screen when change detected
						SetWindowPos(CurrentLoop.hwnd, HWND_TOP, 0, 0, 0, 0, SWP_ASYNCWINDOWPOS | SWP_NOSENDCHANGING | SWP_FRAMECHANGED | SWP_NOSIZE);

						// Change resolution if not fullscreen and ignore certian windows
						if (IsNotFullScreenFlag	&& IsWindow(CurrentLoop.hwnd))																							// Check window handle
						{
							// Get the best screen resolution
							screen_res SizeTemp;
							LONG Delta = GetBestResolution(SizeTemp, WindowSize.Width, WindowSize.Height);

							// Check if the window is same size as a supported screen resolution
							if (Delta <= WindowDelta)
							{
								// Set screen to new resolution
								SetScreenResolution(SizeTemp.Width, SizeTemp.Height);

								// Update window and monitor information
								GetWindowSize(CurrentLoop.hwnd, WindowSize, CurrentLoop.rect);
								GetScreenSize(CurrentLoop.hwnd, CurrentLoop.ScreenSize, mi);
							}
						} // Change resolution

						// Set fullscreen
						if (((IsWindowFullScreen(WindowSize, CurrentLoop.ScreenSize) ||							// Check if window size is the same as screen size
							abs(CurrentLoop.rect.bottom) > 30000 || abs(CurrentLoop.rect.right) > 30000) &&		// Check if window is outside coordinate range
							IsWindow(CurrentLoop.hwnd)))														// Check for valide window handle
						{
							// Set window to fullscreen
							SetFullScreen(CurrentLoop.hwnd, mi);
						}

						// Update window and monitor information
						GetWindowSize(CurrentLoop.hwnd, WindowSize, CurrentLoop.rect);
						GetScreenSize(CurrentLoop.hwnd, CurrentLoop.ScreenSize, mi);

						// Save last loop information
						LastFullscreenLoop = CurrentLoop;

					} // Window is too small

				} // Change detected in screen resolution or window

			} // Start Fullscreen method

			// Store last loop information
			PreviousLoop = CurrentLoop;

			// Wait for a while
			Sleep(g_LoopSleepTime + (ChangeDetectedFlag * g_WindowSleepTime));

		} // Main while loop
	}

	DWORD WINAPI StartThreadFunc(LPVOID pvParam)
	{
		UNREFERENCED_PARAMETER(pvParam);

		// Get thread handle
		InterlockedExchangePointer(&g_hThread, GetCurrentThread());
		if (!InterlockedCompareExchangePointer(&g_hThread, nullptr, nullptr))
		{
			return 0;
		}

		// Set thread flag to running
		g_ThreadRunningFlag = true;

		// Set threat priority high, trick to reduce concurrency problems
		SetThreadPriority(InterlockedCompareExchangePointer(&g_hThread, nullptr, nullptr), THREAD_PRIORITY_HIGHEST);

		// Start main fullscreen function
		FullscreenCallback();

		// Reset thread flag before exiting
		g_ThreadRunningFlag = false;

		// Close handle
		CloseHandle(InterlockedExchangePointer(&g_hThread, nullptr));

		// Set thread ID back to 0
		InterlockedExchange(&g_dwThreadID, 0);

		// Return value
		return 0;
	}

	// Create fullscreen thread
	void StartThread()
	{
		// Start thread
		CreateThread(nullptr, 0, StartThreadFunc, nullptr, 0, &g_dwThreadID);
	}

	// Stop thread
	void StopThread()
	{
		// Set flag to stop thread
		g_StopThreadFlag = true;

		// Wait for thread to exit
		if (g_ThreadRunningFlag && InterlockedCompareExchange(&g_dwThreadID, 0, 0) &&
			GetThreadId(InterlockedCompareExchangePointer(&g_hThread, nullptr, nullptr))
			== InterlockedCompareExchange(&g_dwThreadID, 0, 0))
		{
			// Wait for thread to exit
			WaitForSingleObject(InterlockedCompareExchangePointer(&g_hThread, nullptr, nullptr), INFINITE);
		}
	}
}

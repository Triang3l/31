#include "abPlatform.h"
#ifdef abPlatform_OS_WindowsDesktop
#include "../abCore/abCore.h"
#include <Windows.h>

static HWND abWindowi_WindowsDesktop_Window_HWnd = abNull;

static LRESULT CALLBACK abPlatformi_WindowsDesktop_Window_Proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_CLOSE:
		abCore_RequestQuit(abFalse);
		return 0;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

abBool abPlatform_Window_Init(unsigned int width, unsigned int height) {
	WNDCLASSEXA classDesc = {
		.cbSize = sizeof(WNDCLASSEXA),
		.lpfnWndProc = abPlatformi_WindowsDesktop_Window_Proc,
		.hInstance = GetModuleHandleA(abNull),
		.hCursor = LoadCursor(abNull, IDC_ARROW),
		.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH),
		.lpszClassName = "Tri1Game"
	};
	if (!RegisterClassExA(&classDesc)) {
		return abFalse;
	}
	DWORD const style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
	width = abMax(width, 1u);
	height = abMax(height, 1u);
	RECT rect = { .left = 0, .top = 0, .right = (LONG) width, .bottom = (LONG) height };
	AdjustWindowRect(&rect, style, FALSE);
	int adjustedWidth = rect.right - rect.left, adjustedHeight = rect.bottom - rect.top;
	abWindowi_WindowsDesktop_Window_HWnd = CreateWindowA(classDesc.lpszClassName, "31", style,
			(GetSystemMetrics(SM_CXSCREEN) - adjustedWidth) >> 1u, (GetSystemMetrics(SM_CYSCREEN) - adjustedHeight) >> 1u,
			adjustedWidth, adjustedHeight, abNull, abNull, classDesc.hInstance, abNull);
	if (abWindowi_WindowsDesktop_Window_HWnd == abNull) {
		return abFalse;
	}
	ShowWindow(abWindowi_WindowsDesktop_Window_HWnd, SW_SHOWNORMAL);
	return abTrue;
}

void abPlatform_Window_ProcessEvents() {
	MSG message;
	while (PeekMessageA(&message, abNull, 0u, 0u, PM_NOREMOVE)) {
		BOOL gotMessage = GetMessageA(&message, abNull, 0u, 0u);
		if (gotMessage == (BOOL) -1) { // Error in GetMessage.
			continue;
		}
		if (!gotMessage) { // Got WM_QUIT.
			abCore_RequestQuit(abFalse);
		}
		TranslateMessage(&message);
		DispatchMessageA(&message);
	}
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow) {
	abCore_Run();
	return 0;
}

#endif

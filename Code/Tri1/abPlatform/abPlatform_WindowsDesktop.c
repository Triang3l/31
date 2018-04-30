#include "abPlatform.h"
#ifdef abPlatform_OS_WindowsDesktop
#include "../abCore/abCore.h"
#include "../abGPU/abGPU.h"
#include <Windows.h>

static HWND abPlatformi_WindowsDesktop_Window_HWnd = abNull;

static LRESULT CALLBACK abPlatformi_WindowsDesktop_Window_Proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_CLOSE:
		abCore_RequestQuit(abFalse);
		return 0;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

abBool abPlatform_Window_Init(unsigned int width, unsigned int height) {
	WNDCLASSEXW classDesc = {
		.cbSize = sizeof(WNDCLASSEXW),
		.lpfnWndProc = abPlatformi_WindowsDesktop_Window_Proc,
		.hInstance = GetModuleHandleW(abNull),
		.hCursor = LoadCursor(abNull, IDC_ARROW),
		.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH),
		.lpszClassName = L"Tri1Game"
	};
	if (!RegisterClassExW(&classDesc)) {
		return abFalse;
	}
	DWORD const style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
	width = abMax(width, 1u);
	height = abMax(height, 1u);
	RECT rect = { .left = 0, .top = 0, .right = (LONG) width, .bottom = (LONG) height };
	AdjustWindowRect(&rect, style, FALSE);
	int adjustedWidth = rect.right - rect.left, adjustedHeight = rect.bottom - rect.top;
	abPlatformi_WindowsDesktop_Window_HWnd = CreateWindowW(classDesc.lpszClassName, L"31", style,
			(GetSystemMetrics(SM_CXSCREEN) - adjustedWidth) >> 1u, (GetSystemMetrics(SM_CYSCREEN) - adjustedHeight) >> 1u,
			adjustedWidth, adjustedHeight, abNull, abNull, classDesc.hInstance, abNull);
	if (abPlatformi_WindowsDesktop_Window_HWnd == abNull) {
		return abFalse;
	}
	ShowWindow(abPlatformi_WindowsDesktop_Window_HWnd, SW_SHOWNORMAL);
	return abTrue;
}

abBool abPlatform_Window_GetSize(unsigned int * width, unsigned int * height) {
	if (abPlatformi_WindowsDesktop_Window_HWnd == abNull) {
		return abFalse;
	}
	RECT rect;
	if (!GetClientRect(abPlatformi_WindowsDesktop_Window_HWnd, &rect)) {
		return abFalse;
	}
	int rectWidth = rect.right - rect.left, rectHeight = rect.bottom - rect.top;
	if (rectWidth <= 0 || rectHeight <= 0) {
		return abFalse;
	}
	if (width != abNull) {
		*width = (unsigned int) rectWidth;
	}
	if (height != abNull) {
		*height = (unsigned int) rectHeight;
	}
	return abTrue;
}

abBool abPlatform_Window_InitGPUDisplayChain(struct abGPU_DisplayChain * chain, abTextU8 const * name,
		unsigned int imageCount, enum abGPU_Image_Format format, unsigned int width, unsigned int height) {
	if (abPlatformi_WindowsDesktop_Window_HWnd == abNull) {
		return abFalse;
	}
	return abGPU_DisplayChain_InitForWindowsHWnd(chain, name,
			abPlatformi_WindowsDesktop_Window_HWnd, imageCount, format, width, height);
}

void abPlatform_Window_ProcessEvents() {
	MSG message;
	while (PeekMessageW(&message, abNull, 0u, 0u, PM_NOREMOVE)) {
		BOOL gotMessage = GetMessageW(&message, abNull, 0u, 0u);
		if (gotMessage == (BOOL) -1) { // Error in GetMessage.
			continue;
		}
		if (!gotMessage) { // Got WM_QUIT.
			abCore_RequestQuit(abFalse);
		}
		TranslateMessage(&message);
		DispatchMessageW(&message);
	}
}

void abPlatform_Window_Shutdown() {
	if (abPlatformi_WindowsDesktop_Window_HWnd != abNull) {
		DestroyWindow(abPlatformi_WindowsDesktop_Window_HWnd);
		abPlatformi_WindowsDesktop_Window_HWnd = abNull;
	}
}

void abPlatformi_OS_Windows_Init();

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow) {
	abPlatformi_OS_Windows_Init();
	abCore_Run();
	return 0;
}

#endif

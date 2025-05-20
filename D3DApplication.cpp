#include "D3DApplication.h"
#include "InjectorUI.h"
#include "TitleBarUI.h"

#include <imgui.h>
#include <windowsx.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx11.h>
#include <cstdio>    // Required for sprintf_s
#include <debugapi.h> // Required for OutputDebugStringW

#pragma comment(lib, "d3d11.lib")

D3DApplication* D3DApplication::Instance = nullptr;

// Forward declaration for the global window procedure
LRESULT WINAPI GlobalWindowProcess(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

D3DApplication::D3DApplication(HINSTANCE appInstance) : ApplicationInstance(appInstance) {
	Instance = this;
	OutputDebugStringW(L"D3DApplication constructor called.\n");
}

D3DApplication::~D3DApplication() {
	OutputDebugStringW(L"D3DApplication destructor called.\n");
}

bool D3DApplication::Initialize(int PositionX, int PositionY, int Width, int Height) {
	WCHAR debugMsg[512];
	swprintf_s(debugMsg, L"D3DApplication::Initialize started. HINSTANCE: %p\n", (void*)ApplicationInstance);
	OutputDebugStringW(debugMsg);

	const WCHAR* uniqueClassName = L"MyShadowBindAppWindowClass_A8C7B1F3";

	WNDCLASSEXW WindowClass = {};
	WindowClass.cbSize = sizeof(WNDCLASSEXW);
	WindowClass.style = CS_CLASSDC;
	WindowClass.lpfnWndProc = GlobalWindowProcess; // Correctly uses the global forward-declared one
	WindowClass.cbClsExtra = 0;
	WindowClass.cbWndExtra = 0;
	WindowClass.hInstance = ApplicationInstance;
	WindowClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	WindowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	WindowClass.hbrBackground = nullptr;
	WindowClass.lpszMenuName = nullptr;
	WindowClass.lpszClassName = uniqueClassName;
	WindowClass.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

	swprintf_s(debugMsg, L"Attempting to register window class: %s with HINSTANCE: %p\n", uniqueClassName, (void*)WindowClass.hInstance);
	OutputDebugStringW(debugMsg);

	if (!RegisterClassExW(&WindowClass)) {
		DWORD errorCode = GetLastError();
		swprintf_s(debugMsg, L"!!! RegisterClassExW FAILED! Error Code: %lu !!!\n", errorCode);
		OutputDebugStringW(debugMsg);
		char errorMsgA[256];
		sprintf_s(errorMsgA, sizeof(errorMsgA), "RegisterClassExW FAILED with error code: %lu. Check Debug Output.", errorCode);
		MessageBoxA(nullptr, errorMsgA, "CRITICAL Initialization Error (RegisterClassExW)", MB_OK | MB_ICONERROR);
		return false;
	} else {
		OutputDebugStringW(L"RegisterClassExW SUCCEEDED.\n");
	}

	swprintf_s(debugMsg, L"Attempting to create window with class: %s and HINSTANCE: %p\n", uniqueClassName, (void*)ApplicationInstance);
	OutputDebugStringW(debugMsg);

    // WindowHandle (member) is assigned HERE. Before this line, it's null.
	WindowHandle = CreateWindowExW(
		0,
		uniqueClassName,
		L"Shadow Bind DLL Injector",
		WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX),
		PositionX, PositionY, Width, Height,
		nullptr, nullptr,
		ApplicationInstance,
		nullptr // No lpParam needed for WM_CREATE unless you want to pass 'this'
	);

	if (!WindowHandle) {
		DWORD errorCode = GetLastError();
		swprintf_s(debugMsg, L"!!! CreateWindowExW FAILED! Error Code: %lu !!!\n", errorCode);
		OutputDebugStringW(debugMsg);
		char errorMsgA[256];
		sprintf_s(errorMsgA, sizeof(errorMsgA), "CreateWindowExW FAILED with error code: %lu. Check Debug Output.", errorCode);
		MessageBoxA(nullptr, errorMsgA, "CRITICAL Initialization Error (CreateWindowExW)", MB_OK | MB_ICONERROR);
		UnregisterClassW(uniqueClassName, ApplicationInstance);
		OutputDebugStringW(L"Unregistered class after CreateWindowExW failure.\n");
		return false;
	} else {
		// Now WindowHandle (member) is valid.
		swprintf_s(debugMsg, L"CreateWindowExW SUCCEEDED. Member HWND this->WindowHandle: %p\n", (void*)this->WindowHandle);
		OutputDebugStringW(debugMsg);
	}

	SetWindowPos(this->WindowHandle, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);

	if (!CreateDeviceAndSwapChain()) { // Uses this->WindowHandle, which is now valid
		OutputDebugStringW(L"CreateDeviceAndSwapChain failed.\n");
		DestroyWindow(this->WindowHandle);
		UnregisterClassW(uniqueClassName, ApplicationInstance);
		return false;
	}
	OutputDebugStringW(L"CreateDeviceAndSwapChain succeeded.\n");

	ShowWindow(this->WindowHandle, SW_SHOWDEFAULT); // Uses this->WindowHandle
	UpdateWindow(this->WindowHandle);             // Uses this->WindowHandle

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(this->WindowHandle); // Uses this->WindowHandle, which is valid
	ImGui_ImplDX11_Init(Device, DeviceContext);
	InjectorPanel = new InjectorUI(this->WindowHandle); // Passes valid HWND
	TitleBar = new TitleBarUI(this->WindowHandle);     // Passes valid HWND
	OutputDebugStringW(L"ImGui initialized and UI panels created.\n");
	return true;
}

void D3DApplication::RunMessageLoop() {
	OutputDebugStringW(L"RunMessageLoop starting.\n");
	MSG Message = {};
	while (Message.message != WM_QUIT) {
		if (PeekMessageW(&Message, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&Message);
			DispatchMessageW(&Message);
		} else {
			ImGui_ImplDX11_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			if (TitleBar) TitleBar->Render();
			if (InjectorPanel) InjectorPanel->Render();

			ImGui::Render();
			const float ClearColor[4] = { 0.1f, 0.105f, 0.11f, 1.0f };
			DeviceContext->OMSetRenderTargets(1, &RenderTargetView, nullptr);
			DeviceContext->ClearRenderTargetView(RenderTargetView, ClearColor);
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
			SwapChain->Present(1, 0);
		}
	}
	OutputDebugStringW(L"RunMessageLoop finished.\n");
}

void D3DApplication::Shutdown() {
	OutputDebugStringW(L"Shutdown initiated.\n");
	delete InjectorPanel;
	InjectorPanel = nullptr;
	delete TitleBar;
	TitleBar = nullptr;

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	OutputDebugStringW(L"ImGui shutdown.\n");

	DestroyDeviceAndSwapChain(); // Uses member variables
	OutputDebugStringW(L"Device and SwapChain destroyed.\n");

	if (this->WindowHandle) { // Check member variable
		DestroyWindow(this->WindowHandle);
		WCHAR debugMsg[128];
		swprintf_s(debugMsg, L"Window destroyed: %p\n", (void*)this->WindowHandle);
		OutputDebugStringW(debugMsg);
		this->WindowHandle = nullptr;
	}
	
	const WCHAR* uniqueClassName = L"MyShadowBindAppWindowClass_A8C7B1F3";
	if (ApplicationInstance) { // Check member variable
		if (UnregisterClassW(uniqueClassName, ApplicationInstance)) {
			OutputDebugStringW(L"Window class unregistered successfully.\n");
		} else {
			DWORD lastError = GetLastError();
			WCHAR debugMsg[256];
			swprintf_s(debugMsg, L"UnregisterClassW failed or class was not registered. Error: %lu\n", lastError);
			OutputDebugStringW(debugMsg);
		}
	}
	ApplicationInstance = nullptr;
}

bool D3DApplication::CreateDeviceAndSwapChain() {
    // This function uses this->WindowHandle, which is assumed to be valid
    // if Initialize() has progressed past CreateWindowExW successfully.
	DXGI_SWAP_CHAIN_DESC SwapChainDescription = {};
	SwapChainDescription.BufferCount = 2;
	SwapChainDescription.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	SwapChainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDescription.OutputWindow = this->WindowHandle; // Correct use of member after it's set
	SwapChainDescription.SampleDesc.Count = 1;
	SwapChainDescription.Windowed = TRUE;
	SwapChainDescription.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	UINT CreationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
	CreationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL FeatureLevel;
	HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, CreationFlags, nullptr, 0, D3D11_SDK_VERSION, &SwapChainDescription, &SwapChain, &Device, &FeatureLevel, &DeviceContext);
	if (FAILED(hr)) {
		char errorMsg[256];
		sprintf_s(errorMsg, sizeof(errorMsg), "D3D11CreateDeviceAndSwapChain failed with HRESULT: 0x%08X", static_cast<unsigned int>(hr));
		MessageBoxA(nullptr, errorMsg, "D3D Initialization Error", MB_OK | MB_ICONERROR);
		OutputDebugStringA("D3D11CreateDeviceAndSwapChain failed.\n");
		return false;
	}

	ID3D11Texture2D* BackBuffer = nullptr;
	hr = SwapChain->GetBuffer(0, IID_PPV_ARGS(&BackBuffer));
	if (FAILED(hr)) {
		char errorMsg[256];
		sprintf_s(errorMsg, sizeof(errorMsg), "SwapChain->GetBuffer failed with HRESULT: 0x%08X", static_cast<unsigned int>(hr));
		MessageBoxA(nullptr, errorMsg, "D3D Initialization Error", MB_OK | MB_ICONERROR);
		OutputDebugStringA("SwapChain->GetBuffer failed.\n");
		if (BackBuffer) BackBuffer->Release();
		return false;
	}

	hr = Device->CreateRenderTargetView(BackBuffer, nullptr, &RenderTargetView);
	BackBuffer->Release(); 
	if (FAILED(hr)) {
		char errorMsg[256];
		sprintf_s(errorMsg, sizeof(errorMsg), "Device->CreateRenderTargetView failed with HRESULT: 0x%08X", static_cast<unsigned int>(hr));
		MessageBoxA(nullptr, errorMsg, "D3D Initialization Error", MB_OK | MB_ICONERROR);
		OutputDebugStringA("Device->CreateRenderTargetView failed.\n");
		return false;
	}
	return true;
}

void D3DApplication::DestroyDeviceAndSwapChain() {
	if (RenderTargetView) {
		RenderTargetView->Release();
		RenderTargetView = nullptr;
	}
	if (SwapChain) {
		SwapChain->Release();
		SwapChain = nullptr;
	}
	if (DeviceContext) {
		DeviceContext->Release();
		DeviceContext = nullptr;
	}
	if (Device) {
		Device->Release();
		Device = nullptr;
	}
}

// Modified to take HWND as parameter
LRESULT D3DApplication::HandleWindowMessage(HWND hwnd, UINT Message, WPARAM WParam, LPARAM LParam) {
    // Use the passed-in 'hwnd' for ImGui and DefWindowProcW,
    // as this->WindowHandle might not be set yet during WM_CREATE etc.
	if (ImGui_ImplWin32_WndProcHandler(hwnd, Message, WParam, LParam)) {
		return true;
	}

	WCHAR debugMsg[256]; // For debugging specific messages if needed
	switch (Message) {
		case WM_CREATE:
			// hwnd here is the valid window handle. this->WindowHandle is NOT YET SET.
			swprintf_s(debugMsg, L"HandleWindowMessage: WM_CREATE received for HWND: %p. this->WindowHandle: %p\n", (void*)hwnd, (void*)this->WindowHandle);
			OutputDebugStringW(debugMsg);
			break;
		case WM_NCCREATE:
			// hwnd here is the valid window handle. this->WindowHandle is NOT YET SET.
			swprintf_s(debugMsg, L"HandleWindowMessage: WM_NCCREATE received for HWND: %p. this->WindowHandle: %p\n", (void*)hwnd, (void*)this->WindowHandle);
			OutputDebugStringW(debugMsg);
			break;
		case WM_SIZE: {
            // For WM_SIZE, this->WindowHandle should be valid if it's not SIZE_MINIMIZED during init.
            // However, using 'hwnd' is safer and consistent.
			if (Device && WParam != SIZE_MINIMIZED) {
				if (RenderTargetView) {
					RenderTargetView->Release();
					RenderTargetView = nullptr;
				}
				// Ensure hwnd is used if operations here are critical before this->WindowHandle is set, though less likely for WM_SIZE.
				HRESULT hr = SwapChain->ResizeBuffers(0, static_cast<UINT>(LOWORD(LParam)), static_cast<UINT>(HIWORD(LParam)), DXGI_FORMAT_UNKNOWN, 0);
				if (SUCCEEDED(hr)) {
					ID3D11Texture2D* BackBuffer = nullptr;
					hr = SwapChain->GetBuffer(0, IID_PPV_ARGS(&BackBuffer));
					if (SUCCEEDED(hr)) {
						if (BackBuffer) { 
							Device->CreateRenderTargetView(BackBuffer, nullptr, &RenderTargetView);
							BackBuffer->Release();
						}
					}
				}
			}
			return 0;
		}
		case WM_SYSCOMMAND: {
			if ((WParam & 0xFFF0) == SC_KEYMENU) {
				return 0;
			}
			break; // Important: Fall through to DefWindowProcW for other SC_ commands
		}
		case WM_NCHITTEST: {
			POINT CursorPosition { GET_X_LPARAM(LParam), GET_Y_LPARAM(LParam) };
			ScreenToClient(hwnd, &CursorPosition); // Use passed-in hwnd

			RECT WindowRectangle = {};
			GetClientRect(hwnd, &WindowRectangle); // Use passed-in hwnd
			const LONG Border = 8;

			bool LeftBorder = CursorPosition.x < Border;
			bool RightBorder = CursorPosition.x >= WindowRectangle.right - Border;
			bool TopBorder = CursorPosition.y < Border;
			bool BottomBorder = CursorPosition.y >= WindowRectangle.bottom - Border;

			if (TopBorder && LeftBorder) return HTTOPLEFT;
			if (TopBorder && RightBorder) return HTTOPRIGHT;
			if (BottomBorder && LeftBorder) return HTBOTTOMLEFT;
			if (BottomBorder && RightBorder) return HTBOTTOMRIGHT;
			if (LeftBorder) return HTLEFT;
			if (RightBorder) return HTRIGHT;
			if (TopBorder) return HTTOP;
			if (BottomBorder) return HTBOTTOM;
			
            // TitleBar uses OwnerHWND which is this->WindowHandle.
            // This is fine because WM_NCHITTEST occurs after window creation.
            if (TitleBar && TitleBar->IsMouseOverTitleBar(CursorPosition.x, CursorPosition.y)) {
                 return HTCAPTION;
            }
			return HTCLIENT;
		}
		case WM_DESTROY: {
			OutputDebugStringW(L"WM_DESTROY received.\n");
			PostQuitMessage(0);
			return 0;
		}
	}
	// CRITICAL: Use the 'hwnd' parameter from the window procedure
	return DefWindowProcW(hwnd, Message, WParam, LParam);
}

// Global Window Procedure
LRESULT WINAPI GlobalWindowProcess(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	// During WM_NCCREATE and WM_CREATE, D3DApplication::Instance->WindowHandle is still NULL.
	// But 'hwnd' parameter *is* the valid handle for the window being created.
	if (D3DApplication::Instance) {
        // Pass the 'hwnd' received by this WndProc to the class instance's handler
		return D3DApplication::Instance->HandleWindowMessage(hwnd, msg, wParam, lParam);
	}
    // If instance is null for some reason, fall back to DefWindowProcW with the current hwnd.
	return DefWindowProcW(hwnd, msg, wParam, lParam);
}
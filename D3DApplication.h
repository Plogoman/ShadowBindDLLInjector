#pragma once
#include <Windows.h>
#include <d3d11.h>

struct InjectorUI;
struct TitleBarUI;

struct D3DApplication {
private:
	HINSTANCE ApplicationInstance = nullptr;
	HWND WindowHandle = nullptr; // This will be set AFTER CreateWindowEx returns
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* DeviceContext = nullptr;
	IDXGISwapChain* SwapChain = nullptr;
	ID3D11RenderTargetView* RenderTargetView = nullptr;

	InjectorUI* InjectorPanel = nullptr;
	TitleBarUI* TitleBar = nullptr;

	bool CreateDeviceAndSwapChain();
	void DestroyDeviceAndSwapChain();

public:
	explicit D3DApplication(HINSTANCE ApplicationInstance);
	~D3DApplication();

	bool Initialize(int PositionX, int PositionY, int Width, int Height);
	void RunMessageLoop();
	void Shutdown();

	// Modified to accept HWND
	LRESULT HandleWindowMessage(HWND hwnd, UINT Message, WPARAM WParam, LPARAM LParam);
	static D3DApplication* Instance;
};
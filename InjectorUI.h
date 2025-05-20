#pragma once
#include <imgui.h>
#include <Windows.h>
#include <vector>
#include <string> // Required for std::wstring

extern IMGUI_IMPL_API LRESULT
	ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

template<typename T>
using DynamicArray = std::vector<T>;

// Structure to hold process information
struct ProcessInfo {
	DWORD pid;
	std::wstring processName; // Using wstring to correctly store TCHAR/WCHAR names
};

struct InjectorUI {
private:
	HWND OwnerHWND;
	char DLLPathBuffer[256];
	char PIDInputBuffer[16];          // Stores the selected PID as a string
	DynamicArray<ProcessInfo> ProcessInfoList; // Stores list of processes with names and PIDs

	static static bool InjectUsingRemoteThread(const char* DLLPath, DWORD TargetProcessID);
	void RefreshProcessIDList();

public:
	explicit InjectorUI(HWND OwnerHWND);
	~InjectorUI();

	void Render();
};
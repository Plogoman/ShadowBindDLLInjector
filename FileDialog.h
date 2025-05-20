#pragma once

#include <Windows.h>

struct FileDialog {
	static bool SelectDLL(HWND OwnerHWND, char* OutFilePath, DWORD MaxFilePathLength);
};

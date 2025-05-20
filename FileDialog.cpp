#include "FileDialog.h"
#include <commdlg.h>

#pragma comment(lib, "comdlg32.lib")

bool FileDialog::SelectDLL(HWND OwnerHWND, char *OutFilePath, DWORD MaxFilePathLength) {
	OPENFILENAMEA OpenFileName = {};
	OpenFileName.lStructSize = sizeof(OpenFileName);
	OpenFileName.hwndOwner = OwnerHWND;
	OpenFileName.lpstrFile = OutFilePath;
	OpenFileName.nMaxFile = MaxFilePathLength;
	OpenFileName.lpstrFilter = "DLL Files\0*.dll\0All Files\0*.*\0";
	OpenFileName.lpstrTitle = "Select DLL to Inject";
	OpenFileName.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
	return GetOpenFileNameA(&OpenFileName) == TRUE;
}

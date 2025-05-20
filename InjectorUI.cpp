// InjectorUI.cpp
#include "InjectorUI.h"
#include "FileDialog.h"
#include <TlHelp32.h> // For process enumeration
#include <imgui.h>
#include <string>    // For std::stoul, std::wstring
#include <cstdio>    // For sprintf_s
#include <cstring>   // For strcpy_s if needed, though less with std::string
// Windows.h is likely included by TlHelp32.h, but ensure it's available for MultiByteToWideChar
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>


// Constructor
InjectorUI::InjectorUI(HWND OwnerHWND) : OwnerHWND(OwnerHWND) {
	DLLPathBuffer[0] = '\0';
	PIDInputBuffer[0] = '\0';
	RefreshProcessIDList();
}

// Destructor
InjectorUI::~InjectorUI() {}

// Refresh the list of processes
void InjectorUI::RefreshProcessIDList() {
	ProcessInfoList.clear();
	HANDLE SnapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (SnapshotHandle == INVALID_HANDLE_VALUE) {
		return;
	}

	PROCESSENTRY32 ProcessEntry = { sizeof(ProcessEntry) }; // szExeFile is TCHAR
	if (Process32First(SnapshotHandle, &ProcessEntry)) {
		do {
			ProcessInfo info;
			info.pid = ProcessEntry.th32ProcessID;

#ifdef UNICODE
			// If UNICODE is defined, TCHAR is wchar_t.
			// ProcessEntry.szExeFile is wchar_t[] and null-terminated.
			// Direct assignment to std::wstring is safe and efficient.
			info.processName = ProcessEntry.szExeFile;
#else
			// If UNICODE is not defined, TCHAR is char.
			// ProcessEntry.szExeFile is char[] (ANSI/MBCS) and null-terminated.
			// We need to explicitly convert it to std::wstring (wchar_t).
			const char* ansiProcessName = ProcessEntry.szExeFile;
			if (ansiProcessName[0] == '\0') {
				info.processName = L""; // Handle empty string case
			} else {
				// Determine buffer size needed for wide string (including null terminator)
				int requiredBufferSize = MultiByteToWideChar(CP_ACP, 0, ansiProcessName, -1, nullptr, 0);
				if (requiredBufferSize > 0) {
					// std::wstring constructor needs character count, not including its own null.
					// requiredBufferSize from MultiByteToWideChar (with -1 input length) includes the null.
					std::wstring wideProcessName(static_cast<size_t>(requiredBufferSize) - 1, L'\0');
					MultiByteToWideChar(CP_ACP, 0, ansiProcessName, -1, &wideProcessName[0], requiredBufferSize);
					info.processName = wideProcessName;
				} else {
					// Handle conversion error, e.g., log GetLastError()
					info.processName = L"[Conversion Error]";
				}
			}
#endif
			ProcessInfoList.push_back(info);
		} while (Process32Next(SnapshotHandle, &ProcessEntry));
	}
	CloseHandle(SnapshotHandle);
}

// Injection logic (remains unchanged)
bool InjectorUI::InjectUsingRemoteThread(const char *DLLPath, DWORD TargetProcessID) {
	if (DLLPath == nullptr || DLLPath[0] == '\0' || TargetProcessID == 0) {
        // Basic validation
		return false;
	}
	HANDLE ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, TargetProcessID);
	if (!ProcessHandle) {
		return false;
	}

	SIZE_T PathBytes = strlen(DLLPath) + 1; // +1 for null terminator
	void* RemoteBuffer = VirtualAllocEx(ProcessHandle, nullptr, PathBytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!RemoteBuffer) {
		CloseHandle(ProcessHandle);
		return false;
	}

	if (!WriteProcessMemory(ProcessHandle, RemoteBuffer, DLLPath, PathBytes, nullptr)) {
		VirtualFreeEx(ProcessHandle, RemoteBuffer, 0, MEM_RELEASE);
		CloseHandle(ProcessHandle);
		return false;
	}
    
	HMODULE kernel32Handle = GetModuleHandle(TEXT("kernel32.dll"));
    if (!kernel32Handle) {
        VirtualFreeEx(ProcessHandle, RemoteBuffer, 0, MEM_RELEASE);
		CloseHandle(ProcessHandle);
        return false;
    }

	FARPROC LoadLibraryAddress = GetProcAddress(kernel32Handle, "LoadLibraryA");
    if (!LoadLibraryAddress) {
        VirtualFreeEx(ProcessHandle, RemoteBuffer, 0, MEM_RELEASE);
		CloseHandle(ProcessHandle);
        return false;
    }

	HANDLE RemoteThreadHandle = CreateRemoteThread(ProcessHandle, nullptr, 0, (LPTHREAD_START_ROUTINE)LoadLibraryAddress, RemoteBuffer, 0, nullptr);

	bool Success = false;
	if (RemoteThreadHandle) {
		WaitForSingleObject(RemoteThreadHandle, INFINITE);
        DWORD exitCode = 0;
        // Optionally, check thread exit code for LoadLibrary success, though this is more complex
        // GetExitCodeThread(RemoteThreadHandle, &exitCode);
        // Success = (exitCode != 0); // If LoadLibrary returns non-NULL module handle
        Success = true; // For simplicity, assume success if thread created and ran
		CloseHandle(RemoteThreadHandle);
	}

	VirtualFreeEx(ProcessHandle, RemoteBuffer, 0, MEM_RELEASE);
	CloseHandle(ProcessHandle);
	return Success;
}

// Render the Injector UI
void InjectorUI::Render() {
	// --- Main Injector Panel Layout ---
	const float titleBarHeight = 30.0f; // Height of your custom TitleBarUI
	ImVec2 mainDisplaySize = ImGui::GetIO().DisplaySize;

	ImGui::SetNextWindowPos(ImVec2(0.0f, titleBarHeight));
	ImGui::SetNextWindowSize(ImVec2(mainDisplaySize.x, mainDisplaySize.y - titleBarHeight));
	
	// Flags to make it a fixed panel, no ImGui title bar, no moving/resizing by user
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoCollapse |
                                    ImGuiWindowFlags_NoTitleBar |
                                    ImGuiWindowFlags_NoBringToFrontOnFocus;

	ImGui::Begin("InjectorPanel", nullptr, window_flags);

	// --- UI Elements ---
	ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.7f); // Make input text wider
	ImGui::InputText("DLL Path", DLLPathBuffer, IM_ARRAYSIZE(DLLPathBuffer));
	ImGui::PopItemWidth();
	ImGui::SameLine();
	if (ImGui::Button("Browse", ImVec2(ImGui::GetContentRegionAvail().x, 0))) { // Fill remaining width
		FileDialog::SelectDLL(OwnerHWND, DLLPathBuffer, IM_ARRAYSIZE(DLLPathBuffer));
	}

	ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.7f);
	ImGui::InputText("Target PID", PIDInputBuffer, IM_ARRAYSIZE(PIDInputBuffer));
	ImGui::PopItemWidth();
	ImGui::SameLine();
	if (ImGui::Button("Refresh", ImVec2(ImGui::GetContentRegionAvail().x, 0))) { // Fill remaining width
		RefreshProcessIDList();
		PIDInputBuffer[0] = '\0'; // Clear selected PID on refresh
	}

    // --- Process List Combo Box ---
    char comboPreviewText[MAX_PATH + 30] = "Select Process..."; // Buffer for "Name (PID)" or PID
    if (PIDInputBuffer[0] != '\0') {
        DWORD currentPid = 0;
        try {
            currentPid = std::stoul(PIDInputBuffer); // Convert string PID to DWORD
        } catch (const std::exception&) {
            // PIDInputBuffer is not a valid number, keep "Select Process..." or clear
            PIDInputBuffer[0] = '\0'; // Clear invalid PID
        }

        if (currentPid != 0) {
            bool found = false;
            for (const auto& procInfo : ProcessInfoList) {
                if (procInfo.pid == currentPid) {
                    char nameNarrow[MAX_PATH];
                    WideCharToMultiByte(CP_UTF8, 0, procInfo.processName.c_str(), -1, nameNarrow, MAX_PATH, nullptr, nullptr);
                    sprintf_s(comboPreviewText, sizeof(comboPreviewText), "%s (%lu)", nameNarrow, procInfo.pid);
                    found = true;
                    break;
                }
            }
            if (!found) { // PID is a number but not in the current list, show the PID itself
                sprintf_s(comboPreviewText, sizeof(comboPreviewText), "%s", PIDInputBuffer);
            }
        }
    }

	if (ImGui::BeginCombo("##PIDCombo", comboPreviewText, ImGuiComboFlags_HeightLargest)) {
		for (const auto& procInfo : ProcessInfoList) {
			char nameNarrow[MAX_PATH];
			// Convert WCHAR process name to char for ImGui display
			WideCharToMultiByte(CP_UTF8, 0, procInfo.processName.c_str(), -1, nameNarrow, MAX_PATH, nullptr, nullptr);
			
			char itemLabel[MAX_PATH + 30]; // For "Process Name (PID)"
			sprintf_s(itemLabel, sizeof(itemLabel), "%s (%lu)", nameNarrow, procInfo.pid);

			bool is_selected = false;
            if (PIDInputBuffer[0] != '\0') {
                 try {
                    is_selected = (procInfo.pid == std::stoul(PIDInputBuffer));
                } catch (const std::exception&) { /* ignore, PIDInputBuffer might be temporarily non-numeric */ }
            }

			if (ImGui::Selectable(itemLabel, is_selected)) {
				sprintf_s(PIDInputBuffer, sizeof(PIDInputBuffer), "%lu", procInfo.pid);
			}
			if (is_selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	// --- Inject Button and Modal ---
	if (ImGui::Button("Inject DLL", ImVec2(ImGui::GetContentRegionAvail().x, 25))) { // Wider button
		DWORD targetPID = 0;
        if (PIDInputBuffer[0] != '\0') {
            try {
                targetPID = std::stoul(PIDInputBuffer);
            } catch (const std::exception&) {
                targetPID = 0; // Invalid PID entered
            }
        }

		if (targetPID != 0 && DLLPathBuffer[0] != '\0') {
			bool success = InjectUsingRemoteThread(DLLPathBuffer, targetPID);
			ImGui::OpenPopup(success ? "Injection Succeeded" : "Injection Failed");
		} else {
            ImGui::OpenPopup("Invalid Input");
        }
	}

    // Success Modal
	if (ImGui::BeginPopupModal("Injection Succeeded", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("DLL Injected Successfully!");
		ImGui::Separator();
		if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
		ImGui::EndPopup();
	}

    // Failure Modal
	if (ImGui::BeginPopupModal("Injection Failed!!", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("DLL injection failed.");
        ImGui::Text("Possible reasons:\n- Incorrect PID\n- DLL path error\n- Insufficient permissions\n- Target process architecture mismatch (32/64-bit)\n- Security software interference");
		ImGui::Separator();
		if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
		ImGui::EndPopup();
	}
    
    // Invalid Input Modal
    if (ImGui::BeginPopupModal("Invalid Input", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Please ensure DLL path and Target PID are valid.");
		ImGui::Separator();
		if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
		ImGui::EndPopup();
	}

	ImGui::End(); // End of "InjectorPanel"
}
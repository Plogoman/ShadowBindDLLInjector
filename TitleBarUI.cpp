#include "TitleBarUI.h"
#include <imgui.h>
#include <tchar.h> // For _T macro if you use it, though not strictly needed for ImGui text

TitleBarUI::TitleBarUI(HWND OwnerHWND) : OwnerHWND(OwnerHWND) {}

TitleBarUI::~TitleBarUI() = default;

void TitleBarUI::Render() {
    // Define the height of your custom title bar
    const float titleBarHeight = 30.0f; // Adjust as needed

    // Get the main viewport's width to make the title bar span the window
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 windowPos = viewport->Pos; // Top-left of the main window
    ImVec2 windowSize = viewport->Size; // Size of the main window

    ImGui::SetNextWindowPos(ImVec2(windowPos.x, windowPos.y));
    ImGui::SetNextWindowSize(ImVec2(windowSize.x, titleBarHeight));
    
    ImGui::Begin("TitleBar", nullptr, 
                 ImGuiWindowFlags_NoDecoration | 
                 ImGuiWindowFlags_NoMove | 
                 ImGuiWindowFlags_NoScrollWithMouse | 
                 ImGuiWindowFlags_NoSavedSettings |
                 ImGuiWindowFlags_NoBringToFrontOnFocus);

    // Store the client coordinates of this ImGui window.
    // Note: ImGui window coordinates are screen coordinates.
    // We need to convert them or ensure WM_NCHITTEST coordinates are consistent.
    // For simplicity, WM_NCHITTEST gives screen coordinates which are then converted to client.
    // So, the coordinates stored here should ideally be client coordinates relative to the main HWND.

    // Get the position of the ImGui window (which is our title bar)
    ImVec2 titleBarWindowPos = ImGui::GetWindowPos(); // Screen coordinates
    ImVec2 titleBarWindowSize = ImGui::GetWindowSize();

    // Convert screen coordinates of the title bar to client coordinates of the main HWND
    POINT titleBarTopLeftScreen = { (LONG)titleBarWindowPos.x, (LONG)titleBarWindowPos.y };
    ScreenToClient(OwnerHWND, &titleBarTopLeftScreen);

    TitleBarMinX = (float)titleBarTopLeftScreen.x;
    TitleBarMinY = (float)titleBarTopLeftScreen.y;
    TitleBarMaxX = TitleBarMinX + titleBarWindowSize.x;
    TitleBarMaxY = TitleBarMinY + titleBarWindowSize.y; // The entire ImGui window is the draggable area

    ImGui::TextUnformatted("Shadow Bind"); // Using TextUnformatted for basic text

    // Buttons - adjust positioning as needed
    float buttonWidth = 60.0f;
    float buttonHeight = titleBarHeight - 4.0f; // Slightly smaller than title bar
    float spacing = 5.0f;

    ImGui::SameLine(ImGui::GetWindowWidth() - (buttonWidth * 2) - (spacing * 2) - 10.0f); // Position buttons from the right
    if (ImGui::Button("Minimize", ImVec2(buttonWidth, buttonHeight))) {
        ShowWindow(OwnerHWND, SW_MINIMIZE);
    }
    ImGui::SameLine(ImGui::GetWindowWidth() - buttonWidth - spacing - 10.0f );
    if (ImGui::Button("Close", ImVec2(buttonWidth, buttonHeight))) {
        PostMessage(OwnerHWND, WM_CLOSE, 0, 0);
    }
    ImGui::End();
}

bool TitleBarUI::IsMouseOverTitleBar(int ClientX, int ClientY) const {
    // Check if the provided client coordinates are within the stored bounds
    return (static_cast<float>(ClientX) >= TitleBarMinX &&
            static_cast<float>(ClientX) <= TitleBarMaxX &&
            static_cast<float>(ClientY) >= TitleBarMinY &&
            static_cast<float>(ClientY) <= TitleBarMaxY);
}
#pragma once
#include <Windows.h>

struct TitleBarUI {
private:
    HWND OwnerHWND;
    // Store the client coordinates of the draggable title bar area
    float TitleBarMinX = 0.0f;
    float TitleBarMinY = 0.0f;
    float TitleBarMaxX = 0.0f;
    float TitleBarMaxY = 0.0f; // This will define the height of the draggable area

public:
    explicit TitleBarUI(HWND OwnerHWND);
    ~TitleBarUI();

    void Render();
    // Method to check if client coordinates are over the title bar
    bool IsMouseOverTitleBar(int ClientX, int ClientY) const;
};
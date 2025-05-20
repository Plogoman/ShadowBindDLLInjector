#include "D3DApplication.h"

int WINAPI WinMain(HINSTANCE ApplicationInstance, HINSTANCE, LPSTR, int) {
	D3DApplication Application(ApplicationInstance);
	if (!Application.Initialize(200, 200, 500, 500)) {
		return -1;
	}

	Application.RunMessageLoop();
	Application.Shutdown();
	return 0;
}

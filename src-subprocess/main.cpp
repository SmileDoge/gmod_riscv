#include "stdio.h"
#include "SubprocessEmulator.h"
#include "simple_device.h"

#ifdef _WIN32
#include "Windows.h"
#endif

int main(int argc, char** argv)
{
	SubprocessEmulator emu(argc, argv);

	emu.RegisterDevice("simple_device", CreateSimpleDevice);

	if (emu.Start())
		emu.LogInfo("Emulator started successfully");
	else
	{
		emu.LogInfo("Failed to start emulator");
		return -1;
	}

	return emu.StartLoop();
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	AllocConsole();

	(void)freopen("CONIN$", "r", stdin);
	(void)freopen("CONOUT$", "w", stdout);
	(void)freopen("CONOUT$", "w", stderr);

	return main(__argc, __argv);
}
#endif
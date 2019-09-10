#include "Application.h"

//int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
int main()
{
	auto& app = Application::Instance();
	if (!app.Initialize())
	{
		return 0;
	}
	app.Run();
	app.Terminate();

	return 0;
}
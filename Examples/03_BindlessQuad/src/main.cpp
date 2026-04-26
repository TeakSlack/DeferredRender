#include "AppLayer.h"

#include <Engine.h>
#include <Window/GLFWWindow.h>
#include <Asset/AssetManager.h>

int main()
{
	GLFWWindowSystem windowSystem;
	AssetManager assetManager;
	AppLayer appLayer;

	Engine::Get().RegisterSubmodule(&windowSystem);
	Engine::Get().RegisterSubmodule(&assetManager);
	Engine::Get().PushLayer(&appLayer);
	Engine::Get().Run();
}
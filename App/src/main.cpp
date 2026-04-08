#include "AppLayer.h"

#include <Engine.h>
#include <Window/GLFWWindow.h>
#include <Asset/AssetSystem.h>

int main()
{
	GLFWWindowSystem windowSystem;
	AssetSystem& assetSystem = AssetSystem::Get();
	AppLayer appLayer(windowSystem);

	Engine::Get().RegisterSubmodule(&windowSystem);
	Engine::Get().RegisterSubmodule(&assetSystem);
	Engine::Get().PushLayer(&appLayer);
	Engine::Get().Run();
}
#include "AppLayer.h"

#include <Engine.h>
#include <Window/GLFWWindow.h>
#include <Asset/AssetManager.h>

int main()
{
	GLFWWindowSystem windowSystem;
	AssetManager assetSystem;
	SceneManager sceneManager;
	SceneRenderer sceneRenderer;
	AppLayer appLayer(windowSystem);

	Engine::Get().RegisterSubmodule(&windowSystem);
	Engine::Get().RegisterSubmodule(&assetSystem);
	Engine::Get().RegisterSubmodule(&sceneManager);
	Engine::Get().RegisterSubmodule(&sceneRenderer);
	Engine::Get().PushLayer(&appLayer);
	Engine::Get().Run();
}
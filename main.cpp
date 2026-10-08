#include "Game/GameApplication.h"
#include "MatchaEngine/Core/WindowConfig.h"

//Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	std::unique_ptr<GameApplication> gameApplication_ = std::make_unique<GameApplication>(
		WindowConfig::kDefaultClientWidth, WindowConfig::kDefaultClientHeight);

	gameApplication_->Run();

	return 0;
}
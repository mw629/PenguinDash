#pragma once
#include <Engine.h>
#include "SceneManager.h"
#include "../Editer/EditorManager.h"


class GameApplication
{
private:

	std::unique_ptr<Engine> engine;
	std::unique_ptr<SceneManager> sceneManager;
	std::unique_ptr<EditorManager> editorManager;

public:
	/// <summary>
	/// ゲームアプリケーションの構築（エンジン、シーンマネージャ、エディタマネージャの生成）
	/// </summary>
	/// <param name="kClientWidth">クライアント領域幅</param>
	/// <param name="kClientHeight">クライアント領域高さ</param>
	GameApplication(int32_t kClientWidth, int32_t kClientHeight);

	/// <summary>
	/// メインループの実行
	/// </summary>
	void Run();
};


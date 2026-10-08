#pragma once
#include "GameScene.h"
#include <memory>
#include "IScene.h"
#include "Direction/FreezeTransition.h"

class SceneManager
{
private:

	std::unique_ptr<IScene> scene_;
	bool isTransitioning_ = false;

public:
	SceneManager();
	~SceneManager() = default;

	void ImGui();

	void Initialize();
	
	void PreUpdate();

	void Update();

	void Draw(class Draw& draw);

	void DrawUI(class Draw& draw);

	void Run(class Draw& draw);

	std::unique_ptr<IScene> CreateScene(int sceneID);

	// 氷結・破砕演出付きのシーン切り替え
	void ChangeScene(int nextSceneID);

};


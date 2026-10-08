#pragma once
#include <Engine.h>
#include <memory>
#include "../IScene.h"
#include "GameObjectManager.h"
#include "Camera.h"
#include "../../Editer/EditorUI.h"

class JsonScene : public IScene
{
private:
	std::unique_ptr<GameObjectManager> gameObjectManager_;
	std::unique_ptr<Camera> camera_;
	std::unique_ptr<EditorUI> editorUI_;
	Matrix4x4 view_;

	bool showInitJsonSelectPopup_ = false;
	std::string selectedJsonFile_ = "scene";
	std::string initialSceneJson_ = "Resources/Json/Scene/scene.json";

public:
	~JsonScene() override = default;

	void ImGui() override;
	void Initialize() override;
	void Update() override;
	void Draw(class Draw& draw) override;
};

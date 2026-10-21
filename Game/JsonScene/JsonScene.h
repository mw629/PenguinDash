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
	/// <summary>
	/// デストラクタ
	/// </summary>
	~JsonScene() override = default;

	/// <summary>
	/// ImGuiデバッグUIの描画処理
	/// </summary>
	void ImGui() override;

	/// <summary>
	/// JSONシーンの初期化
	/// </summary>
	void Initialize() override;

	/// <summary>
	/// 毎フレームの更新処理
	/// </summary>
	void Update() override;

	/// <summary>
	/// 3Dオブジェクトの描画処理
	/// </summary>
	/// <param name="draw">描画マネージャ</param>
	void Draw(class Draw& draw) override;
};

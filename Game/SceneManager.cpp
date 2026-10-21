#include "SceneManager.h"
#include "GameScene.h"
#include "TestScene.h"
#include "JsonScene/JsonScene.h"
#include <Engine.h>
#include "../Editer/EditorManager.h"
#include <imgui.h>

// コンストラクタ
SceneManager::SceneManager()
{
	// 最初のシーンをJsonSceneに設定
	scene_ = std::make_unique<GameScene>();
	Initialize();
}

// シーン管理のImGuiデバッグUI描画
void SceneManager::ImGui() {
	if (scene_) {
		scene_->ImGui();
	}
}

// 初期シーンの生成と初期化
void SceneManager::Initialize() {
	FreezeTransition::GetInstance()->Initialize();
	if (scene_) {
		scene_->Initialize();
	}
}

// 更新前の事前処理
void SceneManager::PreUpdate() {
	auto* transition = FreezeTransition::GetInstance();
	transition->Update();

	if (scene_ && scene_->GetSceneChangeRequest() && !isTransitioning_) {
		int nextScene = scene_->GetNextSceneID();
		ChangeScene(nextScene);
	}
}

// 氷結・破砕演出付きのシーン切り替え
void SceneManager::ChangeScene(int nextSceneID) {
	if (isTransitioning_) return;
	isTransitioning_ = true;

	FreezeTransition::GetInstance()->Start([this, nextSceneID]() {
		scene_ = CreateScene(nextSceneID);
		if (scene_) {
			scene_->Initialize();
		}
		isTransitioning_ = false;
	});
}

// シーンおよび画面遷移の更新処理
void SceneManager::Update() {
	if (scene_) {
		scene_->Update();
	}
}

// 現在のシーンの3Dオブジェクト描画処理
void SceneManager::Draw(class Draw& draw) {
	if (scene_) {
		scene_->Draw(draw);
	}
}

// 現在のシーンの2D UI描画処理
void SceneManager::DrawUI(class Draw& draw) {
	if (scene_) {
		scene_->DrawUI(draw);
	} else {
		// 最前面に氷結・破片トランジションを描画（シーン未ロード時）
		FreezeTransition::GetInstance()->Draw(draw);
	}
}

// 毎フレームの更新と描画の一括実行
void SceneManager::Run(class Draw& draw)
{
	ImGui();
	Update();
	Draw(draw);
}

// 指定IDに応じたシーンインスタンスを生成
std::unique_ptr<IScene> SceneManager::CreateScene(int sceneID)
{
	switch (sceneID) {
	case SceneID::Test:  return std::make_unique<TestScene>();
	case SceneID::Game:  return std::make_unique<GameScene>(); 
	case SceneID::Json:  return std::make_unique<JsonScene>();
	default: return nullptr;
	}
}

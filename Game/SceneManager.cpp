#include "SceneManager.h"
#include "GameScene.h"
#include "TestScene.h"
#include "JsonScene/JsonScene.h"
#include <Engine.h>
#include "../Editer/EditorManager.h"
#include <imgui.h>

SceneManager::SceneManager()
{
	// 最初のシーンをJsonSceneに設定
	scene_ = std::make_unique<GameScene>();
	Initialize();
}

void SceneManager::ImGui() {
	if (scene_) {
		scene_->ImGui();
	}

#ifdef _USE_IMGUI
	if (ImGui::Begin("Scene Transition")) {
		if (ImGui::Button("Test Freeze & Shatter", ImVec2(220, 30))) {
			FreezeTransition::GetInstance()->Start(0.75f, 0.75f, nullptr);
		}
		ImGui::SameLine();
		const char* phaseNames[] = { "None", "Freezing", "Cracking", "Shattering", "Finished" };
		int p = static_cast<int>(FreezeTransition::GetInstance()->GetPhase());
		ImGui::Text("Phase: %s", (p >= 0 && p <= 4) ? phaseNames[p] : "Unknown");
	}
	ImGui::End();
#endif
}

void SceneManager::Initialize() {
	FreezeTransition::GetInstance()->Initialize();
	if (scene_) {
		scene_->Initialize();
	}
}

void SceneManager::PreUpdate() {
	auto* transition = FreezeTransition::GetInstance();
	transition->Update(1.0f / 60.0f);

	if (scene_ && scene_->GetSceneChangeRequest() && !isTransitioning_) {
		int nextScene = scene_->GetNextSceneID();
		ChangeScene(nextScene);
	}
}

void SceneManager::ChangeScene(int nextSceneID) {
	if (isTransitioning_) return;
	isTransitioning_ = true;

	FreezeTransition::GetInstance()->Start(0.75f, 0.75f, [this, nextSceneID]() {
		scene_ = CreateScene(nextSceneID);
		if (scene_) {
			scene_->Initialize();
		}
		isTransitioning_ = false;
	});
}

void SceneManager::Update() {
	if (scene_) {
		scene_->Update();
	}
}

void SceneManager::Draw(class Draw& draw) {
	if (scene_) {
		scene_->Draw(draw);
	}
}

void SceneManager::DrawUI(class Draw& draw) {
	if (scene_) {
		scene_->DrawUI(draw);
	}
	// 最前面に氷結・破片トランジションを描画
	FreezeTransition::GetInstance()->Draw(draw);
}

void SceneManager::Run(class Draw& draw)
{
	ImGui();
	Update();
	Draw(draw);
}

std::unique_ptr<IScene> SceneManager::CreateScene(int sceneID)
{
	switch (sceneID) {
	case SceneID::Test:  return std::make_unique<TestScene>();
	case SceneID::Game:  return std::make_unique<GameScene>(); 
	case SceneID::Json:  return std::make_unique<JsonScene>();
	default: return nullptr;
	}
}

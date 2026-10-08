#include "JsonScene.h"
#include <imgui.h>
#include <filesystem>
#include <algorithm>
#include "../../Editer/EditorManager.h"
#include "AssetManager.h"
#include "Model.h"
#include "RenderObject.h"
#include "Calculation.h"
#include "Draw.h"

void JsonScene::ImGui()
{
#ifdef _USE_IMGUI
	if (showInitJsonSelectPopup_) {
		ImGui::OpenPopup("Select Initial Scene JSON");
		
		ImGuiIO& io = ImGui::GetIO();
		ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		
		if (ImGui::BeginPopupModal("Select Initial Scene JSON", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
			ImGui::Text("Select a JSON file to load for the scene:");
			
			std::vector<std::string> jsonFiles;
			try {
				for (const auto& entry : std::filesystem::directory_iterator("Resources/Json/Scene")) {
					if (entry.is_regular_file() && entry.path().extension() == ".json") {
						jsonFiles.push_back(entry.path().stem().string());
					}
				}
			} catch (...) {}

			if (ImGui::BeginCombo("Scene JSON", selectedJsonFile_.c_str())) {
				for (const auto& file : jsonFiles) {
					bool is_selected = (selectedJsonFile_ == file);
					if (ImGui::Selectable(file.c_str(), is_selected)) {
						selectedJsonFile_ = file;
					}
					if (is_selected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			ImGui::Separator();
			
			if (ImGui::Button("Load & Start", ImVec2(120, 0))) {
				initialSceneJson_ = "Resources/Json/Scene/" + selectedJsonFile_ + ".json";
				EditorManager::SetCurrentFileName(selectedJsonFile_);
				gameObjectManager_->LoadScene(initialSceneJson_);
				showInitJsonSelectPopup_ = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
		
		return;
	}

	ImGui::Begin("JsonScene");
	camera_->ImGui();
	ImGui::End();

	Matrix4x4 projection = MakePerspectiveFovMatrix(0.45f, float(1280.0f) / float(720.0f), 0.1f, 10000.0f);
	editorUI_->Draw(gameObjectManager_.get(), view_, projection);

	EditorManager::SetSceneOverlayCallback([this]() {
		Matrix4x4 proj = MakePerspectiveFovMatrix(0.45f, float(1280.0f) / float(720.0f), 0.1f, 10000.0f);
		editorUI_->DrawGizmoInScene(view_, proj);
	});
#endif
}

void JsonScene::Initialize()
{
	sceneID_ = SceneID::Json;
	sceneChangeRequest_ = false;

	gameObjectManager_ = std::make_unique<GameObjectManager>();
	camera_ = std::make_unique<Camera>();
	editorUI_ = std::make_unique<EditorUI>();

	if (!EditorManager::IsPlaying()) {
		camera_->SetDebugCamera(true);
	} else {
		camera_->SetDebugCamera(false);
	}
	
	Transform camTransform{ {0.5f,0.5f,0.5f},{0.3f,0.0f,0.0f,},{0.0f,8.0f,-15.0f} };
	camera_->SetTransform(camTransform);
	camera_->Update();
	view_ = camera_->GetViewMatrix();

	// エディターでの保存・読み込み先をJsonSceneに設定
	EditorManager::SetSaveCallback([this](const std::string& filePath) {
		gameObjectManager_->SaveScene(filePath);
	});
	EditorManager::SetLoadCallback([this](const std::string& filePath) {
		gameObjectManager_->LoadScene(filePath);
	});
	EditorManager::SetFileDropCallback([this](const std::string& dropPath) {
		size_t lastSlash = dropPath.find_last_of("/\\");
		if (lastSlash != std::string::npos) {
			std::string dirPath = dropPath.substr(0, lastSlash);
			std::string fileName = dropPath.substr(lastSlash + 1);
			
			size_t extPos = fileName.find_last_of(".");
			if (extPos != std::string::npos) {
				std::string ext = fileName.substr(extPos);
				std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
				if (ext == ".obj" || ext == ".gltf") {
					try {
						ModelData modelData = AssetManager::LoadModel(dirPath, fileName);
						auto model = std::make_shared<Model>();
						model->Initialize(modelData);
						model->name_ = fileName;
						
						auto renderObj = std::make_shared<RenderObject>(model);
						renderObj->SetName(fileName);
						renderObj->modelFilePath_ = dirPath + "/" + fileName;
						gameObjectManager_->AddObject(renderObj);
					} catch (const std::exception& e) {
					}
				}
			}
		}
	});

#ifdef _USE_IMGUI
	showInitJsonSelectPopup_ = true;
#else
	gameObjectManager_->LoadScene(initialSceneJson_);
#endif
}

void JsonScene::Update()
{
	if (showInitJsonSelectPopup_) {
		return;
	}

	bool isEnginePlaying = EditorManager::IsPlaying();
	camera_->SetDebugCamera(!isEnginePlaying);

	camera_->Update();
	view_ = camera_->GetViewMatrix();

	float speedMultiplier = isEnginePlaying ? 1.0f : 0.0f;
	gameObjectManager_->UpdateAll(view_, speedMultiplier);
}

void JsonScene::Draw(class Draw& draw)
{
	if (showInitJsonSelectPopup_) {
		return;
	}

	draw.SetCamera(camera_.get());
	gameObjectManager_->DrawAll(draw);
}

#include "EditorManager.h"
#include <Engine.h>
#include <filesystem>
#include <vector>
#include <chrono>
#include <windows.h>
#include <fstream>

#ifdef _USE_IMGUI
#include "../externals/imgui/imgui.h"
#include "../externals/imgui/imgui_internal.h"
#include "../externals/imgui/ImGuizmo.h"
#endif // _USE_IMGUI

#include "../MatchaEngine/Core/LogHandler.h"
#include "../MatchaEngine/Resource/Texture.h"
#include "LanguageManager.h"
#include <unordered_map>
#include <algorithm>

#include "../MatchaEngine/Graphics/RenderTexture.h"
#include "../MatchaEngine/Graphics/DepthStencil.h"
#include "../MatchaEngine/GameObjects/Effect/Emitter.h"
#include "../MatchaEngine/GameObjects/Camera/Camera.h"
#include "../MatchaEngine/GameObjects/Line/Grid.h"
#include "../MatchaEngine/Graphics/DescriptorHeap.h"
#include "../MatchaEngine/Graphics/GraphicsDevice.h"
#include "../MatchaEngine/Common/CommandContext.h"
#include "../MatchaEngine/Graphics/Render/Draw.h"
#include "../MatchaEngine/GameObjects/Object/3d/Model.h"
#include "../MatchaEngine/Resource/Load.h"

#ifdef _USE_IMGUI
static std::filesystem::path s_selectedResourceDir = "resources";
static std::unordered_map<std::string, D3D12_GPU_DESCRIPTOR_HANDLE> s_iconCache;
static std::unique_ptr<Texture> s_editorTexture;
static bool s_resetLayoutRequested = false;
static bool s_saveLayoutRequested = false;
static bool s_loadLayoutRequested = false;
static float s_layoutNoticeTimer = 0.0f;
static std::string s_layoutNoticeMsg = "";
static const std::string s_layoutFilePath = "Resources/Layout/saved_layout.ini";

int EditorManager::s_gizmoOp = 0;

void DrawDirectoryTree(const std::filesystem::path& dirPath) {
	try {
		for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
			if (entry.is_directory()) {
				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
				if (s_selectedResourceDir == entry.path()) {
					flags |= ImGuiTreeNodeFlags_Selected;
				}
				
				auto u8name = entry.path().filename().u8string();
				bool isOpen = ImGui::TreeNodeEx(reinterpret_cast<const char*>(u8name.c_str()), flags);
				if (ImGui::IsItemClicked()) {
					s_selectedResourceDir = entry.path();
				}
				if (isOpen) {
					DrawDirectoryTree(entry.path());
					ImGui::TreePop();
				}
			}
		}
	} catch (...) {}
}

void DrawDirectoryContents(const std::filesystem::path& dirPath) {
	try {
		float padding = 16.0f;
		float thumbnailSize = 64.0f;
		float cellSize = thumbnailSize + padding;
		float panelWidth = ImGui::GetContentRegionAvail().x;
		int columnCount = (int)(panelWidth / cellSize);
		if (columnCount < 1) columnCount = 1;

		ImGui::Columns(columnCount, 0, false);

		for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
			std::string name;
			auto u8name = entry.path().filename().u8string();
			name = std::string(reinterpret_cast<const char*>(u8name.c_str()));
			
			bool isDirectory = entry.is_directory();
			std::string ext = entry.path().extension().string();
			std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
			
			ImGui::PushID(name.c_str());
			
			ImTextureID texID = 0;
			bool isImage = false;
			
			if (!isDirectory && (ext == ".png" || ext == ".jpg" || ext == ".jpeg")) {
				isImage = true;
				auto u8str = entry.path().u8string();
				std::string pathStr(reinterpret_cast<const char*>(u8str.c_str()));
				std::replace(pathStr.begin(), pathStr.end(), '\\', '/');

				if (s_iconCache.find(pathStr) == s_iconCache.end()) {
					if (!s_editorTexture) s_editorTexture = std::make_unique<Texture>();
					int id = s_editorTexture->CreateTexture(pathStr);
					s_iconCache[pathStr] = s_editorTexture->TextureData(id);
				}
				D3D12_GPU_DESCRIPTOR_HANDLE handle = s_iconCache[pathStr];
				texID = (ImTextureID)handle.ptr;
			}
			
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			if (isImage) {
				ImGui::ImageButton(name.c_str(), texID, ImVec2(thumbnailSize, thumbnailSize));
			} else {
				ImGui::Button("##Icon", ImVec2(thumbnailSize, thumbnailSize));
				
				ImDrawList* drawList = ImGui::GetWindowDrawList();
				ImVec2 rectMin = ImGui::GetItemRectMin();
				ImVec2 rectMax = ImGui::GetItemRectMax();
				float itemW = rectMax.x - rectMin.x;
				float itemH = rectMax.y - rectMin.y;
				
				ImU32 outlineColor = IM_COL32(150, 150, 150, 255);
				if (ImGui::IsItemHovered()) outlineColor = IM_COL32(220, 220, 220, 255);
				
				if (isDirectory) {
					ImU32 folderColor = IM_COL32(234, 194, 82, 255);
					if (ImGui::IsItemHovered()) folderColor = IM_COL32(255, 214, 102, 255);
					ImU32 folderDark = IM_COL32(204, 164, 52, 255);
					float tabW = itemW * 0.45f;
					float tabH = itemH * 0.15f;
					drawList->AddRectFilled(rectMin, ImVec2(rectMin.x + tabW, rectMin.y + tabH), folderDark, 2.0f);
					drawList->AddRectFilled(ImVec2(rectMin.x, rectMin.y + tabH * 0.8f), ImVec2(rectMin.x + itemW, rectMin.y + itemH), folderColor, 4.0f);
				} else {
					ImU32 docColor = IM_COL32(230, 230, 230, 255);
					if (ImGui::IsItemHovered()) docColor = IM_COL32(250, 250, 250, 255);
					
					ImU32 labelBgColor = IM_COL32(100, 100, 100, 255);
					if (ext == ".json") labelBgColor = IM_COL32(180, 180, 50, 255);
					else if (ext == ".dds") labelBgColor = IM_COL32(150, 50, 50, 255);
					else if (ext == ".obj" || ext == ".gltf") labelBgColor = IM_COL32(50, 150, 200, 255);
					else if (ext == ".hlsl" || ext == ".hlsli") labelBgColor = IM_COL32(50, 200, 150, 255);
					
					float foldSize = itemW * 0.25f;
					
					drawList->AddRectFilled(rectMin, ImVec2(rectMin.x + itemW, rectMin.y + itemH), docColor, 2.0f);
					drawList->AddRect(rectMin, ImVec2(rectMin.x + itemW, rectMin.y + itemH), outlineColor, 2.0f);
					
					ImVec2 foldPts[3] = {
						ImVec2(rectMin.x + itemW - foldSize, rectMin.y),
						ImVec2(rectMin.x + itemW - foldSize, rectMin.y + foldSize),
						ImVec2(rectMin.x + itemW, rectMin.y + foldSize)
					};
					drawList->AddConvexPolyFilled(foldPts, 3, IM_COL32(180, 180, 180, 255));
					
					float labelH = itemH * 0.35f;
					ImVec2 labelPos = ImVec2(rectMin.x, rectMin.y + itemH * 0.45f);
					drawList->AddRectFilled(labelPos, ImVec2(rectMin.x + itemW, labelPos.y + labelH), labelBgColor);
					
					std::string text = ext.empty() ? "" : ext.substr(1);
					std::transform(text.begin(), text.end(), text.begin(), ::toupper);
					
					if (text.length() > 4) text = text.substr(0, 4); // Keep text short
					
					ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
					ImVec2 textPos = ImVec2(rectMin.x + (itemW - textSize.x) * 0.5f, labelPos.y + (labelH - textSize.y) * 0.5f);
					drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), text.c_str());
				}
			}
			ImGui::PopStyleColor();

			if (!isDirectory) {
				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
					auto u8str = entry.path().u8string();
					std::string pathStr(reinterpret_cast<const char*>(u8str.c_str()));
					std::replace(pathStr.begin(), pathStr.end(), '\\', '/');
					ImGui::SetDragDropPayload("RESOURCE_FILE", pathStr.c_str(), pathStr.size() + 1);
					ImGui::Text("Drag %s", name.c_str());
					ImGui::EndDragDropSource();
				}
			}
			
			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
				if (isDirectory) {
					s_selectedResourceDir = entry.path();
				}
			}
			
			ImGui::TextWrapped("%s", name.c_str());
			
			ImGui::NextColumn();
			ImGui::PopID();
		}
		ImGui::Columns(1);
	} catch (...) {}
}
#endif // _USE_IMGUI

#ifdef _USE_IMGUI
bool EditorManager::isPlaying_ = false;
float EditorManager::playSpeed_ = 1.0f;
#else
bool EditorManager::isPlaying_ = true;
float EditorManager::playSpeed_ = 1.0f;
#endif

#ifdef _USE_IMGUI
ImVec2 EditorManager::s_sceneImagePos = ImVec2(0, 0);
ImVec2 EditorManager::s_sceneImageSize = ImVec2(0, 0);
#endif

EditorManager::SceneOverlayCallback EditorManager::s_sceneOverlayCallback_ = nullptr;
EditorManager::EditorCallback EditorManager::s_saveCallback_ = nullptr;
EditorManager::EditorCallback EditorManager::s_loadCallback_ = nullptr;
EditorManager::FileDropCallback EditorManager::s_fileDropCallback_ = nullptr;
EditorManager::GameViewDrawCallback EditorManager::s_gameViewDrawCallback_ = nullptr;
EditorManager::GameViewUIDrawCallback EditorManager::s_gameViewUIDrawCallback_ = nullptr;
std::string EditorManager::s_currentFileName_ = "scene";

EditorManager::~EditorManager() = default;

#ifdef _USE_IMGUI
static float CalculateCPUUsage() {
    static FILETIME prevIdleTime = {};
    static FILETIME prevKernelTime = {};
    static FILETIME prevUserTime = {};
    static bool firstCall = true;

    FILETIME idleTime, kernelTime, userTime;
    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        return 0.0f;
    }

    if (firstCall) {
        prevIdleTime = idleTime;
        prevKernelTime = kernelTime;
        prevUserTime = userTime;
        firstCall = false;
        return 0.0f;
    }

    auto FileTimeToQuad = [](const FILETIME& ft) -> ULONGLONG {
        return (static_cast<ULONGLONG>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
    };

    ULONGLONG idle = FileTimeToQuad(idleTime) - FileTimeToQuad(prevIdleTime);
    ULONGLONG kernel = FileTimeToQuad(kernelTime) - FileTimeToQuad(prevKernelTime);
    ULONGLONG user = FileTimeToQuad(userTime) - FileTimeToQuad(prevUserTime);

    prevIdleTime = idleTime;
    prevKernelTime = kernelTime;
    prevUserTime = userTime;

    ULONGLONG total = kernel + user;
    if (total == 0) return 0.0f;

    if (total < idle) return 0.0f;
    ULONGLONG active = total - idle;

    return (static_cast<float>(active) / static_cast<float>(total)) * 100.0f;
}

static float GetCPUUsageSmooth() {
    static float s_cpuUsage = 0.0f;
    static auto lastTime = std::chrono::steady_clock::now();
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime).count();
    if (elapsed >= 200) { // Update every 200ms
        s_cpuUsage = CalculateCPUUsage();
        lastTime = now;
    }
    return s_cpuUsage;
}

static void GetGPUMemoryInfo(Engine* engine, float& currentUsageMB, float& budgetMB) {
    currentUsageMB = 0.0f;
    budgetMB = 0.0f;
    if (!engine || !engine->graphics) return;
    IDXGIAdapter4* adapter = engine->graphics->GetUseAdapter();
    if (!adapter) return;

    DXGI_QUERY_VIDEO_MEMORY_INFO info{};
    if (SUCCEEDED(adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info))) {
        currentUsageMB = static_cast<float>(info.CurrentUsage) / (1024.0f * 1024.0f);
        budgetMB = static_cast<float>(info.Budget) / (1024.0f * 1024.0f);
    }
}
#endif // _USE_IMGUI

void EditorManager::Update(Engine* engine)
{
#ifdef _USE_IMGUI

	static bool showSaveAsPopup = false;
	static bool showLoadPopup = false;
	static char fileNameBuffer[256] = "";

	auto getFullPath = [](const std::string& name) {
		std::string fname = name;
		if (fname.length() < 5 || fname.substr(fname.length() - 5) != ".json") {
			fname += ".json";
		}
		return "Resources/Json/Scene/" + fname;
	};

	// ===== Unity風メインメニューバー =====
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu(LanguageManager::Tr("File"))) {
			std::string displayFileName = s_currentFileName_.empty() ? LanguageManager::Tr("None") : s_currentFileName_ + ".json";
			ImGui::TextDisabled(LanguageManager::Tr("Current File: %s"), displayFileName.c_str());
			ImGui::Separator();

			if (ImGui::MenuItem(LanguageManager::Tr("Save"), "Ctrl+S")) {
				if (s_currentFileName_.empty()) {
					showSaveAsPopup = true;
					snprintf(fileNameBuffer, sizeof(fileNameBuffer), "%s", "scene");
				} else {
					if (s_saveCallback_) s_saveCallback_(getFullPath(s_currentFileName_));
				}
			}
			if (ImGui::MenuItem(LanguageManager::Tr("Save As..."))) {
				showSaveAsPopup = true;
				snprintf(fileNameBuffer, sizeof(fileNameBuffer), "%s", s_currentFileName_.empty() ? "scene" : s_currentFileName_.c_str());
			}
			if (ImGui::MenuItem(LanguageManager::Tr("Load..."), "Ctrl+L")) {
				showLoadPopup = true;
				snprintf(fileNameBuffer, sizeof(fileNameBuffer), "%s", s_currentFileName_.empty() ? "scene" : s_currentFileName_.c_str());
			}
			ImGui::Separator();
			if (ImGui::MenuItem(LanguageManager::Tr("Exit"))) {
				Engine::SetEnd(true);
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu(LanguageManager::Tr("Window"))) {
			ImGui::MenuItem(LanguageManager::Tr("Debug Info"), nullptr, &showFinalWindow_);
			ImGui::MenuItem(LanguageManager::Tr("Resources"), nullptr, &showResourcesWindow_);
			ImGui::MenuItem(LanguageManager::Tr("Logs"), nullptr, &showLogsWindow_);
			ImGui::MenuItem(LanguageManager::Tr("Particle Editor"), nullptr, &showParticleViewer_);
			ImGui::MenuItem(LanguageManager::Tr("Object Editor"), nullptr, &showModelViewer_);
			ImGui::MenuItem(LanguageManager::Tr("Game View"), nullptr, &showGameViewWindow_);
			ImGui::Separator();
			if (ImGui::BeginMenu(LanguageManager::Tr("Layout"))) {
				if (ImGui::MenuItem(LanguageManager::Tr("Save Layout"))) {
					s_saveLayoutRequested = true;
				}
				bool hasSavedLayout = std::filesystem::exists(s_layoutFilePath);
				if (ImGui::MenuItem(LanguageManager::Tr("Restore Layout"), nullptr, false, hasSavedLayout)) {
					s_loadLayoutRequested = true;
				}
				ImGui::Separator();
				if (ImGui::MenuItem(LanguageManager::Tr("Reset to Default"))) {
					s_resetLayoutRequested = true;
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu(LanguageManager::Tr("Settings"))) {
			if (ImGui::BeginMenu(LanguageManager::Tr("Language"))) {
				if (ImGui::MenuItem("English", nullptr, LanguageManager::GetLanguage() == EditorLanguage::English)) {
					LanguageManager::SetLanguage(EditorLanguage::English);
				}
				if (ImGui::MenuItem("日本語", nullptr, LanguageManager::GetLanguage() == EditorLanguage::Japanese)) {
					LanguageManager::SetLanguage(EditorLanguage::Japanese);
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}

		// --- メニューバー中央にPlay/Stopボタン ---
		float menuBarWidth = ImGui::GetWindowWidth();
		float buttonAreaWidth = 240.0f;
		ImGui::SetCursorPosX((menuBarWidth - buttonAreaWidth) * 0.5f);

		if (isPlaying_) {
			// Stopボタン（赤）
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.15f, 0.15f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.25f, 0.25f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
			if (ImGui::Button(LanguageManager::Tr("  Stop  "))) {
				isPlaying_ = false;
			}
			ImGui::PopStyleColor(3);
		} else {
			// Playボタン（緑）
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.65f, 0.15f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.75f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.55f, 0.1f, 1.0f));
			if (ImGui::Button(LanguageManager::Tr("  Play  "))) {
				isPlaying_ = true;
			}
			ImGui::PopStyleColor(3);
		}

		ImGui::SameLine();
		ImGui::SetNextItemWidth(80.0f);
		const char* speedLabels[] = { "x4", "x2", "x1", "x-2", "x-4" };
		const float speedValues[] = { 4.0f, 2.0f, 1.0f, 0.5f, 0.25f };
		static int speedIndex = 2; // Default to x1
		if (ImGui::Combo("##PlaySpeed", &speedIndex, speedLabels, IM_ARRAYSIZE(speedLabels))) {
			playSpeed_ = speedValues[speedIndex];
		}

		if (s_layoutNoticeTimer > 0.0f) {
			s_layoutNoticeTimer -= ImGui::GetIO().DeltaTime;
			float textWidth = ImGui::CalcTextSize(s_layoutNoticeMsg.c_str()).x;
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() - textWidth - 20.0f);
			ImGui::TextColored(ImVec4(0.3f, 0.85f, 0.3f, 1.0f), "%s", s_layoutNoticeMsg.c_str());
		}

		ImGui::EndMainMenuBar();
	}

	if (s_loadLayoutRequested) {
		s_loadLayoutRequested = false;
		if (std::filesystem::exists(s_layoutFilePath)) {
			ImGui::LoadIniSettingsFromDisk(s_layoutFilePath.c_str());

			// ウィンドウの表示状態 (EditorState) を復元
			std::ifstream in(s_layoutFilePath);
			if (in.is_open()) {
				std::string line;
				bool inEditorState = false;
				while (std::getline(in, line)) {
					if (!line.empty() && line.back() == '\r') {
						line.pop_back();
					}
					if (line == "[EditorState]") {
						inEditorState = true;
						continue;
					}
					if (inEditorState) {
						if (line.empty() || line[0] == '[') {
							break;
						}
						auto eq = line.find('=');
						if (eq != std::string::npos) {
							std::string key = line.substr(0, eq);
							try {
								int val = std::stoi(line.substr(eq + 1));
								if (key == "showFinalWindow") showFinalWindow_ = (val != 0);
								else if (key == "showResourcesWindow") showResourcesWindow_ = (val != 0);
								else if (key == "showLogsWindow") showLogsWindow_ = (val != 0);
								else if (key == "showParticleViewer") showParticleViewer_ = (val != 0);
								else if (key == "showModelViewer") showModelViewer_ = (val != 0);
								else if (key == "showGameViewWindow") showGameViewWindow_ = (val != 0);
							} catch (...) {}
						}
					}
				}
			}

			s_layoutNoticeMsg = LanguageManager::Tr("Layout restored.");
			s_layoutNoticeTimer = 3.0f;
			Log("[Editor] Layout restored from Resources/Layout/saved_layout.ini\n");
		}
	}

	// ウィンドウ全体をDockSpaceとして設定（各ImGuiウィンドウをドッキング固定可能にする）
	// ※すべてのImGuiウィンドウ生成より前に呼び出す必要があります
	ImGuiID dockspace_id = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

	if (s_saveLayoutRequested) {
		s_saveLayoutRequested = false;
		try {
			std::filesystem::create_directories("Resources/Layout");
			ImGui::SaveIniSettingsToDisk(s_layoutFilePath.c_str());

			// ウィンドウの表示状態 (EditorState) を追記保存
			std::ofstream out(s_layoutFilePath, std::ios::app);
			if (out.is_open()) {
				out << "\n[EditorState]\n";
				out << "showFinalWindow=" << (showFinalWindow_ ? 1 : 0) << "\n";
				out << "showResourcesWindow=" << (showResourcesWindow_ ? 1 : 0) << "\n";
				out << "showLogsWindow=" << (showLogsWindow_ ? 1 : 0) << "\n";
				out << "showParticleViewer=" << (showParticleViewer_ ? 1 : 0) << "\n";
				out << "showModelViewer=" << (showModelViewer_ ? 1 : 0) << "\n";
				out << "showGameViewWindow=" << (showGameViewWindow_ ? 1 : 0) << "\n";
			}

			s_layoutNoticeMsg = LanguageManager::Tr("Layout saved.");
			s_layoutNoticeTimer = 3.0f;
			Log("[Editor] Layout saved to Resources/Layout/saved_layout.ini\n");
		} catch (...) {
			Log("[Editor] Failed to save layout.\n");
		}
	}

	if (s_resetLayoutRequested) {
		s_resetLayoutRequested = false;
		ImGui::DockBuilderRemoveNode(dockspace_id);
		ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->WorkSize);

		ImGuiID dock_main_id = dockspace_id;
		ImGuiID dock_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.2f, nullptr, &dock_main_id);
		ImGuiID dock_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, nullptr, &dock_main_id);
		ImGuiID dock_down = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.3f, nullptr, &dock_main_id);

		ImGui::DockBuilderDockWindow("Hierarchy", dock_left);
		ImGui::DockBuilderDockWindow("ヒエラルキー", dock_left);
		ImGui::DockBuilderDockWindow("Inspector", dock_right);
		ImGui::DockBuilderDockWindow("インスペクター", dock_right);
		ImGui::DockBuilderDockWindow("Debug Info", dock_right);
		ImGui::DockBuilderDockWindow("デバッグ情報", dock_right);
		ImGui::DockBuilderDockWindow("Resources", dock_down);
		ImGui::DockBuilderDockWindow("リソース", dock_down);
		ImGui::DockBuilderDockWindow("Logs", dock_down);
		ImGui::DockBuilderDockWindow("ログ", dock_down);
		ImGui::DockBuilderDockWindow("Scene", dock_main_id);
		ImGui::DockBuilderDockWindow("シーン", dock_main_id);
		ImGui::DockBuilderDockWindow("Game View", dock_main_id);
		ImGui::DockBuilderDockWindow("GameScene", dock_main_id);
		ImGui::DockBuilderFinish(dockspace_id);

		showFinalWindow_ = true;
		showResourcesWindow_ = true;
		showLogsWindow_ = true;
		showParticleViewer_ = false;
		showModelViewer_ = false;
		showGameViewWindow_ = true;

		s_layoutNoticeMsg = LanguageManager::Tr("Layout reset to default.");
		s_layoutNoticeTimer = 3.0f;
		Log("[Editor] Layout reset to default.\n");
	}

	// Ctrl+S / Ctrl+L ショートカット
	if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
		if (s_currentFileName_.empty()) {
			showSaveAsPopup = true;
			snprintf(fileNameBuffer, sizeof(fileNameBuffer), "%s", "scene");
		} else {
			if (s_saveCallback_) s_saveCallback_(getFullPath(s_currentFileName_));
		}
	}
	if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_L)) {
		showLoadPopup = true;
		snprintf(fileNameBuffer, sizeof(fileNameBuffer), "%s", s_currentFileName_.empty() ? "scene" : s_currentFileName_.c_str());
	}

	// Save As... ポップアップ
	if (showSaveAsPopup) {
		ImGui::OpenPopup(LanguageManager::Tr("Save As..."));
		showSaveAsPopup = false;
	}
	if (ImGui::BeginPopupModal(LanguageManager::Tr("Save As..."), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		std::string displayFileName = s_currentFileName_.empty() ? LanguageManager::Tr("None") : s_currentFileName_ + ".json";
		ImGui::TextDisabled(LanguageManager::Tr("Current File: %s"), displayFileName.c_str());
		ImGui::Text(LanguageManager::Tr("File name (saved in Resources/Json/Scene/):"));
		ImGui::InputText("##savepath", fileNameBuffer, sizeof(fileNameBuffer));
		if (ImGui::Button(LanguageManager::Tr("Save"), ImVec2(120, 0))) {
			s_currentFileName_ = fileNameBuffer;
			if (s_saveCallback_) s_saveCallback_(getFullPath(s_currentFileName_));
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button(LanguageManager::Tr("Cancel"), ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	// Load Scene... ポップアップ
	if (showLoadPopup) {
		ImGui::OpenPopup(LanguageManager::Tr("Load..."));
		showLoadPopup = false;
	}
	if (ImGui::BeginPopupModal(LanguageManager::Tr("Load..."), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		std::string displayFileName = s_currentFileName_.empty() ? LanguageManager::Tr("None") : s_currentFileName_ + ".json";
		ImGui::TextDisabled(LanguageManager::Tr("Current File: %s"), displayFileName.c_str());
		ImGui::Text(LanguageManager::Tr("Select file to load from Resources/Json/Scene/:"));

		// List files
		std::vector<std::string> jsonFiles;
		try {
			for (const auto& entry : std::filesystem::directory_iterator("Resources/Json/Scene")) {
				if (entry.is_regular_file() && entry.path().extension() == ".json") {
					jsonFiles.push_back(entry.path().stem().string());
				}
			}
		} catch (...) {}

		if (ImGui::BeginCombo(LanguageManager::Tr("Target File"), s_currentFileName_.c_str())) {
			for (const auto& file : jsonFiles) {
				bool is_selected = (s_currentFileName_ == file);
				if (ImGui::Selectable(file.c_str(), is_selected)) {
					s_currentFileName_ = file;
					snprintf(fileNameBuffer, sizeof(fileNameBuffer), "%s", file.c_str());
				}
				if (is_selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		ImGui::InputText("##loadpath", fileNameBuffer, sizeof(fileNameBuffer));

		if (ImGui::Button(LanguageManager::Tr("Load"), ImVec2(120, 0))) {
			s_currentFileName_ = fileNameBuffer;
			if (s_loadCallback_) s_loadCallback_(getFullPath(s_currentFileName_));
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button(LanguageManager::Tr("Cancel"), ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	// 各種ウィンドウ描画

	if (showFinalWindow_) {
		ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
		ImGui::Begin(LanguageManager::Tr("Debug Info"), &showFinalWindow_);

		if (ImGui::BeginTabBar("DebugTabs")) {
			if (ImGui::BeginTabItem(LanguageManager::Tr("System Data"))) {
				ImGui::Text(LanguageManager::Tr("--- Performance ---"));

				// FPS Graph and numeric
				float currentFps = ImGui::GetIO().Framerate;
				static float fpsHistory[100] = {};
				for (int i = 0; i < 99; ++i) {
					fpsHistory[i] = fpsHistory[i + 1];
				}
				fpsHistory[99] = currentFps;
				ImGui::Text("FPS: %.1f (%.3f ms/frame)", currentFps, 1000.0f / currentFps);
				ImGui::PlotLines("##FPSGraph", fpsHistory, 100, 0, nullptr, 0.0f, 120.0f, ImVec2(0, 60.0f));

				// CPU Graph and numeric
				float currentCpu = GetCPUUsageSmooth();
				static float cpuHistory[100] = {};
				for (int i = 0; i < 99; ++i) {
					cpuHistory[i] = cpuHistory[i + 1];
				}
				cpuHistory[99] = currentCpu;
				ImGui::Text(LanguageManager::Tr("CPU Usage: %.1f %%"), currentCpu);
				ImGui::PlotLines("##CPUGraph", cpuHistory, 100, 0, nullptr, 0.0f, 100.0f, ImVec2(0, 60.0f));

				// GPU Graph and numeric
				float currentGpuUsage = 0.0f;
				float gpuBudget = 0.0f;
				GetGPUMemoryInfo(engine, currentGpuUsage, gpuBudget);
				static float gpuHistory[100] = {};
				for (int i = 0; i < 99; ++i) {
					gpuHistory[i] = gpuHistory[i + 1];
				}
				gpuHistory[99] = currentGpuUsage;
				if (gpuBudget > 0.0f) {
					ImGui::Text(LanguageManager::Tr("GPU VRAM: %.1f MB / %.1f MB"), currentGpuUsage, gpuBudget);
					ImGui::PlotLines("##GPUGraph", gpuHistory, 100, 0, nullptr, 0.0f, gpuBudget, ImVec2(0, 60.0f));
				} else {
					ImGui::Text(LanguageManager::Tr("GPU VRAM: %.1f MB"), currentGpuUsage);
					ImGui::PlotLines("##GPUGraph", gpuHistory, 100, 0, nullptr, 0.0f, 4096.0f, ImVec2(0, 60.0f));
				}

				static int frameCount = 0;
				frameCount++;
				ImGui::Text(LanguageManager::Tr("Frame Count: %d"), frameCount);

				size_t mem = engine->GetProcessMemoryUsage();
				ImGui::Text(LanguageManager::Tr("Memory Usage: %.2f MB"), mem / (1024.0f * 1024.0f));
				ImGui::Spacing();

				ImGui::Text(LanguageManager::Tr("--- Application ---"));
				ImGui::Text(LanguageManager::Tr("Resolution: %d x %d"), engine->GetClientWidth(), engine->GetClientHeight());
				ImGui::Spacing();

				ImGui::Text(LanguageManager::Tr("--- Light Settings ---"));
				static bool showLight = false;
				ImGui::Checkbox(LanguageManager::Tr("Enable Light Settings (Shortcut: F1)"), &showLight);
				if (ImGui::IsKeyPressed(ImGuiKey_F1))
				{
					showLight = !showLight;
				}
				
				if (showLight)
				{
					if (engine->GetLightManager()) {
						engine->GetLightManager()->ImGui();
					}
				}

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem(LanguageManager::Tr("Post Effect"))) {
				auto& postEffects = const_cast<std::vector<std::unique_ptr<PostEffect>>&>(engine->GetPostEffects());
				for (size_t i = 0; i < postEffects.size(); ++i) {
					ImGui::PushID(static_cast<int>(i));
					std::string layerTitle = "Layer " + std::to_string(i) + ": " + postEffects[i]->GetActiveShaderName();
					ImGui::TextColored(ImVec4(0.35f, 0.75f, 1.0f, 1.0f), "%s", layerTitle.c_str());
					postEffects[i]->ImGuiWindow();
					if (ImGui::Button(LanguageManager::Tr("Remove Layer"))) {
						postEffects.erase(postEffects.begin() + i);
						ImGui::PopID();
						break; // Break and skip the rest of the loop for this frame to avoid invalid iterators
					}
					ImGui::Separator();
					ImGui::PopID();
				}
				if (ImGui::Button(LanguageManager::Tr("Add Post Effect Layer"))) {
					auto newEffect = std::make_unique<PostEffect>();
					newEffect->Initialize();
					postEffects.push_back(std::move(newEffect));
				}
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem(LanguageManager::Tr("Resource List"))) {
				if (engine->GetTextureLoader()) {
					engine->GetTextureLoader()->Draw();
				}
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("GPU Profiler")) {
				GpuProfiler* profiler = engine->GetGpuProfiler();
				if (profiler) {
					float totalMs = profiler->GetTotalTimeMs();
					ImGui::Text("Total Measured GPU Time: %.3f ms (%.1f us)", totalMs, totalMs * 1000.0f);
					ImGui::Separator();

					const auto& results = profiler->GetResults();
					if (results.empty()) {
						ImGui::TextDisabled("No active GPU profiles recorded in this frame.");
					} else {
						if (ImGui::BeginTable("GpuProfilerTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
							ImGui::TableSetupColumn("Compute Shader / Task");
							ImGui::TableSetupColumn("Time (ms)");
							ImGui::TableSetupColumn("Avg (ms)");
							ImGui::TableSetupColumn("Max (ms)");
							ImGui::TableHeadersRow();

							for (const auto& res : results) {
								ImGui::TableNextRow();
								ImGui::TableSetColumnIndex(0);
								ImGui::Text("%s", res.name.c_str());

								ImGui::TableSetColumnIndex(1);
								ImGui::Text("%.3f ms", res.timeMs);

								ImGui::TableSetColumnIndex(2);
								ImGui::Text("%.3f ms", res.avgTimeMs);

								ImGui::TableSetColumnIndex(3);
								ImGui::Text("%.3f ms", res.maxTimeMs);
							}
							ImGui::EndTable();
						}

						ImGui::Spacing();
						ImGui::Text("Breakdown:");
						for (const auto& res : results) {
							float fraction = totalMs > 0.0001f ? (res.timeMs / totalMs) : 0.0f;
							char overlay[64];
							snprintf(overlay, sizeof(overlay), "%s: %.3f ms (%.1f%%)", res.name.c_str(), res.timeMs, fraction * 100.0f);
							ImGui::ProgressBar(fraction, ImVec2(-1.0f, 0.0f), overlay);
						}
					}
				} else {
					ImGui::TextDisabled("GPU Profiler is not initialized.");
				}
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem(LanguageManager::Tr("Culling & Rendering"))) {
				Draw* draw = engine->GetDraw();
				if (draw) {
					bool frustumCulling = draw->IsFrustumCullingEnabled();
					if (ImGui::Checkbox(LanguageManager::Tr("Enable Frustum Culling"), &frustumCulling)) {
						draw->SetFrustumCullingEnabled(frustumCulling);
					}

					bool debugAABB = draw->IsDebugDrawAABB();
					if (ImGui::Checkbox(LanguageManager::Tr("Draw Bounding Boxes (Debug Wireframe)"), &debugAABB)) {
						draw->SetDebugDrawAABB(debugAABB);
					}

					ImGui::Separator();
					ImGui::Text(LanguageManager::Tr("--- Frustum Culling Statistics ---"));
					uint32_t total = draw->GetTotalDrawCalls();
					uint32_t culled = draw->GetCulledDrawCalls();
					uint32_t rendered = (total >= culled) ? (total - culled) : 0;
					float cullPercent = (total > 0) ? (static_cast<float>(culled) / static_cast<float>(total) * 100.0f) : 0.0f;

					ImGui::Text(LanguageManager::Tr("Total Draw Calls: %u"), total);
					ImGui::Text(LanguageManager::Tr("Rendered Calls: %u"), rendered);
					ImGui::Text(LanguageManager::Tr("Culled Calls: %u (%.1f%%)"), culled, cullPercent);

					char overlay[64];
					snprintf(overlay, sizeof(overlay), "Culled: %u / %u (%.1f%%)", culled, total, cullPercent);
					ImGui::ProgressBar(cullPercent / 100.0f, ImVec2(-1.0f, 0.0f), overlay);

					ImGui::Separator();
					ImGui::Text(LanguageManager::Tr("--- Back-face Culling ---"));
					ImGui::BulletText(LanguageManager::Tr("3D Opaque Models: Back-face culling (CW)"));
					ImGui::BulletText(LanguageManager::Tr("SkyBox: Front-face culling"));
					ImGui::BulletText(LanguageManager::Tr("Sprites / UI / Billboards: Cull None (Double-sided)"));
					ImGui::BulletText(LanguageManager::Tr("Per-object / Per-material CullMode can be changed in Inspector."));
				}
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		ImGui::End();
	}

	ImGui::Begin(LanguageManager::Tr("Scene"));

	// --- ギズモ操作ボタン ---
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 0));
	
	ImVec4 activeCol = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
	ImVec4 defaultCol = ImGui::GetStyleColorVec4(ImGuiCol_Button);
	
	ImGui::PushStyleColor(ImGuiCol_Button, s_gizmoOp == 0 ? activeCol : defaultCol);
	if (ImGui::Button("T")) s_gizmoOp = 0;
	ImGui::PopStyleColor();
	ImGui::SameLine();
	
	ImGui::PushStyleColor(ImGuiCol_Button, s_gizmoOp == 1 ? activeCol : defaultCol);
	if (ImGui::Button("R")) s_gizmoOp = 1;
	ImGui::PopStyleColor();
	ImGui::SameLine();
	
	ImGui::PushStyleColor(ImGuiCol_Button, s_gizmoOp == 2 ? activeCol : defaultCol);
	if (ImGui::Button("S")) s_gizmoOp = 2;
	ImGui::PopStyleColor();
	
	ImGui::PopStyleVar();
	ImGui::SameLine();
	
	// --- アスペクト比設定 ---
	ImGui::SetNextItemWidth(120);
	const char* aspectLabels[] = { "Free", "16:9", "4:3", "1:1", "21:9" };
	const float aspectRatios[] = { 0.0f, 16.0f / 9.0f, 4.0f / 3.0f, 1.0f, 21.0f / 9.0f };
	ImGui::Combo(LanguageManager::Tr("Aspect Ratio"), &sceneAspectRatioIndex_, aspectLabels, IM_ARRAYSIZE(aspectLabels));
	ImGui::Separator();

	ImVec2 availSize = ImGui::GetContentRegionAvail();
	ImVec2 imageSize = availSize;
	ImVec2 cursorStart = ImGui::GetCursorPos();

	if (sceneAspectRatioIndex_ > 0 && availSize.x > 0.0f && availSize.y > 0.0f) {
		float targetAspect = aspectRatios[sceneAspectRatioIndex_];
		float availAspect = availSize.x / availSize.y;

		if (availAspect > targetAspect) {
			// ウィンドウが横に広い → 高さに合わせ、左右に余白
			imageSize.y = availSize.y;
			imageSize.x = availSize.y * targetAspect;
		} else {
			// ウィンドウが縦に長い → 幅に合わせ、上下に余白
			imageSize.x = availSize.x;
			imageSize.y = availSize.x / targetAspect;
		}

		// 余白を計算して中央配置
		float offsetX = (availSize.x - imageSize.x) * 0.5f;
		float offsetY = (availSize.y - imageSize.y) * 0.5f;
		ImGui::SetCursorPos(ImVec2(cursorStart.x + offsetX, cursorStart.y + offsetY));
	}

	s_sceneImagePos = ImGui::GetCursorScreenPos();
	s_sceneImageSize = imageSize;

	if (engine->GetFinalRenderTexture()) {
		ImGui::Image((ImTextureID)engine->GetFinalRenderTexture()->GetSrvHandleGPU().ptr, imageSize);
		
		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_FILE")) {
				const char* dropPath = (const char*)payload->Data;
				if (s_fileDropCallback_) {
					s_fileDropCallback_(dropPath);
				}
			}
			ImGui::EndDragDropTarget();
		}
	}

	// ギズモ等のオーバーレイ描画コールバックをSceneウィンドウのBegin/Endの間に呼び出す
	if (s_sceneOverlayCallback_) {
		s_sceneOverlayCallback_();
	}

	ImGui::End();

	// Resources Window
	if (showResourcesWindow_) {
		ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
		if (ImGui::Begin(LanguageManager::Tr("Resources"), &showResourcesWindow_)) {
			if (ImGui::BeginTable("ResourceBrowser", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable)) {
				ImGui::TableNextRow();
				
				// Left pane: Directory tree
				ImGui::TableSetColumnIndex(0);
				ImGui::BeginChild("DirTree", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
				ImGuiTreeNodeFlags rootFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen;
				if (s_selectedResourceDir == "resources") rootFlags |= ImGuiTreeNodeFlags_Selected;
				bool rootOpen = ImGui::TreeNodeEx("resources", rootFlags);
				if (ImGui::IsItemClicked()) s_selectedResourceDir = "resources";
				if (rootOpen) {
					DrawDirectoryTree("resources");
					ImGui::TreePop();
				}
				ImGui::EndChild();

				// Right pane: Contents of selected directory
				ImGui::TableSetColumnIndex(1);
				ImGui::BeginChild("DirContents", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
				if (std::filesystem::exists(s_selectedResourceDir) && std::filesystem::is_directory(s_selectedResourceDir)) {
					ImGui::TextUnformatted(s_selectedResourceDir.string().c_str());
					ImGui::Separator();
					DrawDirectoryContents(s_selectedResourceDir);
				} else {
					ImGui::Text(LanguageManager::Tr("Directory not found."));
				}
				ImGui::EndChild();

				ImGui::EndTable();
			}
		}
		ImGui::End();
	}

	// Logs Window
	if (showLogsWindow_) {
		ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver);
		if (ImGui::Begin(LanguageManager::Tr("Logs"), &showLogsWindow_)) {
			if (ImGui::Button(LanguageManager::Tr("Clear Logs"))) {
				ClearLogs();
			}
			ImGui::Separator();
			ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
			const auto& logs = GetLogs();
			for (const auto& log : logs) {
				if (log.find("[ERROR]") != std::string::npos || log.find("[FATAL]") != std::string::npos || log.find("CRASH") != std::string::npos) {
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.35f, 0.35f, 1.0f));
					ImGui::TextUnformatted(log.c_str());
					ImGui::PopStyleColor();
				}
				else if (log.find("[WARN]") != std::string::npos) {
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.3f, 1.0f));
					ImGui::TextUnformatted(log.c_str());
					ImGui::PopStyleColor();
				}
				else if (log.find("[INFO]") != std::string::npos) {
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
					ImGui::TextUnformatted(log.c_str());
					ImGui::PopStyleColor();
				}
				else {
					ImGui::TextUnformatted(log.c_str());
				}
			}
			// 自動スクロール
			if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
				ImGui::SetScrollHereY(1.0f);
			}
			ImGui::EndChild();
		}
		ImGui::End();
	}

	// Particle Viewer Window
	if (showParticleViewer_) {
		auto FocusParticleCamera = [&]() {
			if (!previewParticle_ || !previewCamera_) return;
			EmitterData ped = previewParticle_->GetEmitterData();
			EmitterSphere pes = previewParticle_->GetEmitterSphere();
			EmitterCircle pec = previewParticle_->GetEmitterCircle();
			EmitterCone pecone = previewParticle_->GetEmitterCone();
			EmitterType peType = previewParticle_->GetEmitterType();
			Vector3 pBasePos = previewParticle_->GetBaseParticleData().transform.translate;

			Vector3 center;
			float maxDim = 5.0f;
			if (peType == EmitterType::Sphere) {
				center = pes.translate + pBasePos;
				maxDim = (std::max)(pes.radius * 2.0f, 3.0f);
			} else if (peType == EmitterType::Circle) {
				center = pec.translate + pBasePos;
				maxDim = (std::max)(pec.outerRadius * 2.0f, 3.0f);
			} else if (peType == EmitterType::Cone) {
				center = pecone.translate + pBasePos + Vector3{ 0.0f, 1.0f, 0.0f };
				maxDim = (std::max)(pecone.radius * 3.0f, 3.0f);
			} else {
				center = ped.transform.translate + pBasePos;
				maxDim = (std::max)({ ped.transform.scale.x, ped.transform.scale.y, ped.transform.scale.z, 3.0f });
			}
			float distance = (std::clamp)(maxDim * 1.2f, 5.0f, 25.0f);
			previewCamera_->Focus(center, distance);
		};

		if (!isParticleViewerInitialized_) {
			particleRenderTexture_ = std::make_unique<RenderTexture>();
			particleDepthStencil_ = std::make_unique<DepthStencil>();
			previewParticle_ = std::make_unique<Emitter>();
			previewCamera_ = std::make_unique<Camera>();
			previewGrid_ = std::make_unique<Grid>();

			particleRenderTexture_->Initialize(engine->graphics->GetDevice(), 512, 512, engine->descriptorHeap->GetSrvDescriptorHeap(), engine->descriptorHeap->GetDescriptorSizeSRV());
			particleDepthStencil_->CreateDepthStencil(engine->graphics->GetDevice(), 512, 512);
			previewGrid_->CreateGrid();

			previewParticle_->Initialize();
			if (std::filesystem::exists("Resources/Json/Particle/Snowparticle.json")) {
				previewParticle_->LoadFromJson("Snowparticle");
			} else if (std::filesystem::exists("Resources/Json/Particle/Dustparticle.json")) {
				previewParticle_->LoadFromJson("Dustparticle");
			}

			previewCamera_->GetDebugCameraRef().SetEnableInput(false);
			previewCamera_->SetAspectRatio(1.0f);
			FocusParticleCamera();
			previewCamera_->Update();

			isParticleViewerInitialized_ = true;
		}

		// Draw the ImGui window FIRST so any resource recreations happen BEFORE recording draw calls
		ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
		if (ImGui::Begin(LanguageManager::Tr("Particle Editor"), &showParticleViewer_)) {
			ImGui::Columns(2, "ParticleEditorColumns", true);
			ImGui::SetColumnWidth(0, 532.0f); // Make sure image fits + padding

			ImGui::Text(LanguageManager::Tr("Preview:"));
			ImVec2 vMin = ImGui::GetCursorScreenPos();
			ImGui::Image((ImTextureID)particleRenderTexture_->GetSrvHandleGPU().ptr, ImVec2(512, 512));
			bool isPreviewHovered = ImGui::IsItemHovered();

			// プレビュー画像上での直感的なカメラ操作（Orbit / Pan / Zoom）
			if (isPreviewHovered && !ImGuizmo::IsUsing()) {
				ImGuiIO& io = ImGui::GetIO();
				if (io.MouseWheel != 0.0f) {
					previewCamera_->GetDebugCameraRef().Zoom(io.MouseWheel * 2.0f);
				}
				if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
					ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
					ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
					if (io.KeyShift) {
						previewCamera_->GetDebugCameraRef().Pan(dragDelta.x, dragDelta.y);
					} else {
						previewCamera_->GetDebugCameraRef().Orbit(dragDelta.x, dragDelta.y);
					}
				}
				if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
					ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle);
					ImGui::ResetMouseDragDelta(ImGuiMouseButton_Middle);
					previewCamera_->GetDebugCameraRef().Pan(dragDelta.x, dragDelta.y);
				}
			}

			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
			ImGuizmo::SetRect(vMin.x, vMin.y, 512.0f, 512.0f);
			ImGuizmo::SetGizmoSizeClipSpace(0.15f);

			previewCamera_->Update();
			Matrix4x4 viewMat = previewCamera_->GetViewMatrix();
			Matrix4x4 projMat = previewCamera_->GetProjectionMatrix();

			EmitterData ed = previewParticle_->GetEmitterData();
			EmitterSphere es = previewParticle_->GetEmitterSphere();
			EmitterCircle ec = previewParticle_->GetEmitterCircle();
			EmitterCone econe = previewParticle_->GetEmitterCone();
			EmitterType eType = previewParticle_->GetEmitterType();
			Vector3 basePos = previewParticle_->GetBaseParticleData().transform.translate;
			Matrix4x4 worldMat;
			if (eType == EmitterType::Sphere) {
				worldMat = MakeAffineMatrix(es.translate + basePos, Vector3{ es.radius * 2.0f, es.radius * 2.0f, es.radius * 2.0f }, Vector3{ 0.0f, 0.0f, 0.0f });
			} else if (eType == EmitterType::Circle) {
				worldMat = MakeAffineMatrix(ec.translate + basePos, Vector3{ ec.outerRadius * 2.0f, 1.0f, ec.outerRadius * 2.0f }, ec.rotate);
			} else if (eType == EmitterType::Cone) {
				worldMat = MakeAffineMatrix(econe.translate + basePos, Vector3{ econe.radius * 2.0f, 1.0f, econe.radius * 2.0f }, econe.rotate);
			} else {
				worldMat = MakeAffineMatrix(ed.transform.translate + basePos, ed.transform.scale, ed.transform.rotate);
			}

			static ImGuizmo::OPERATION currentOp = ImGuizmo::TRANSLATE;

			// Draw a wireframe cube, sphere, circle, or cone at Emitter location if enabled
			if (showEmitterCube_) {
				auto drawList = ImGui::GetWindowDrawList();
				Matrix4x4 viewProj = MultiplyMatrix4x4(viewMat, projMat);
				Matrix4x4 wvp = MultiplyMatrix4x4(worldMat, viewProj);
				ImU32 color = IM_COL32(255, 255, 255, 255);

				if (eType == EmitterType::Sphere) {
					const int kSegments = 24;
					float pi = 3.1415926535f;
					auto DrawCirclePlane = [&](auto getCornerPoint) {
						ImVec2 pts[24];
						bool allValid = true;
						for (int i = 0; i < kSegments; ++i) {
							float angle = (2.0f * pi * i) / kSegments;
							Vector3 pt = getCornerPoint(0.5f * std::cos(angle), 0.5f * std::sin(angle));
							float w = pt.x * wvp.m[0][3] + pt.y * wvp.m[1][3] + pt.z * wvp.m[2][3] + wvp.m[3][3];
							if (w < 0.1f) allValid = false;
							Vector3 projected = TransformMatrix(pt, wvp);
							pts[i].x = vMin.x + (projected.x + 1.0f) * 0.5f * 512.0f;
							pts[i].y = vMin.y + (1.0f - projected.y) * 0.5f * 512.0f;
						}
						if (allValid) {
							for (int i = 0; i < kSegments; ++i) {
								drawList->AddLine(pts[i], pts[(i + 1) % kSegments], color, 2.0f);
							}
						}
					};
					DrawCirclePlane([](float c, float s) { return Vector3{ c, s, 0.0f }; });
					DrawCirclePlane([](float c, float s) { return Vector3{ 0.0f, c, s }; });
					DrawCirclePlane([](float c, float s) { return Vector3{ c, 0.0f, s }; });
				} else if (eType == EmitterType::Circle) {
					const int kSegments = 32;
					float pi = 3.1415926535f;
					auto DrawCircleRadius = [&](float radius, ImU32 col) {
						ImVec2 pts[32];
						bool allValid = true;
						for (int i = 0; i < kSegments; ++i) {
							float angle = (2.0f * pi * i) / kSegments;
							Vector3 pt = { radius * std::cos(angle), 0.0f, radius * std::sin(angle) };
							Matrix4x4 rot = Rotation(ec.rotate);
							Vector3 wPos = ec.translate + basePos + TransformMatrix(pt, rot);
							float w = wPos.x * viewProj.m[0][3] + wPos.y * viewProj.m[1][3] + wPos.z * viewProj.m[2][3] + viewProj.m[3][3];
							if (w < 0.1f) allValid = false;
							Vector3 projected = TransformMatrix(wPos, viewProj);
							pts[i].x = vMin.x + (projected.x + 1.0f) * 0.5f * 512.0f;
							pts[i].y = vMin.y + (1.0f - projected.y) * 0.5f * 512.0f;
						}
						if (allValid) {
							for (int i = 0; i < kSegments; ++i) {
								drawList->AddLine(pts[i], pts[(i + 1) % kSegments], col, 2.0f);
							}
						}
					};
					DrawCircleRadius(ec.outerRadius, IM_COL32(0, 255, 255, 255));
					if (ec.innerRadius > 0.01f) {
						DrawCircleRadius(ec.innerRadius, IM_COL32(0, 180, 255, 180));
					}
				} else if (eType == EmitterType::Cone) {
					const int kSegments = 24;
					float pi = 3.1415926535f;
					Matrix4x4 rot = Rotation(econe.rotate);
					ImVec2 pts[24];
					bool allValid = true;
					for (int i = 0; i < kSegments; ++i) {
						float angle = (2.0f * pi * i) / kSegments;
						Vector3 pt = { econe.radius * std::cos(angle), 0.0f, econe.radius * std::sin(angle) };
						Vector3 wPos = econe.translate + basePos + TransformMatrix(pt, rot);
						float w = wPos.x * viewProj.m[0][3] + wPos.y * viewProj.m[1][3] + wPos.z * viewProj.m[2][3] + viewProj.m[3][3];
						if (w < 0.1f) allValid = false;
						Vector3 projected = TransformMatrix(wPos, viewProj);
						pts[i].x = vMin.x + (projected.x + 1.0f) * 0.5f * 512.0f;
						pts[i].y = vMin.y + (1.0f - projected.y) * 0.5f * 512.0f;
					}
					if (allValid) {
						for (int i = 0; i < kSegments; ++i) {
							drawList->AddLine(pts[i], pts[(i + 1) % kSegments], IM_COL32(255, 200, 0, 255), 2.0f);
						}
					}
					float coneHeight = 2.0f;
					float topRadius = econe.radius + std::tan(econe.angle) * coneHeight;
					for (int k = 0; k < 4; ++k) {
						float angle = (pi * 0.5f) * k;
						Vector3 pBase = { econe.radius * std::cos(angle), 0.0f, econe.radius * std::sin(angle) };
						Vector3 pTop = { topRadius * std::cos(angle), coneHeight, topRadius * std::sin(angle) };
						Vector3 wBase = econe.translate + basePos + TransformMatrix(pBase, rot);
						Vector3 wTop = econe.translate + basePos + TransformMatrix(pTop, rot);
						Vector3 prjBase = TransformMatrix(wBase, viewProj);
						Vector3 prjTop = TransformMatrix(wTop, viewProj);
						ImVec2 bScreen = { vMin.x + (prjBase.x + 1.0f) * 0.5f * 512.0f, vMin.y + (1.0f - prjBase.y) * 0.5f * 512.0f };
						ImVec2 tScreen = { vMin.x + (prjTop.x + 1.0f) * 0.5f * 512.0f, vMin.y + (1.0f - prjTop.y) * 0.5f * 512.0f };
						drawList->AddLine(bScreen, tScreen, IM_COL32(255, 200, 0, 180), 1.5f);
					}
				} else {
					Vector3 corners[8] = {
						{-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f},
						{-0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f,  0.5f}, { 0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f}
					};
					int edges[12][2] = {
						{0,1}, {1,2}, {2,3}, {3,0},
						{4,5}, {5,6}, {6,7}, {7,4},
						{0,4}, {1,5}, {2,6}, {3,7}
					};
					// 辺ごとにクリッピングして描画（カメラが近づいても線が消えないようにする）
					for (int i = 0; i < 12; ++i) {
						Vector3 p0 = corners[edges[i][0]];
						Vector3 p1 = corners[edges[i][1]];
						float w0 = p0.x * wvp.m[0][3] + p0.y * wvp.m[1][3] + p0.z * wvp.m[2][3] + wvp.m[3][3];
						float w1 = p1.x * wvp.m[0][3] + p1.y * wvp.m[1][3] + p1.z * wvp.m[2][3] + wvp.m[3][3];
						if (w0 < 0.1f && w1 < 0.1f) continue;
						if (w0 < 0.1f) {
							float t = (0.1f - w0) / (w1 - w0);
							p0 = p0 + (p1 - p0) * t;
						} else if (w1 < 0.1f) {
							float t = (0.1f - w1) / (w0 - w1);
							p1 = p1 + (p0 - p1) * t;
						}
						Vector3 prj0 = TransformMatrix(p0, wvp);
						Vector3 prj1 = TransformMatrix(p1, wvp);
						ImVec2 s0 = { vMin.x + (prj0.x + 1.0f) * 0.5f * 512.0f, vMin.y + (1.0f - prj0.y) * 0.5f * 512.0f };
						ImVec2 s1 = { vMin.x + (prj1.x + 1.0f) * 0.5f * 512.0f, vMin.y + (1.0f - prj1.y) * 0.5f * 512.0f };
						drawList->AddLine(s0, s1, color, 2.0f);
					}
				}
			}
			ImGuizmo::Manipulate(&viewMat.m[0][0], &projMat.m[0][0], currentOp, ImGuizmo::LOCAL, &worldMat.m[0][0]);

			if (ImGuizmo::IsUsing()) {
				float t[3], r[3], s[3];
				ImGuizmo::DecomposeMatrixToComponents(&worldMat.m[0][0], t, r, s);
				float pi = 3.1415926535f;
				if (eType == EmitterType::Sphere) {
					es.translate = { t[0] - basePos.x, t[1] - basePos.y, t[2] - basePos.z };
					es.radius = (s[0] + s[1] + s[2]) / 6.0f;
					previewParticle_->SetEmitterSphere(es);
				} else if (eType == EmitterType::Circle) {
					ec.translate = { t[0] - basePos.x, t[1] - basePos.y, t[2] - basePos.z };
					ec.rotate = { r[0] * pi / 180.0f, r[1] * pi / 180.0f, r[2] * pi / 180.0f };
					ec.outerRadius = (s[0] + s[2]) * 0.25f;
					previewParticle_->SetEmitterCircle(ec);
				} else if (eType == EmitterType::Cone) {
					econe.translate = { t[0] - basePos.x, t[1] - basePos.y, t[2] - basePos.z };
					econe.rotate = { r[0] * pi / 180.0f, r[1] * pi / 180.0f, r[2] * pi / 180.0f };
					econe.radius = (s[0] + s[2]) * 0.25f;
					previewParticle_->SetEmitterCone(econe);
				} else {
					ed.transform.translate = { t[0] - basePos.x, t[1] - basePos.y, t[2] - basePos.z };
					ed.transform.rotate = { r[0] * pi / 180.0f, r[1] * pi / 180.0f, r[2] * pi / 180.0f };
					ed.transform.scale = { s[0], s[1], s[2] };
					previewParticle_->SetEmitterData(ed);
				}
			}

			ImGui::NextColumn();

			// 右側の設定項目をChildウィンドウにして、スクロール時に左側のプレビュー画像が一緒に移動（スクロールアウト）しないように位置を固定する
			ImGui::BeginChild("ParticleControls", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

			ImGui::Text(LanguageManager::Tr("Controls:"));
			ImGui::Checkbox(LanguageManager::Tr("Show Grid"), &showGridInViewer_);
			ImGui::SameLine();
			ImGui::Checkbox(LanguageManager::Tr("Show Emitter Cube"), &showEmitterCube_);

			// プレビュー再生コントロール
			ImGui::Separator();
			ImGui::Text(LanguageManager::Tr("Playback:"));
			if (ImGui::Button(LanguageManager::Tr("Restart / Emit"))) {
				previewParticle_->ClearParticles();
				if (previewParticle_->GetLoop()) {
					previewParticle_->Emit();
				} else {
					previewParticle_->TriggerBurst();
				}
			}
			ImGui::SameLine();
			static bool autoReplayOneShot = true;
			static float oneShotTimer = 0.0f;
			if (!previewParticle_->GetLoop()) {
				ImGui::Checkbox(LanguageManager::Tr("Auto Replay (One-Shot)"), &autoReplayOneShot);
				if (autoReplayOneShot) {
					oneShotTimer += 1.0f / 60.0f;
					if (previewParticle_->GetEffectDefinition()->GetEffectDefinitionNum() == 0 || oneShotTimer >= 2.0f) {
						previewParticle_->TriggerBurst();
						oneShotTimer = 0.0f;
					}
				}
			} else {
				bool isStop = previewParticle_->GetStop();
				if (ImGui::Checkbox(LanguageManager::Tr("Pause Emission"), &isStop)) {
					previewParticle_->SetStop(isStop);
				}
			}
			
			ImGui::Separator();
			ImGui::Text(LanguageManager::Tr("Gizmo Operation:"));
			if (ImGui::RadioButton(LanguageManager::Tr("Translate"), currentOp == ImGuizmo::TRANSLATE)) currentOp = ImGuizmo::TRANSLATE;
			ImGui::SameLine();
			if (ImGui::RadioButton(LanguageManager::Tr("Rotate"), currentOp == ImGuizmo::ROTATE)) currentOp = ImGuizmo::ROTATE;
			ImGui::SameLine();
			if (ImGui::RadioButton(LanguageManager::Tr("Scale"), currentOp == ImGuizmo::SCALE)) currentOp = ImGuizmo::SCALE;

			ImGui::Separator();
			ImGui::Text(LanguageManager::Tr("Camera:"));
			if (ImGui::Button(LanguageManager::Tr("Focus Emitter"))) {
				FocusParticleCamera();
			}
			ImGui::SameLine();
			if (ImGui::Button(LanguageManager::Tr("Reset to Origin"))) {
				previewCamera_->Focus({ 0.0f, 0.0f, 0.0f }, 15.0f);
			}

			Vector3 camTarget = previewCamera_->GetTarget();
			if (ImGui::DragFloat3(LanguageManager::Tr("Camera Target"), &camTarget.x, 0.1f)) {
				previewCamera_->SetTarget(camTarget);
			}
			Transform& camT = const_cast<Transform&>(previewCamera_->GetTransform());
			if (ImGui::DragFloat3(LanguageManager::Tr("Camera Pos"), &camT.translate.x, 0.1f)) {
				previewCamera_->SetTransform(camT);
			}
			ImGui::TextDisabled(LanguageManager::Tr("(Preview: RMB: Orbit, Shift+RMB/MMB: Pan, Wheel: Zoom)"));

			ImGui::Separator();
			Vector3 prevPos = previewParticle_->GetPosition();
			float prevScale = previewParticle_->GetEmitterData().transform.scale.x;
			EmitterType prevType = previewParticle_->GetEmitterType();

			previewParticle_->ImGui();

			Vector3 newPos = previewParticle_->GetPosition();
			float newScale = previewParticle_->GetEmitterData().transform.scale.x;
			EmitterType newType = previewParticle_->GetEmitterType();
			if ((std::abs(prevPos.x - newPos.x) > 0.01f ||
				 std::abs(prevPos.y - newPos.y) > 0.01f ||
				 std::abs(prevPos.z - newPos.z) > 0.01f ||
				 std::abs(prevScale - newScale) > 0.01f ||
				 prevType != newType) && !ImGuizmo::IsUsing()) {
				FocusParticleCamera();
			}

			ImGui::EndChild();

			ImGui::Columns(1);
		}
		ImGui::End();

		// Draw the particle into the render texture AFTER ImGui has processed (and potentially recreated resources)
		previewCamera_->Update();
		Matrix4x4 projMat = previewCamera_->GetProjectionMatrix();
		previewParticle_->SetCustomProjectionMatrix(projMat);
		previewParticle_->Update(previewCamera_->GetViewMatrix());

		auto cmdList = engine->command->GetCommandList();
		ID3D12DescriptorHeap* descriptorHeaps[] = { engine->descriptorHeap->GetSrvDescriptorHeap() };
		cmdList->SetDescriptorHeaps(1, descriptorHeaps);

		particleRenderTexture_->TransitionToRenderTarget(cmdList);
		particleRenderTexture_->Clear(cmdList);
		particleDepthStencil_->TransitionToDepthWrite(cmdList);
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = particleRenderTexture_->GetRtvHandle();
		particleDepthStencil_->SetDSV(cmdList, &rtvHandle);

		// Store old viewport/scissor and set new ones for 512x512
		D3D12_VIEWPORT vp = { 0.0f, 0.0f, 512.0f, 512.0f, 0.0f, 1.0f };
		D3D12_RECT scissor = { 0, 0, 512, 512 };
		cmdList->RSSetViewports(1, &vp);
		cmdList->RSSetScissorRects(1, &scissor);

		engine->draw->SetCamera(previewCamera_.get());
		if (showGridInViewer_) {
			previewGrid_->SettingWvp(previewCamera_->GetViewMatrix(), &projMat);
			engine->draw->DrawGrid(previewGrid_.get());
		}

		static int s_viewerFrameCount = 0;
		if (s_viewerFrameCount++ % 120 == 0) {
			EmitterData ped = previewParticle_->GetEmitterData();
			const Transform& pcamT = previewCamera_->GetTransform();
			LOG_INFO(std::format("ParticleViewer: useGpu={}, emitterPos=({:.1f},{:.1f},{:.1f}), emitterScale=({:.1f},{:.1f},{:.1f}), camPos=({:.1f},{:.1f},{:.1f})",
				previewParticle_->GetUseGpuParticle(),
				ped.transform.translate.x, ped.transform.translate.y, ped.transform.translate.z,
				ped.transform.scale.x, ped.transform.scale.y, ped.transform.scale.z,
				pcamT.translate.x, pcamT.translate.y, pcamT.translate.z));
		}

		previewParticle_->Draw(*engine->draw);

		particleRenderTexture_->TransitionToShaderResource(cmdList);

		// Restore engine's main render target
		D3D12_CPU_DESCRIPTOR_HANDLE mainRtv = engine->GetRenderTexture()->GetRtvHandle();
		engine->depthStencil->SetDSV(cmdList, &mainRtv);
		cmdList->RSSetViewports(1, engine->viewportScissor->GetViewport());
		cmdList->RSSetScissorRects(1, engine->viewportScissor->GetScissorRect());
	}

	// Model Viewer Window
	if (showModelViewer_) {
		if (!isModelViewerInitialized_) {
			modelRenderTexture_ = std::make_unique<RenderTexture>();
			modelDepthStencil_ = std::make_unique<DepthStencil>();
			previewModel_ = std::make_unique<Model>();
			modelCamera_ = std::make_unique<Camera>();
			modelGrid_ = std::make_unique<Grid>();

			modelRenderTexture_->Initialize(engine->graphics->GetDevice(), 512, 512, engine->descriptorHeap->GetSrvDescriptorHeap(), engine->descriptorHeap->GetDescriptorSizeSRV());
			modelDepthStencil_->CreateDepthStencil(engine->graphics->GetDevice(), 512, 512);
			modelGrid_->CreateGrid();

			modelCamera_->SetAspectRatio(1.0f);
			Transform camT = { {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {0.0f, 2.0f, -10.0f} };
			modelCamera_->SetTransform(camT);
			modelCamera_->Update();

			isModelViewerInitialized_ = true;
		}

		ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
		if (ImGui::Begin(LanguageManager::Tr("Object Editor"), &showModelViewer_)) {
			ImGui::Columns(2, "ModelEditorColumns", true);
			ImGui::SetColumnWidth(0, 532.0f);

			ImGui::Text(LanguageManager::Tr("Preview:"));
			ImVec2 vMin = ImGui::GetCursorScreenPos();
			ImGui::Image((ImTextureID)modelRenderTexture_->GetSrvHandleGPU().ptr, ImVec2(512, 512));
			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_FILE")) {
					const char* dropPath = (const char*)payload->Data;
					std::string pathStr = dropPath;
					if (pathStr.length() > 4 && (pathStr.substr(pathStr.length() - 4) == ".obj" || pathStr.substr(pathStr.length() - 5) == ".gltf")) {
						currentModelPath_ = pathStr;
						std::filesystem::path p(pathStr);
						std::string dir = p.parent_path().string() + "/";
						std::string file = p.filename().string();
						ModelData data;
						if (file.find(".obj") != std::string::npos) {
							data = LoadObjFile(dir, file);
						} else {
							data = AssimpLoadObjFile(dir, file);
						}
						previewModel_->Initialize(data);
						previewModel_->name_ = "Preview Model";
					}
				}
				ImGui::EndDragDropTarget();
			}

			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
			ImGuizmo::SetRect(vMin.x, vMin.y, 512.0f, 512.0f);
			ImGuizmo::SetGizmoSizeClipSpace(0.15f);

			modelCamera_->Update();
			Matrix4x4 viewMat = modelCamera_->GetViewMatrix();
			Matrix4x4 projMat = modelCamera_->GetProjectionMatrix();

			Transform t = previewModel_->GetTransform();
			Matrix4x4 worldMat = MakeAffineMatrix(t.translate, t.scale, t.rotate);

			static ImGuizmo::OPERATION currentOpModel = ImGuizmo::TRANSLATE;

			ImGuizmo::Manipulate(&viewMat.m[0][0], &projMat.m[0][0], currentOpModel, ImGuizmo::LOCAL, &worldMat.m[0][0]);

			if (ImGuizmo::IsUsing()) {
				float tr[3], r[3], s[3];
				ImGuizmo::DecomposeMatrixToComponents(&worldMat.m[0][0], tr, r, s);
				float pi = 3.1415926535f;
				t.translate = { tr[0], tr[1], tr[2] };
				t.rotate = { r[0] * pi / 180.0f, r[1] * pi / 180.0f, r[2] * pi / 180.0f };
				t.scale = { s[0], s[1], s[2] };
				previewModel_->SetTransform(t);
			}

			ImGui::NextColumn();

			ImGui::BeginChild("ModelControls", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

			ImGui::Text(LanguageManager::Tr("Controls:"));
			ImGui::Checkbox(LanguageManager::Tr("Show Grid"), &showGridInModelViewer_);

			ImGui::Separator();
			ImGui::Text(LanguageManager::Tr("Gizmo Operation:"));
			if (ImGui::RadioButton(LanguageManager::Tr("Translate##m"), currentOpModel == ImGuizmo::TRANSLATE)) currentOpModel = ImGuizmo::TRANSLATE;
			ImGui::SameLine();
			if (ImGui::RadioButton(LanguageManager::Tr("Rotate##m"), currentOpModel == ImGuizmo::ROTATE)) currentOpModel = ImGuizmo::ROTATE;
			ImGui::SameLine();
			if (ImGui::RadioButton(LanguageManager::Tr("Scale##m"), currentOpModel == ImGuizmo::SCALE)) currentOpModel = ImGuizmo::SCALE;

			Transform& camT2 = const_cast<Transform&>(modelCamera_->GetTransform());
			if (ImGui::DragFloat3(LanguageManager::Tr("Camera Pos##m"), &camT2.translate.x, 0.1f)) {
				modelCamera_->SetTransform(camT2);
			}

			ImGui::Separator();
			ImGui::TextWrapped(LanguageManager::Tr("Drop .obj or .gltf file here to preview"));
			ImGui::TextDisabled("%s", currentModelPath_.empty() ? "None" : currentModelPath_.c_str());

			if (!currentModelPath_.empty()) {
				ImGui::Separator();
				if (ImGui::Button(LanguageManager::Tr("Clear Model"))) {
					currentModelPath_ = "";
					previewModel_ = std::make_unique<Model>(); // Reset model
				}
				ImGui::Separator();
				previewModel_->ImGui();
			}

			ImGui::EndChild();
			ImGui::Columns(1);
		}
		ImGui::End();

		modelCamera_->Update();
		Matrix4x4 modelProjMat = modelCamera_->GetProjectionMatrix();
		previewModel_->Update(modelCamera_->GetViewMatrix());
		if (!currentModelPath_.empty()) {
			previewModel_->SettingWvp(modelCamera_->GetViewMatrix(), &modelProjMat);
		}

		auto cmdList = engine->command->GetCommandList();
		modelRenderTexture_->TransitionToRenderTarget(cmdList);
		modelRenderTexture_->Clear(cmdList);
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = modelRenderTexture_->GetRtvHandle();
		modelDepthStencil_->SetDSV(cmdList, &rtvHandle);

		D3D12_VIEWPORT vp = { 0.0f, 0.0f, 512.0f, 512.0f, 0.0f, 1.0f };
		D3D12_RECT scissor = { 0, 0, 512, 512 };
		cmdList->RSSetViewports(1, &vp);
		cmdList->RSSetScissorRects(1, &scissor);

		engine->draw->SetCamera(modelCamera_.get());
		if (showGridInModelViewer_) {
			modelGrid_->SettingWvp(modelCamera_->GetViewMatrix(), &modelProjMat);
			engine->draw->DrawGrid(modelGrid_.get());
		}
		if (!currentModelPath_.empty()) {
			engine->draw->DrawModel(previewModel_.get());
		}

		modelRenderTexture_->TransitionToShaderResource(cmdList);

		// Restore main
		D3D12_CPU_DESCRIPTOR_HANDLE mainRtv = engine->GetRenderTexture()->GetRtvHandle();
		engine->depthStencil->SetDSV(cmdList, &mainRtv);
		cmdList->RSSetViewports(1, engine->viewportScissor->GetViewport());
		cmdList->RSSetScissorRects(1, engine->viewportScissor->GetScissorRect());
	}

	// Game View Window
	if (showGameViewWindow_) {
		if (!isGameViewInitialized_) {
			for (int i = 0; i < 2; ++i) {
				gameViewRenderTextures_[i] = std::make_unique<RenderTexture>();
				gameViewRenderTextures_[i]->Initialize(
					engine->graphics->GetDevice(), 1280, 720,
					engine->descriptorHeap->GetSrvDescriptorHeap(),
					engine->descriptorHeap->GetDescriptorSizeSRV());
			}
			gameViewDepthStencil_ = std::make_unique<DepthStencil>();
			gameViewDepthStencil_->CreateDepthStencil(
				engine->graphics->GetDevice(), 1280, 720,
				engine->descriptorHeap->GetSrvDescriptorHeap(),
				engine->descriptorHeap->GetDescriptorSizeSRV());
			isGameViewInitialized_ = true;
		}

		ImGui::SetNextWindowSize(ImVec2(800, 450), ImGuiCond_FirstUseEver);
		if (ImGui::Begin(LanguageManager::Tr("Game View"), &showGameViewWindow_)) {
			ImVec2 availSize = ImGui::GetContentRegionAvail();
			ImVec2 imageSize = availSize;
			ImVec2 cursorStart = ImGui::GetCursorPos();

			float targetAspect = 1280.0f / 720.0f;
			float availAspect = availSize.x / availSize.y;

			if (availAspect > targetAspect) {
				imageSize.y = availSize.y;
				imageSize.x = availSize.y * targetAspect;
			} else {
				imageSize.x = availSize.x;
				imageSize.y = availSize.x / targetAspect;
			}

			float offsetX = (availSize.x - imageSize.x) * 0.5f;
			float offsetY = (availSize.y - imageSize.y) * 0.5f;
			ImGui::SetCursorPos(ImVec2(cursorStart.x + offsetX, cursorStart.y + offsetY));

			ImGui::Image((ImTextureID)gameViewRenderTextures_[gameViewFinalRTIndex_]->GetSrvHandleGPU().ptr, imageSize);
		}
		ImGui::End();

		// Draw into Game View Render Texture
		auto cmdList = engine->command->GetCommandList();
		gameViewRenderTextures_[0]->TransitionToRenderTarget(cmdList);
		gameViewRenderTextures_[0]->Clear(cmdList);
		gameViewDepthStencil_->TransitionToDepthWrite(cmdList);
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = gameViewRenderTextures_[0]->GetRtvHandle();
		gameViewDepthStencil_->SetDSV(cmdList, &rtvHandle);

		D3D12_VIEWPORT vp = { 0.0f, 0.0f, 1280.0f, 720.0f, 0.0f, 1.0f };
		D3D12_RECT scissor = { 0, 0, 1280, 720 };
		cmdList->RSSetViewports(1, &vp);
		cmdList->RSSetScissorRects(1, &scissor);

		ID3D12DescriptorHeap* descriptorHeaps[] = { engine->descriptorHeap->GetSrvDescriptorHeap() };
		cmdList->SetDescriptorHeaps(1, descriptorHeaps);

		if (s_gameViewDrawCallback_) {
			s_gameViewDrawCallback_(*engine->draw);

			gameViewRenderTextures_[0]->TransitionToShaderResource(cmdList);
			gameViewDepthStencil_->TransitionToShaderResource(cmdList);

			// ポストエフェクト適用 (Ping-pong 描画)
			int currentRT = 0;
			int nextRT = 1;

			const auto& postEffects = engine->GetPostEffects();
			for (auto& effect : postEffects) {
				if (effect->IsNormalEffect()) continue;

				gameViewRenderTextures_[nextRT]->TransitionToRenderTarget(cmdList);
				D3D12_CPU_DESCRIPTOR_HANDLE nextRtvHandle = gameViewRenderTextures_[nextRT]->GetRtvHandle();
				cmdList->OMSetRenderTargets(1, &nextRtvHandle, false, nullptr);

				cmdList->RSSetViewports(1, &vp);
				cmdList->RSSetScissorRects(1, &scissor);

				cmdList->SetDescriptorHeaps(1, descriptorHeaps);

				engine->draw->DrawPostEffect(
					gameViewRenderTextures_[currentRT]->GetSrvHandleGPU(),
					effect->GetActiveShaderName(),
					effect.get(),
					gameViewDepthStencil_->GetSrvHandleGPU());

				gameViewRenderTextures_[nextRT]->TransitionToShaderResource(cmdList);

				std::swap(currentRT, nextRT);
			}

			// Game View UI (HUD) 描画（ポストエフェクト後に描画することでUIが汚れないようにする）
			if (s_gameViewUIDrawCallback_) {
				gameViewRenderTextures_[currentRT]->TransitionToRenderTarget(cmdList);
				gameViewDepthStencil_->TransitionToDepthWrite(cmdList);
				D3D12_CPU_DESCRIPTOR_HANDLE uiRtvHandle = gameViewRenderTextures_[currentRT]->GetRtvHandle();
				gameViewDepthStencil_->SetDSV(cmdList, &uiRtvHandle);

				cmdList->RSSetViewports(1, &vp);
				cmdList->RSSetScissorRects(1, &scissor);

				cmdList->SetDescriptorHeaps(1, descriptorHeaps);

				s_gameViewUIDrawCallback_(*engine->draw);

				gameViewRenderTextures_[currentRT]->TransitionToShaderResource(cmdList);
				gameViewDepthStencil_->TransitionToShaderResource(cmdList);
			}

			gameViewFinalRTIndex_ = currentRT;
		} else {
			gameViewRenderTextures_[0]->TransitionToShaderResource(cmdList);
			gameViewFinalRTIndex_ = 0;
		}

		// Restore engine's main render target
		D3D12_CPU_DESCRIPTOR_HANDLE mainRtv = engine->GetRenderTexture()->GetRtvHandle();
		engine->depthStencil->SetDSV(cmdList, &mainRtv);
		cmdList->RSSetViewports(1, engine->viewportScissor->GetViewport());
		cmdList->RSSetScissorRects(1, engine->viewportScissor->GetScissorRect());
		cmdList->SetDescriptorHeaps(1, descriptorHeaps);
	}

#endif
}

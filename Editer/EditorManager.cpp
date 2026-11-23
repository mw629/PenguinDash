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
#include "Command/CommandManager.h"
#include <unordered_map>
#include <algorithm>

#include "../MatchaEngine/Graphics/RenderTexture.h"
#include "../MatchaEngine/Graphics/DepthStencil.h"
#include "../MatchaEngine/GameObjects/Effect/Emitter.h"
#include "../MatchaEngine/GameObjects/Effect/EffectDefinition.h"
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
    if (!engine || !engine->GetGraphics()) return;
    IDXGIAdapter4* adapter = engine->GetGraphics()->GetUseAdapter();
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
		if (ImGui::BeginMenu(LanguageManager::Tr("Edit"))) {
			std::string undoLabel = CommandManager::GetInstance().CanUndo()
				? (std::string(LanguageManager::Tr("Undo")) + " (" + CommandManager::GetInstance().GetUndoCommandName() + ")")
				: std::string(LanguageManager::Tr("Undo"));
			if (ImGui::MenuItem(undoLabel.c_str(), "Ctrl+Z", false, CommandManager::GetInstance().CanUndo())) {
				CommandManager::GetInstance().Undo();
			}

			std::string redoLabel = CommandManager::GetInstance().CanRedo()
				? (std::string(LanguageManager::Tr("Redo")) + " (" + CommandManager::GetInstance().GetRedoCommandName() + ")")
				: std::string(LanguageManager::Tr("Redo"));
			if (ImGui::MenuItem(redoLabel.c_str(), "Ctrl+Y", false, CommandManager::GetInstance().CanRedo())) {
				CommandManager::GetInstance().Redo();
			}

			ImGui::Separator();
			if (ImGui::MenuItem(LanguageManager::Tr("Clear History"), nullptr, false, CommandManager::GetInstance().CanUndo() || CommandManager::GetInstance().CanRedo())) {
				CommandManager::GetInstance().Clear();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu(LanguageManager::Tr("Window"))) {
			ImGui::MenuItem(LanguageManager::Tr("Debug Info"), nullptr, &showFinalWindow_);
			ImGui::MenuItem(LanguageManager::Tr("Resources"), nullptr, &showResourcesWindow_);
			ImGui::MenuItem(LanguageManager::Tr("Logs"), nullptr, &showLogsWindow_);
			ImGui::MenuItem(LanguageManager::Tr("Particle Editor"), nullptr, &showParticleViewer_);
			ImGui::MenuItem(LanguageManager::Tr("Object Editor"), nullptr, &showModelViewer_);
			ImGui::MenuItem(LanguageManager::Tr("Game"), nullptr, &showGameViewWindow_);
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
		ImGui::DockBuilderDockWindow("Game", dock_main_id);
		ImGui::DockBuilderDockWindow("Game View", dock_main_id);
		ImGui::DockBuilderDockWindow("ゲーム", dock_main_id);
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

			if (ImGui::BeginTabItem(LanguageManager::Tr("Design Patterns"))) {
				ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "=== 1. Object Pool Pattern ===");
				struct DemoParticle {
					int id = 0;
					bool active = false;
					void Reset() { active = false; id = 0; }
				};
				static Matcha::ObjectPool<DemoParticle> s_demoPool(10);
				static std::vector<DemoParticle*> s_acquiredList;

				ImGui::Text("Capacity: %zu | Active: %zu | Free: %zu",
					s_demoPool.GetCapacity(), s_demoPool.GetActiveCount(), s_demoPool.GetFreeCount());
				float poolUsage = s_demoPool.GetCapacity() > 0 ? (float)s_demoPool.GetActiveCount() / (float)s_demoPool.GetCapacity() : 0.0f;
				ImGui::ProgressBar(poolUsage, ImVec2(-1.0f, 0.0f));

				if (ImGui::Button("Acquire (貸出)")) {
					auto* p = s_demoPool.Acquire();
					if (p) {
						p->active = true;
						p->id = static_cast<int>(s_acquiredList.size() + 1);
						s_acquiredList.push_back(p);
					}
				}
				ImGui::SameLine();
				if (ImGui::Button("Release (返却)") && !s_acquiredList.empty()) {
					auto* p = s_acquiredList.back();
					s_acquiredList.pop_back();
					s_demoPool.Release(p);
				}
				ImGui::SameLine();
				if (ImGui::Button("Release All (一括返却)")) {
					s_demoPool.ReleaseAll();
					s_acquiredList.clear();
				}

				ImGui::Separator();
				ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "=== 2. Dirty Flag Pattern ===");
				GameObject* sel = GameObject::GetSelectedObject();
				if (sel) {
					ImGui::Text("Selected: %s", sel->GetName().c_str());
					ImGui::Text("Is WorldMatrix Dirty: %s", sel->IsDirty() ? "TRUE (Needs recalculation)" : "FALSE (Cached)");
					if (ImGui::Button("Force Mark Dirty")) {
						sel->MarkDirty();
					}
				} else {
					ImGui::TextDisabled("Select an object to inspect Dirty Flag.");
				}

				ImGui::Separator();
				ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "=== 3. Command Pattern (Undo / Redo) ===");
				CommandManager& cmdMgr = CommandManager::GetInstance();
				ImGui::Text("Undo Stack: %zu | Redo Stack: %zu", cmdMgr.GetUndoCount(), cmdMgr.GetRedoCount());
				ImGui::Text("Next Undo: %s", cmdMgr.CanUndo() ? cmdMgr.GetUndoCommandName().c_str() : "(None)");
				ImGui::Text("Next Redo: %s", cmdMgr.CanRedo() ? cmdMgr.GetRedoCommandName().c_str() : "(None)");
				if (ImGui::Button("Undo (Ctrl+Z)") && cmdMgr.CanUndo()) {
					cmdMgr.Undo();
				}
				ImGui::SameLine();
				if (ImGui::Button("Redo (Ctrl+Y)") && cmdMgr.CanRedo()) {
					cmdMgr.Redo();
				}
				ImGui::SameLine();
				if (ImGui::Button("Clear History")) {
					cmdMgr.Clear();
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

	ImGui::SameLine();
	// --- デバッグカメラ操作 ---
	Camera *sceneCamera = engine->GetDraw()->GetCamera();
	if (sceneCamera) {
		bool isDebug = sceneCamera->GetDebugCamera();
		if (ImGui::Checkbox(LanguageManager::Tr("Debug Camera"), &isDebug)) {
			sceneCamera->SetDebugCamera(isDebug);
		}
		if (isDebug) {
			ImGui::SameLine();
			if (ImGui::Button(LanguageManager::Tr("Reset View"))) {
				sceneCamera->ResetDebugCameraToGameCamera();
			}
		}
	}

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
		bool isSceneHovered = ImGui::IsItemHovered();

		// Sceneウィンドウ上での直感的なデバッグカメラ操作 (Orbit / Pan / Zoom)
		if (sceneCamera) {
			sceneCamera->GetDebugCameraRef().SetEnableInput(false); // エディタ内ではImGuiマウス操作に専念
			if (sceneCamera->GetDebugCamera() && isSceneHovered && !ImGuizmo::IsUsing()) {
				ImGuiIO &io = ImGui::GetIO();
				if (io.MouseWheel != 0.0f) {
					sceneCamera->GetDebugCameraRef().Zoom(io.MouseWheel * 2.0f);
				}
				// 右ドラッグ または Alt+左ドラッグ で Orbit (視点回転)
				if (ImGui::IsMouseDragging(ImGuiMouseButton_Right) || (io.KeyAlt && ImGui::IsMouseDragging(ImGuiMouseButton_Left))) {
					ImGuiMouseButton btn = ImGui::IsMouseDragging(ImGuiMouseButton_Right) ? ImGuiMouseButton_Right : ImGuiMouseButton_Left;
					ImVec2 dragDelta = ImGui::GetMouseDragDelta(btn);
					ImGui::ResetMouseDragDelta(btn);
					if (io.KeyShift) {
						sceneCamera->GetDebugCameraRef().Pan(dragDelta.x, dragDelta.y);
					} else {
						sceneCamera->GetDebugCameraRef().Orbit(dragDelta.x, dragDelta.y);
					}
				}
				// 中ボタンドラッグ で Pan (平行移動)
				if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
					ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle);
					ImGui::ResetMouseDragDelta(ImGuiMouseButton_Middle);
					sceneCamera->GetDebugCameraRef().Pan(dragDelta.x, dragDelta.y);
				}
			}
		}

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
			float distance = (std::clamp)(maxDim * 1.5f, 5.0f, 150.0f);
			previewCamera_->Focus(center, distance);
		};

		if (!isParticleViewerInitialized_) {
			particleRenderTexture_ = std::make_unique<RenderTexture>();
			particleDepthStencil_ = std::make_unique<DepthStencil>();
			previewParticle_ = std::make_unique<Emitter>();
			previewCamera_ = std::make_unique<Camera>();
			previewGrid_ = std::make_unique<Grid>();

			particleRenderTexture_->Initialize(engine->GetGraphics()->GetDevice(), 512, 512, engine->GetDescriptorHeap()->GetSrvDescriptorHeap(), engine->GetDescriptorHeap()->GetDescriptorSizeSRV());
			particleDepthStencil_->CreateDepthStencil(engine->GetGraphics()->GetDevice(), 512, 512);
			previewGrid_->CreateGrid();

			previewParticle_->Initialize();
			if (std::filesystem::exists("Resources/Json/Particle/Snowparticle.json")) {
				previewParticle_->LoadFromJson("Snowparticle");
			} else if (std::filesystem::exists("Resources/Json/Particle/Dustparticle.json")) {
				previewParticle_->LoadFromJson("Dustparticle");
			}

			previewCamera_->GetDebugCameraRef().SetEnableInput(false);
			previewCamera_->SetAspectRatio(1.0f);
			previewCamera_->SetDebugCamera(true);
			FocusParticleCamera();
			previewCamera_->Update();

			isParticleViewerInitialized_ = true;
		}

		if (!isSingleParticleViewerInitialized_) {
			singleParticleRenderTexture_ = std::make_unique<RenderTexture>();
			singleParticleDepthStencil_ = std::make_unique<DepthStencil>();
			singleParticleCamera_ = std::make_unique<Camera>();
			singleParticleEffectDef_ = std::make_unique<EffectDefinition>();

			singleParticleRenderTexture_->Initialize(engine->GetGraphics()->GetDevice(), 256, 256, engine->GetDescriptorHeap()->GetSrvDescriptorHeap(), engine->GetDescriptorHeap()->GetDescriptorSizeSRV());
			singleParticleDepthStencil_->CreateDepthStencil(engine->GetGraphics()->GetDevice(), 256, 256);

			singleParticleCamera_->GetDebugCameraRef().SetEnableInput(false);
			singleParticleCamera_->SetAspectRatio(1.0f);
			singleParticleCamera_->SetDebugCamera(true);
			singleParticleCamera_->Focus(Vector3{ 0.0f, 0.0f, 0.0f }, 2.5f);
			singleParticleCamera_->Update();

			singleParticleEffectDef_->Initialize(EffectShape::Plane);
			isSingleParticleViewerInitialized_ = true;
		}

		if (previewCamera_ && !previewCamera_->GetDebugCamera()) {
			previewCamera_->SetDebugCamera(true);
			FocusParticleCamera();
		}

		// Draw the ImGui window FIRST so any resource recreations happen BEFORE recording draw calls
		ImGui::SetNextWindowSize(ImVec2(960, 800), ImGuiCond_FirstUseEver);
		if (ImGui::Begin(LanguageManager::Tr("Particle Editor"), &showParticleViewer_)) {
			// 左カラム: プレビュー表示エリア
			ImGui::BeginGroup();

			ImGui::Text(LanguageManager::Tr("Preview:"));
			ImVec2 vMin = ImGui::GetCursorScreenPos();
			ImGui::Image((ImTextureID)particleRenderTexture_->GetSrvHandleGPU().ptr, ImVec2(512, 512));
			bool isPreviewHovered = ImGui::IsItemHovered();

			// プレビュー画像の下に出ているパーティクル1個の見た目を表示
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Text("%s", LanguageManager::Tr("Single Particle View:"));

			if (singleParticleRenderTexture_) {
				ImGui::Image((ImTextureID)singleParticleRenderTexture_->GetSrvHandleGPU().ptr, ImVec2(200, 200));
			}

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

			ImGui::EndGroup(); // 左カラム終了

			ImGui::SameLine(0.0f, 16.0f); // 右上から開始！

			// 右カラム: 操作・設定エリア（右上からウィンドウ下端まで）
			ImGui::BeginGroup();
			ImGui::BeginChild("ParticleControls", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

			// ========================================================
			// 1. プリセット / ファイル管理 (常に最上部で一目で操作可能)
			// ========================================================
			if (ImGui::CollapsingHeader(LanguageManager::Tr("Presets & File"), ImGuiTreeNodeFlags_DefaultOpen)) {
				std::filesystem::path dir = "Resources/Json/Particle";
				std::vector<std::string> jsonFiles;
				if (std::filesystem::exists(dir)) {
					for (const auto& entry : std::filesystem::directory_iterator(dir)) {
						if (entry.path().extension() == ".json") {
							jsonFiles.push_back(entry.path().filename().string());
						}
					}
				}

				char* saveBuf = previewParticle_->GetSaveFileNameBuffer();
				size_t bufSize = previewParticle_->GetSaveFileNameBufferSize();

				if (ImGui::BeginCombo(LanguageManager::Tr("Saved Particles"), saveBuf)) {
					for (const auto& file : jsonFiles) {
						std::string nameWithoutExt = file;
						size_t extPos = nameWithoutExt.find(".json");
						if (extPos != std::string::npos) {
							nameWithoutExt = nameWithoutExt.substr(0, extPos);
						}

						bool is_selected = (std::string(saveBuf) == nameWithoutExt);
						if (ImGui::Selectable(file.c_str(), is_selected)) {
							previewParticle_->SetSaveFileName(nameWithoutExt);
							previewParticle_->LoadFromJson(nameWithoutExt);
							FocusParticleCamera();
						}
						if (is_selected) {
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}

				ImGui::InputText(LanguageManager::Tr("File Name"), saveBuf, bufSize);
				if (ImGui::Button(LanguageManager::Tr("Load JSON"))) {
					previewParticle_->LoadFromJson(saveBuf);
					FocusParticleCamera();
				}
				ImGui::SameLine();
				if (ImGui::Button(LanguageManager::Tr("Save JSON"))) {
					previewParticle_->SaveToJson(saveBuf);
				}
				ImGui::SameLine();
				if (ImGui::Button(LanguageManager::Tr("Clear Particles"))) {
					previewParticle_->ClearParticles();
				}
			}

			// ========================================================
			// 2. プレビュー再生コントロール (クイックアクセス)
			// ========================================================
			ImGui::Separator();
			ImGui::Text("%s:", LanguageManager::Tr("Playback"));
			if (ImGui::Button(LanguageManager::Tr("Restart / Emit"))) {
				previewParticle_->ClearParticles();
				if (previewParticle_->GetLoop()) {
					previewParticle_->Emit();
				} else {
					previewParticle_->TriggerBurst();
				}
			}
			ImGui::SameLine();
			bool isLoop = previewParticle_->GetLoop();
			if (ImGui::Checkbox(LanguageManager::Tr("Loop Emission"), &isLoop)) {
				previewParticle_->SetLoop(isLoop);
			}
			ImGui::SameLine();
			if (isLoop) {
				bool isStop = previewParticle_->GetStop();
				if (ImGui::Checkbox(LanguageManager::Tr("Pause Emission"), &isStop)) {
					previewParticle_->SetStop(isStop);
				}
			} else {
				static bool autoReplayOneShot = true;
				static float oneShotTimer = 0.0f;
				ImGui::Checkbox(LanguageManager::Tr("Auto Replay"), &autoReplayOneShot);
				if (autoReplayOneShot) {
					oneShotTimer += Engine::kDefaultDeltaTime;
					if (previewParticle_->GetEffectDefinition()->GetEffectDefinitionNum() == 0 || oneShotTimer >= kOneShotReplayInterval) {
						previewParticle_->TriggerBurst();
						oneShotTimer = 0.0f;
					}
				}
			}
			ImGui::SameLine();
			ImGui::TextDisabled("| %s: %d", LanguageManager::Tr("Active Particles"), previewParticle_->GetEffectDefinition() ? previewParticle_->GetEffectDefinition()->GetEffectDefinitionNum() : 0);

			// ========================================================
			// 3. カテゴリ別タブバー (直感的なパラメータ編集)
			// ========================================================
			ImGui::Separator();
			if (ImGui::BeginTabBar("ParticleEditorTabs", ImGuiTabBarFlags_None)) {
				// ----------------------------------------------------
				// TAB 1: 見た目・マテリアル (Visual & Material)
				// ----------------------------------------------------
				if (ImGui::BeginTabItem(LanguageManager::Tr("Visual & Material"))) {
					ImGui::Spacing();

					const char* shapes[] = { "Plane", "Cylinder", "Ring", "Cube" };
					int curShape = static_cast<int>(previewParticle_->GetShape());
					if (ImGui::Combo(LanguageManager::Tr("Shape"), &curShape, shapes, IM_ARRAYSIZE(shapes))) {
						previewParticle_->SetShape(static_cast<EffectShape>(curShape));
					}

					bool isBillboard = previewParticle_->GetBillboard();
					if (ImGui::Checkbox(LanguageManager::Tr("Billboard"), &isBillboard)) {
						previewParticle_->SetBillboard(isBillboard);
					}

					EffectShapeData& shapeData = previewParticle_->GetShapeDataRef();
					bool shapeChanged = false;
					if (previewParticle_->GetShape() == EffectShape::Plane) {
						shapeChanged |= ImGui::DragFloat2(LanguageManager::Tr("Plane Size"), &shapeData.planeSize.x, 0.05f, 0.01f, 100.0f, "%.2f");
						const char* pivotItems[] = { "Center", "Bottom", "Top" };
						shapeChanged |= ImGui::Combo(LanguageManager::Tr("Plane Pivot"), &shapeData.planePivot, pivotItems, IM_ARRAYSIZE(pivotItems));
						shapeChanged |= ImGui::DragFloat2(LanguageManager::Tr("Plane Offset"), &shapeData.planeOffset.x, 0.05f, -50.0f, 50.0f, "%.2f");
					} else if (previewParticle_->GetShape() == EffectShape::Cylinder) {
						shapeChanged |= ImGui::DragInt(LanguageManager::Tr("Cylinder Divide"), &shapeData.cylinderDivide, 1.0f, 3, 128);
						shapeChanged |= ImGui::DragFloat(LanguageManager::Tr("Cylinder Top Radius"), &shapeData.cylinderTopRadius, 0.05f, 0.0f, 100.0f);
						shapeChanged |= ImGui::DragFloat(LanguageManager::Tr("Cylinder Bottom Radius"), &shapeData.cylinderBottomRadius, 0.05f, 0.0f, 100.0f);
						shapeChanged |= ImGui::DragFloat(LanguageManager::Tr("Cylinder Height"), &shapeData.cylinderHeight, 0.05f, 0.0f, 100.0f);
					} else if (previewParticle_->GetShape() == EffectShape::Ring) {
						shapeChanged |= ImGui::DragInt(LanguageManager::Tr("Ring Divide"), &shapeData.ringDivide, 1.0f, 3, 128);
						shapeChanged |= ImGui::DragFloat(LanguageManager::Tr("Ring Outer Radius"), &shapeData.ringOuterRadius, 0.05f, 0.0f, 100.0f);
						shapeChanged |= ImGui::DragFloat(LanguageManager::Tr("Ring Inner Radius"), &shapeData.ringInnerRadius, 0.05f, 0.0f, 100.0f);
					} else if (previewParticle_->GetShape() == EffectShape::Cube) {
						shapeChanged |= ImGui::DragFloat3(LanguageManager::Tr("Cube Size"), &shapeData.cubeSize.x, 0.05f, 0.0f, 100.0f);
					}
					if (shapeChanged) {
						previewParticle_->SetShapeData(shapeData);
						if (singleParticleEffectDef_) {
							singleParticleEffectDef_->SetShapeData(shapeData);
						}
					}

					ImGui::Separator();
					ImGui::Text("%s:", LanguageManager::Tr("Texture"));
					std::string curTexPath = previewParticle_->GetTexturePath();
					std::filesystem::path tp(curTexPath);
					std::string texButtonLabel = tp.empty() ? "(None) Drop Image Here" : tp.filename().string();
					if (previewParticle_->GetEffectDefinition()) {
						D3D12_GPU_DESCRIPTOR_HANDLE texH = previewParticle_->GetEffectDefinition()->GetTextureSrvHandleGPU();
						if (texH.ptr != 0) {
							Vector4 curColor = previewParticle_->GetEnableColorOverLifetime() ? previewParticle_->GetStartColor() : previewParticle_->GetBaseParticleData().color;
							ImGui::Image((ImTextureID)texH.ptr, ImVec2(40, 40), ImVec2(0,0), ImVec2(1,1), ImVec4(curColor.x, curColor.y, curColor.z, 1.0f), ImVec4(1,1,1,0.5f));
							ImGui::SameLine();
						}
					}
					ImGui::Button(texButtonLabel.c_str(), ImVec2(-FLT_MIN, 40));
					if (ImGui::BeginDragDropTarget()) {
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_FILE")) {
							std::string path((const char*)payload->Data, payload->DataSize - 1);
							if (path.find(".png") != std::string::npos || path.find(".jpg") != std::string::npos || path.find(".dds") != std::string::npos) {
								previewParticle_->SetTexturePath(path);
							}
						}
						ImGui::EndDragDropTarget();
					}
					ImGui::TextDisabled(LanguageManager::Tr("(Drag and drop an image from the Resources window)"));

					ImGui::Text("%s:", LanguageManager::Tr("Quick Textures"));
					if (ImGui::SmallButton("circle.png")) {
						previewParticle_->SetTexturePath("Resources/Texture/circle.png");
					}
					ImGui::SameLine();
					if (ImGui::SmallButton("uvChecker.png")) {
						previewParticle_->SetTexturePath("Resources/Texture/uvChecker.png");
					}

					ImGui::Separator();
					EffectDefinitionData& baseData = previewParticle_->GetBaseParticleDataRef();
					ImGui::ColorEdit4(LanguageManager::Tr("Base Color"), &baseData.color.x);

					bool& enableColOverLifetime = previewParticle_->GetEnableColorOverLifetimeRef();
					ImGui::Checkbox(LanguageManager::Tr("Enable Color Over Lifetime"), &enableColOverLifetime);
					if (enableColOverLifetime) {
						Vector4& startCol = previewParticle_->GetStartColorRef();
						Vector4& endCol = previewParticle_->GetEndColorRef();
						ImGui::ColorEdit4(LanguageManager::Tr("Start Color"), &startCol.x);
						ImGui::ColorEdit4(LanguageManager::Tr("End Color"), &endCol.x);
					}

					ImGui::Separator();
					const char* blendModes[] = { "None", "Normal", "Add", "Subtract", "Multiply", "Screen" };
					int curBlend = static_cast<int>(previewParticle_->GetBlend());
					if (ImGui::Combo(LanguageManager::Tr("Blend Mode"), &curBlend, blendModes, IM_ARRAYSIZE(blendModes))) {
						previewParticle_->SetBlend(static_cast<BlendMode>(curBlend));
					}

					std::vector<std::string> shaderNames;
					std::filesystem::path shaderDir = "Resources/Shader/ParticleShader";
					if (std::filesystem::exists(shaderDir)) {
						for (const auto& entry : std::filesystem::directory_iterator(shaderDir)) {
							std::string filename = entry.path().filename().string();
							if (filename.find(".PS.hlsl") != std::string::npos) {
								std::string sName = filename.substr(0, filename.find(".PS.hlsl")) + "Shader";
								shaderNames.push_back(sName);
							}
						}
					}
					std::string curShader = previewParticle_->GetShader();
					if (ImGui::BeginCombo(LanguageManager::Tr("Shader"), curShader.c_str())) {
						for (const auto& shader : shaderNames) {
							bool is_selected = (curShader == shader);
							if (ImGui::Selectable(shader.c_str(), is_selected)) {
								previewParticle_->SetShader(shader);
							}
							if (is_selected) ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}

					ImGui::EndTabItem();
				}

				// ----------------------------------------------------
				// TAB 2: 発生・エリア (Spawn & Emitter)
				// ----------------------------------------------------
				if (ImGui::BeginTabItem(LanguageManager::Tr("Spawn & Emitter"))) {
					ImGui::Spacing();

					const char* emitterTypes[] = { "Box", "Sphere", "Circle", "Cone" };
					int currentType = static_cast<int>(previewParticle_->GetEmitterType());
					if (ImGui::Combo(LanguageManager::Tr("Emitter Type"), &currentType, emitterTypes, IM_ARRAYSIZE(emitterTypes))) {
						previewParticle_->SetEmitterType(static_cast<EmitterType>(currentType));
						FocusParticleCamera();
					}

					if (previewParticle_->GetEmitterType() == EmitterType::Box) {
						EmitterData& ed = previewParticle_->GetEmitterDataRef();
						ImGui::DragFloat3(LanguageManager::Tr("Position"), &ed.transform.translate.x, 0.05f, -FLT_MAX, FLT_MAX, "%.2f");
						ImGui::DragFloat3(LanguageManager::Tr("Rotation"), &ed.transform.rotate.x, 0.05f, -FLT_MAX, FLT_MAX, "%.2f");
						ImGui::DragFloat3(LanguageManager::Tr("Spawn Area Scale"), &ed.transform.scale.x, 0.05f, 0.01f, 500.0f, "%.2f");
						ImGui::DragInt(LanguageManager::Tr("Count"), reinterpret_cast<int*>(&ed.count), 1, 1, 1000);
						ImGui::DragFloat(LanguageManager::Tr("Frequency"), &ed.frequency, 0.01f, 0.001f, 10.0f, "%.3f");
					} else if (previewParticle_->GetEmitterType() == EmitterType::Sphere) {
						EmitterSphere& es = previewParticle_->GetEmitterSphereRef();
						ImGui::DragFloat3(LanguageManager::Tr("Position"), &es.translate.x, 0.05f, -FLT_MAX, FLT_MAX, "%.2f");
						ImGui::DragFloat(LanguageManager::Tr("Radius"), &es.radius, 0.05f, 0.01f, 500.0f, "%.2f");
						ImGui::DragInt(LanguageManager::Tr("Count"), reinterpret_cast<int*>(&es.count), 1, 1, 1000);
						ImGui::DragFloat(LanguageManager::Tr("Frequency"), &es.frequency, 0.01f, 0.001f, 10.0f, "%.3f");
					} else if (previewParticle_->GetEmitterType() == EmitterType::Circle) {
						EmitterCircle& ec = previewParticle_->GetEmitterCircleRef();
						ImGui::DragFloat3(LanguageManager::Tr("Position"), &ec.translate.x, 0.05f, -FLT_MAX, FLT_MAX, "%.2f");
						ImGui::DragFloat3(LanguageManager::Tr("Rotation"), &ec.rotate.x, 0.05f, -FLT_MAX, FLT_MAX, "%.2f");
						ImGui::DragFloat(LanguageManager::Tr("Outer Radius"), &ec.outerRadius, 0.05f, 0.01f, 500.0f, "%.2f");
						ImGui::DragFloat(LanguageManager::Tr("Inner Radius"), &ec.innerRadius, 0.05f, 0.0f, ec.outerRadius, "%.2f");
						ImGui::DragFloat(LanguageManager::Tr("Radial Velocity"), &ec.radialVelocity, 0.01f, -100.0f, 100.0f, "%.2f");
						ImGui::DragInt(LanguageManager::Tr("Count"), reinterpret_cast<int*>(&ec.count), 1, 1, 1000);
						ImGui::DragFloat(LanguageManager::Tr("Frequency"), &ec.frequency, 0.01f, 0.001f, 10.0f, "%.3f");
					} else if (previewParticle_->GetEmitterType() == EmitterType::Cone) {
						EmitterCone& econe = previewParticle_->GetEmitterConeRef();
						ImGui::DragFloat3(LanguageManager::Tr("Position"), &econe.translate.x, 0.05f, -FLT_MAX, FLT_MAX, "%.2f");
						ImGui::DragFloat3(LanguageManager::Tr("Rotation"), &econe.rotate.x, 0.05f, -FLT_MAX, FLT_MAX, "%.2f");
						ImGui::DragFloat(LanguageManager::Tr("Radius"), &econe.radius, 0.05f, 0.01f, 500.0f, "%.2f");
						ImGui::SliderAngle(LanguageManager::Tr("Spread Angle"), &econe.angle, 0.0f, 90.0f);
						ImGui::DragFloat(LanguageManager::Tr("Speed"), &econe.speed, 0.01f, -100.0f, 100.0f, "%.2f");
						ImGui::DragInt(LanguageManager::Tr("Count"), reinterpret_cast<int*>(&econe.count), 1, 1, 1000);
						ImGui::DragFloat(LanguageManager::Tr("Frequency"), &econe.frequency, 0.01f, 0.001f, 10.0f, "%.3f");
					}

					ImGui::Separator();
					ImGui::Text("%s:", LanguageManager::Tr("Burst Settings"));
					uint32_t burstCount = previewParticle_->GetBurstCount();
					if (ImGui::DragInt(LanguageManager::Tr("Burst Count"), reinterpret_cast<int*>(&burstCount), 1, 1, 10000)) {
						previewParticle_->SetBurstCount(burstCount);
					}
					if (ImGui::Button(LanguageManager::Tr("Trigger Burst"))) {
						previewParticle_->TriggerBurst();
					}

					ImGui::EndTabItem();
				}

				// ----------------------------------------------------
				// TAB 3: 動き・寿命 (Movement & Lifetime)
				// ----------------------------------------------------
				if (ImGui::BeginTabItem(LanguageManager::Tr("Movement & Lifetime"))) {
					ImGui::Spacing();

					EffectDefinitionData& baseData = previewParticle_->GetBaseParticleDataRef();
					ImGui::DragFloat(LanguageManager::Tr("LifeTime"), &baseData.lifeTime, 0.05f, 0.01f, 60.0f, "%.2f");
					ImGui::DragFloat3(LanguageManager::Tr("Base Size (Scale)"), &baseData.transform.scale.x, 0.01f, 0.001f, 100.0f, "%.2f");
					ImGui::DragFloat3(LanguageManager::Tr("Base Rotation"), &baseData.transform.rotate.x, 0.01f, -FLT_MAX, FLT_MAX, "%.2f");

					bool& alignToVel = previewParticle_->GetAlignToVelocityRef();
					ImGui::Checkbox(LanguageManager::Tr("Align to Velocity"), &alignToVel);

					ImGui::Separator();
					ImGui::Text("%s:", LanguageManager::Tr("Particle Movement Settings"));
					ParticleMovementData& mv = previewParticle_->GetMovementDataRef();
					ImGui::DragFloat3(LanguageManager::Tr("Base Velocity"), &mv.baseVelocity.x, 0.001f, -50.0f, 50.0f, "%.3f");
					ImGui::DragFloat3(LanguageManager::Tr("Velocity Variance"), &mv.velocityVariance.x, 0.001f, 0.0f, 50.0f, "%.3f");
					ImGui::DragFloat3(LanguageManager::Tr("Acceleration (Gravity)"), &mv.acceleration.x, 0.0001f, -5.0f, 5.0f, "%.4f");
					ImGui::DragFloat(LanguageManager::Tr("Radial Speed"), &mv.radialSpeed, 0.001f, -50.0f, 50.0f, "%.3f");
					ImGui::DragFloat(LanguageManager::Tr("Radial Speed Variance"), &mv.radialSpeedVariance, 0.001f, 0.0f, 50.0f, "%.3f");

					ImGui::Separator();
					ImGui::Text("%s:", LanguageManager::Tr("Scale Over Lifetime"));
					bool& enableScaleLifetime = previewParticle_->GetEnableScaleOverLifetimeRef();
					ImGui::Checkbox(LanguageManager::Tr("Enable Scale Over Lifetime"), &enableScaleLifetime);
					if (enableScaleLifetime) {
						const char* curves[] = { "Linear", "BellCurve (0 -> 1 -> 0)" };
						int& curveType = previewParticle_->GetScaleCurveTypeRef();
						ImGui::Combo(LanguageManager::Tr("Curve Type"), &curveType, curves, IM_ARRAYSIZE(curves));

						Vector3& startScale = previewParticle_->GetStartScaleRef();
						Vector3& endScale = previewParticle_->GetEndScaleRef();
						if (curveType == 0) {
							ImGui::DragFloat3(LanguageManager::Tr("Start Scale"), &startScale.x, 0.01f, 0.0f, 100.0f, "%.2f");
							ImGui::DragFloat3(LanguageManager::Tr("End Scale"), &endScale.x, 0.01f, 0.0f, 100.0f, "%.2f");
						} else {
							ImGui::DragFloat3(LanguageManager::Tr("Peak Scale"), &startScale.x, 0.01f, 0.0f, 100.0f, "%.2f");
						}
					}
					ImGui::DragFloat3(LanguageManager::Tr("Size Variance"), &mv.sizeVariance.x, 0.01f, 0.0f, 10.0f, "%.2f");

					ImGui::EndTabItem();
				}

				// ----------------------------------------------------
				// TAB 4: ツール・環境 (Tools & Camera)
				// ----------------------------------------------------
				if (ImGui::BeginTabItem(LanguageManager::Tr("Tools & Camera"))) {
					ImGui::Spacing();

					ImGui::Text("%s:", LanguageManager::Tr("Display Options"));
					ImGui::Checkbox(LanguageManager::Tr("Show Grid"), &showGridInViewer_);
					ImGui::SameLine();
					ImGui::Checkbox(LanguageManager::Tr("Show Emitter Cube"), &showEmitterCube_);

					ImGui::Separator();
					ImGui::Text("%s:", LanguageManager::Tr("Gizmo Operation"));
					if (ImGui::RadioButton(LanguageManager::Tr("Translate"), currentOp == ImGuizmo::TRANSLATE)) currentOp = ImGuizmo::TRANSLATE;
					ImGui::SameLine();
					if (ImGui::RadioButton(LanguageManager::Tr("Rotate"), currentOp == ImGuizmo::ROTATE)) currentOp = ImGuizmo::ROTATE;
					ImGui::SameLine();
					if (ImGui::RadioButton(LanguageManager::Tr("Scale"), currentOp == ImGuizmo::SCALE)) currentOp = ImGuizmo::SCALE;

					ImGui::Separator();
					ImGui::Text("%s:", LanguageManager::Tr("Camera Controls"));
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
					Vector3 camEye = previewCamera_->GetDebugCameraRef().GetEye();
					if (ImGui::DragFloat3(LanguageManager::Tr("Camera Pos"), &camEye.x, 0.1f)) {
						previewCamera_->GetDebugCameraRef().SetEye(camEye);
					}
					ImGui::TextDisabled(LanguageManager::Tr("(Preview: RMB: Orbit, Shift+RMB/MMB: Pan, Wheel: Zoom)"));

					ImGui::Separator();
					ImGui::Text("%s:", LanguageManager::Tr("Engine Features"));
					bool useGpu = previewParticle_->GetUseGpuParticle();
					if (ImGui::Checkbox(LanguageManager::Tr("Use GPU Particle (Compute Shader)"), &useGpu)) {
						previewParticle_->SetUseGpuParticle(useGpu);
					}
					bool& isHit = previewParticle_->GetIsHitRef();
					ImGui::Checkbox(LanguageManager::Tr("IsHit"), &isHit);

					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}

			ImGui::EndChild();
			ImGui::EndGroup(); // 右カラム終了
		}
		ImGui::End();

		// Draw the particle into the render texture AFTER ImGui has processed (and potentially recreated resources)
		previewCamera_->Update();
		Matrix4x4 projMat = previewCamera_->GetProjectionMatrix();
		previewParticle_->SetCustomProjectionMatrix(projMat);
		previewParticle_->Update(previewCamera_->GetViewMatrix());

		auto cmdList = engine->GetCommand()->GetCommandList();
		ID3D12DescriptorHeap* descriptorHeaps[] = { engine->GetDescriptorHeap()->GetSrvDescriptorHeap() };
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

		Camera* prevCamera = engine->GetDraw()->GetCamera();
		engine->GetDraw()->SetCamera(previewCamera_.get());
		if (showGridInViewer_) {
			previewGrid_->SettingWvp(previewCamera_->GetViewMatrix(), &projMat);
			engine->GetDraw()->DrawGrid(previewGrid_.get());
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

		previewParticle_->Draw(*engine->GetDraw());

		particleRenderTexture_->TransitionToShaderResource(cmdList);

		// Render Single Particle Preview
		if (singleParticleEffectDef_ && singleParticleRenderTexture_) {
			static std::string s_lastSingleParticleTexPath = "";
			if (s_lastSingleParticleTexPath != previewParticle_->GetTexturePath() && !previewParticle_->GetTexturePath().empty()) {
				s_lastSingleParticleTexPath = previewParticle_->GetTexturePath();
				singleParticleEffectDef_->SetTexturePath(s_lastSingleParticleTexPath);
			}

			static EffectShape s_lastShape = EffectShape::Plane;
			if (s_lastShape != previewParticle_->GetShape()) {
				s_lastShape = previewParticle_->GetShape();
				singleParticleEffectDef_->SetShape(s_lastShape, previewParticle_->GetShapeData());
			}

			singleParticleEffectDef_->SetShader(previewParticle_->GetShader());
			singleParticleEffectDef_->SetBlend(previewParticle_->GetBlend());
			singleParticleEffectDef_->SetBillboard(previewParticle_->GetBillboard());

			singleParticleTurntableAngle_ += 0.02f;
			singleParticleCamera_->Update();
			Matrix4x4 singleProj = singleParticleCamera_->GetProjectionMatrix();
			singleParticleEffectDef_->SetCustomProjectionMatrix(singleProj);

			std::list<EffectDefinitionData> singleList;
			EffectDefinitionData pData = previewParticle_->GetBaseParticleData();
			pData.transform.translate = { 0.0f, 0.0f, 0.0f };
			pData.transform.scale = { 1.0f, 1.0f, 1.0f };
			pData.transform.rotate = { 0.0f, singleParticleTurntableAngle_, 0.0f };
			pData.lifeTime = 999999.0f;
			pData.currentTime = 0.0f;
			if (previewParticle_->GetEnableColorOverLifetime()) {
				pData.color = previewParticle_->GetStartColor();
			}
			singleList.push_back(pData);
			singleParticleEffectDef_->Update(singleParticleCamera_->GetViewMatrix(), singleList);

			singleParticleRenderTexture_->TransitionToRenderTarget(cmdList);
			singleParticleRenderTexture_->Clear(cmdList);
			singleParticleDepthStencil_->TransitionToDepthWrite(cmdList);
			D3D12_CPU_DESCRIPTOR_HANDLE singleRtv = singleParticleRenderTexture_->GetRtvHandle();
			singleParticleDepthStencil_->SetDSV(cmdList, &singleRtv);

			D3D12_VIEWPORT singleVp = { 0.0f, 0.0f, 256.0f, 256.0f, 0.0f, 1.0f };
			D3D12_RECT singleScissor = { 0, 0, 256, 256 };
			cmdList->RSSetViewports(1, &singleVp);
			cmdList->RSSetScissorRects(1, &singleScissor);

			engine->GetDraw()->SetCamera(singleParticleCamera_.get());
			engine->GetDraw()->DrawParticle(singleParticleEffectDef_.get());

			singleParticleRenderTexture_->TransitionToShaderResource(cmdList);
		}

		// Restore engine's main render target
		D3D12_CPU_DESCRIPTOR_HANDLE mainRtv = engine->GetRenderTexture()->GetRtvHandle();
		engine->GetDepthStencil()->SetDSV(cmdList, &mainRtv);
		cmdList->RSSetViewports(1, engine->GetViewportScissor()->GetViewport());
		cmdList->RSSetScissorRects(1, engine->GetViewportScissor()->GetScissorRect());
		engine->GetDraw()->SetCamera(prevCamera);
	}

	// Model Viewer Window
	if (showModelViewer_) {
		if (!isModelViewerInitialized_) {
			modelRenderTexture_ = std::make_unique<RenderTexture>();
			modelDepthStencil_ = std::make_unique<DepthStencil>();
			previewModel_ = std::make_unique<Model>();
			modelCamera_ = std::make_unique<Camera>();
			modelGrid_ = std::make_unique<Grid>();

			modelRenderTexture_->Initialize(engine->GetGraphics()->GetDevice(), 512, 512, engine->GetDescriptorHeap()->GetSrvDescriptorHeap(), engine->GetDescriptorHeap()->GetDescriptorSizeSRV());
			modelDepthStencil_->CreateDepthStencil(engine->GetGraphics()->GetDevice(), 512, 512);
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
						previewModel_->SetName("Preview Model");
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

		auto cmdList = engine->GetCommand()->GetCommandList();
		modelRenderTexture_->TransitionToRenderTarget(cmdList);
		modelRenderTexture_->Clear(cmdList);
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = modelRenderTexture_->GetRtvHandle();
		modelDepthStencil_->SetDSV(cmdList, &rtvHandle);

		D3D12_VIEWPORT vp = { 0.0f, 0.0f, 512.0f, 512.0f, 0.0f, 1.0f };
		D3D12_RECT scissor = { 0, 0, 512, 512 };
		cmdList->RSSetViewports(1, &vp);
		cmdList->RSSetScissorRects(1, &scissor);

		Camera* prevModelCamera = engine->GetDraw()->GetCamera();
		engine->GetDraw()->SetCamera(modelCamera_.get());
		if (showGridInModelViewer_) {
			modelGrid_->SettingWvp(modelCamera_->GetViewMatrix(), &modelProjMat);
			engine->GetDraw()->DrawGrid(modelGrid_.get());
		}
		if (!currentModelPath_.empty()) {
			engine->GetDraw()->DrawModel(previewModel_.get());
		}

		modelRenderTexture_->TransitionToShaderResource(cmdList);

		// Restore main
		D3D12_CPU_DESCRIPTOR_HANDLE mainRtv = engine->GetRenderTexture()->GetRtvHandle();
		engine->GetDepthStencil()->SetDSV(cmdList, &mainRtv);
		cmdList->RSSetViewports(1, engine->GetViewportScissor()->GetViewport());
		cmdList->RSSetScissorRects(1, engine->GetViewportScissor()->GetScissorRect());
		engine->GetDraw()->SetCamera(prevModelCamera);
	}

	// Game View Window
	if (showGameViewWindow_) {
		if (!isGameViewInitialized_) {
			for (int i = 0; i < 2; ++i) {
				gameViewRenderTextures_[i] = std::make_unique<RenderTexture>();
				gameViewRenderTextures_[i]->Initialize(
					engine->GetGraphics()->GetDevice(),
					WindowConfig::kDefaultClientWidth, WindowConfig::kDefaultClientHeight,
					engine->GetDescriptorHeap()->GetSrvDescriptorHeap(),
					engine->GetDescriptorHeap()->GetDescriptorSizeSRV());
			}
			gameViewDepthStencil_ = std::make_unique<DepthStencil>();
			gameViewDepthStencil_->CreateDepthStencil(
				engine->GetGraphics()->GetDevice(),
				WindowConfig::kDefaultClientWidth, WindowConfig::kDefaultClientHeight,
				engine->GetDescriptorHeap()->GetSrvDescriptorHeap(),
				engine->GetDescriptorHeap()->GetDescriptorSizeSRV());
			isGameViewInitialized_ = true;
		}

		ImGui::SetNextWindowSize(ImVec2(800, 450), ImGuiCond_FirstUseEver);
		if (ImGui::Begin(LanguageManager::Tr("Game"), &showGameViewWindow_)) {
			ImVec2 availSize = ImGui::GetContentRegionAvail();
			ImVec2 imageSize = availSize;
			ImVec2 cursorStart = ImGui::GetCursorPos();

			float targetAspect = Camera::kDefaultAspectRatio;
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
		auto cmdList = engine->GetCommand()->GetCommandList();
		gameViewRenderTextures_[0]->TransitionToRenderTarget(cmdList);
		gameViewRenderTextures_[0]->Clear(cmdList);
		gameViewDepthStencil_->TransitionToDepthWrite(cmdList);
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = gameViewRenderTextures_[0]->GetRtvHandle();
		gameViewDepthStencil_->SetDSV(cmdList, &rtvHandle);

		D3D12_VIEWPORT vp = {
			0.0f, 0.0f,
			static_cast<float>(WindowConfig::kDefaultClientWidth),
			static_cast<float>(WindowConfig::kDefaultClientHeight),
			0.0f, 1.0f
		};
		D3D12_RECT scissor = {
			0, 0,
			WindowConfig::kDefaultClientWidth,
			WindowConfig::kDefaultClientHeight
		};
		cmdList->RSSetViewports(1, &vp);
		cmdList->RSSetScissorRects(1, &scissor);

		ID3D12DescriptorHeap* descriptorHeaps[] = { engine->GetDescriptorHeap()->GetSrvDescriptorHeap() };
		cmdList->SetDescriptorHeaps(1, descriptorHeaps);

		if (s_gameViewDrawCallback_) {
			s_gameViewDrawCallback_(*engine->GetDraw());

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

				engine->GetDraw()->DrawPostEffect(
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

				s_gameViewUIDrawCallback_(*engine->GetDraw());

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
		engine->GetDepthStencil()->SetDSV(cmdList, &mainRtv);
		cmdList->RSSetViewports(1, engine->GetViewportScissor()->GetViewport());
		cmdList->RSSetScissorRects(1, engine->GetViewportScissor()->GetScissorRect());
		cmdList->SetDescriptorHeaps(1, descriptorHeaps);
	}

#endif
}

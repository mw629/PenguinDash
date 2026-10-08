#pragma once
#include <functional>
#include <memory>
#include <string>

#ifdef _USE_IMGUI
#include <imgui.h>
#endif // _USE_IMGUI

class Engine;
class RenderTexture;
class DepthStencil;
class Emitter;
class Camera;
class Grid;
class Model;

class Draw;

class EditorManager {
public:
	// Sceneウィンドウ内のオーバーレイ描画用コールバック型
	using SceneOverlayCallback = std::function<void()>;
	// Save/Loadコールバック型
	using EditorCallback = std::function<void(const std::string&)>;
	// FileDropコールバック型
	using FileDropCallback = std::function<void(const std::string&)>;
	// Game View描画コールバック型
	using GameViewDrawCallback = std::function<void(Draw&)>;
	using GameViewUIDrawCallback = std::function<void(Draw&)>;

private:
	static bool isPlaying_;
	static float playSpeed_;
	static SceneOverlayCallback s_sceneOverlayCallback_;
	static EditorCallback s_saveCallback_;
	static EditorCallback s_loadCallback_;
	static FileDropCallback s_fileDropCallback_;
	static GameViewDrawCallback s_gameViewDrawCallback_;
	static GameViewUIDrawCallback s_gameViewUIDrawCallback_;
	static std::string s_currentFileName_;
	
	bool showFinalWindow_ = true;
	bool showResourcesWindow_ = true;
	bool showLogsWindow_ = true;
	bool showParticleViewer_ = false;
	bool showGameViewWindow_ = true;
	int sceneAspectRatioIndex_ = 0; // 0: Free, 1: 16:9, 2: 4:3, 3: 1:1, 4: 21:9

	std::unique_ptr<RenderTexture> particleRenderTexture_;
	std::unique_ptr<DepthStencil> particleDepthStencil_;
	std::unique_ptr<Emitter> previewParticle_;
	std::unique_ptr<Camera> previewCamera_;
	std::unique_ptr<Grid> previewGrid_;
	bool isParticleViewerInitialized_ = false;
	bool showGridInViewer_ = true;
	bool showEmitterCube_ = true;

	std::unique_ptr<RenderTexture> gameViewRenderTextures_[2];
	std::unique_ptr<DepthStencil> gameViewDepthStencil_;
	bool isGameViewInitialized_ = false;
	int gameViewFinalRTIndex_ = 0;

	bool showModelViewer_ = false;
	std::unique_ptr<RenderTexture> modelRenderTexture_;
	std::unique_ptr<DepthStencil> modelDepthStencil_;
	std::unique_ptr<Model> previewModel_;
	std::unique_ptr<Camera> modelCamera_;
	std::unique_ptr<Grid> modelGrid_;
	bool isModelViewerInitialized_ = false;
	bool showGridInModelViewer_ = true;
	std::string currentModelPath_ = "";

public:
	~EditorManager();

	static void SetPlaying(bool playing) { isPlaying_ = playing; }
	static bool IsPlaying() { return isPlaying_; }

	static void SetPlaySpeed(float speed) { playSpeed_ = speed; }
	static float GetPlaySpeed() { return playSpeed_; }

	static void SetSceneOverlayCallback(SceneOverlayCallback cb) { s_sceneOverlayCallback_ = cb; }
	static void ClearSceneOverlayCallback() { s_sceneOverlayCallback_ = nullptr; }

	static void SetSaveCallback(EditorCallback cb) { s_saveCallback_ = cb; }
	static void SetLoadCallback(EditorCallback cb) { s_loadCallback_ = cb; }
	static void SetFileDropCallback(FileDropCallback cb) { s_fileDropCallback_ = cb; }

	static void SetGameViewDrawCallback(GameViewDrawCallback cb, GameViewUIDrawCallback uiCb = nullptr) {
		s_gameViewDrawCallback_ = cb;
		s_gameViewUIDrawCallback_ = uiCb;
	}
	static void SetGameViewUIDrawCallback(GameViewUIDrawCallback uiCb) {
		s_gameViewUIDrawCallback_ = uiCb;
	}

	static void SetCurrentFileName(const std::string& name) { s_currentFileName_ = name; }
	static std::string GetCurrentFileName() { return s_currentFileName_; }

	static int s_gizmoOp; // 0: Translate, 1: Rotate, 2: Scale

#ifdef _USE_IMGUI
	static ImVec2 s_sceneImagePos;
	static ImVec2 s_sceneImageSize;
#endif // _USE_IMGUI

	// 毎フレーム呼ばれるエディタUIの更新処理
	void Update(Engine* engine);
};

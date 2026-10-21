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
	static constexpr float kOneShotReplayInterval = 2.0f;
	
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
	/// <summary>
	/// デストラクタ
	/// </summary>
	~EditorManager();

	/// <summary>
	/// 再生中フラグの設定
	/// </summary>
	/// <param name="playing">true: 再生中, false: 停止中</param>
	static void SetPlaying(bool playing) { isPlaying_ = playing; }

	/// <summary>
	/// エディタが再生中かどうかを取得
	/// </summary>
	/// <returns>再生中ならtrue</returns>
	static bool IsPlaying() { return isPlaying_; }

	/// <summary>
	/// 再生速度スケールの設定
	/// </summary>
	/// <param name="speed">速度スケール（1.0fが等速）</param>
	static void SetPlaySpeed(float speed) { playSpeed_ = speed; }

	/// <summary>
	/// 再生速度スケールの取得
	/// </summary>
	/// <returns>速度スケール</returns>
	static float GetPlaySpeed() { return playSpeed_; }

	/// <summary>
	/// Sceneウィンドウ上のオーバーレイ描画コールバック設定
	/// </summary>
	static void SetSceneOverlayCallback(SceneOverlayCallback cb) { s_sceneOverlayCallback_ = cb; }

	/// <summary>
	/// Sceneウィンドウ上のオーバーレイ描画コールバック解除
	/// </summary>
	static void ClearSceneOverlayCallback() { s_sceneOverlayCallback_ = nullptr; }

	/// <summary>
	/// シーン保存コールバックの設定
	/// </summary>
	static void SetSaveCallback(EditorCallback cb) { s_saveCallback_ = cb; }

	/// <summary>
	/// シーン読み込みコールバックの設定
	/// </summary>
	static void SetLoadCallback(EditorCallback cb) { s_loadCallback_ = cb; }

	/// <summary>
	/// ファイルドロップコールバックの設定
	/// </summary>
	static void SetFileDropCallback(FileDropCallback cb) { s_fileDropCallback_ = cb; }

	/// <summary>
	/// GameViewウィンドウの描画コールバック設定
	/// </summary>
	/// <param name="cb">3D描画コールバック</param>
	/// <param name="uiCb">2D UI描画コールバック</param>
	static void SetGameViewDrawCallback(GameViewDrawCallback cb, GameViewUIDrawCallback uiCb = nullptr) {
		s_gameViewDrawCallback_ = cb;
		s_gameViewUIDrawCallback_ = uiCb;
	}

	/// <summary>
	/// GameViewウィンドウの2D UI描画コールバック設定
	/// </summary>
	/// <param name="uiCb">UI描画コールバック</param>
	static void SetGameViewUIDrawCallback(GameViewUIDrawCallback uiCb) {
		s_gameViewUIDrawCallback_ = uiCb;
	}

	/// <summary>
	/// 現在編集中のファイル名を設定
	/// </summary>
	static void SetCurrentFileName(const std::string& name) { s_currentFileName_ = name; }

	/// <summary>
	/// 現在編集中のファイル名を取得
	/// </summary>
	static std::string GetCurrentFileName() { return s_currentFileName_; }

	static int s_gizmoOp; // 0: Translate, 1: Rotate, 2: Scale

#ifdef _USE_IMGUI
	static ImVec2 s_sceneImagePos;
	static ImVec2 s_sceneImageSize;
#endif // _USE_IMGUI

	/// <summary>
	/// エディタ全体のImGui描画およびGameViewの描画更新
	/// </summary>
	/// <param name="engine">エンジンインスタンスポインタ</param>
	void Update(Engine* engine);
};

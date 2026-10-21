#pragma once

enum SceneID
{
	Test = 0,
	Title = 1,
	StageSelect = 2,
	Game = 3,
	Clear = 4,
	GameOver = 5,
	Json = 6
};

class IScene
{
protected:

	int sceneID_ = -1;
	int nextSceneID_;
	bool sceneChangeRequest_;

public:

	virtual ~IScene() {};

	/// <summary>
	/// ImGuiデバッグウィンドウの描画
	/// </summary>
	virtual void ImGui() = 0;

	/// <summary>
	/// シーンの初期化
	/// </summary>
	virtual void Initialize() = 0;

	/// <summary>
	/// シーンの毎フレーム更新
	/// </summary>
	virtual void Update() = 0;

	/// <summary>
	/// 3Dオブジェクトの描画処理
	/// </summary>
	/// <param name="draw">描画マネージャ</param>
	virtual void Draw(class Draw& draw) = 0;

	/// <summary>
	/// 2D UIおよびオーバーレイの描画処理
	/// </summary>
	/// <param name="draw">描画マネージャ</param>
	virtual void DrawUI(class Draw& /*draw*/) {}

	int GetSceneID() { return sceneID_; }
	int GetNextSceneID() { return nextSceneID_; }
	bool GetSceneChangeRequest() { return sceneChangeRequest_; }

};

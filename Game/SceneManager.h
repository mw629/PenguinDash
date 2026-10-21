#pragma once
#include "GameScene.h"
#include <memory>
#include "IScene.h"
#include "Direction/FreezeTransition.h"

class SceneManager
{
private:

	std::unique_ptr<IScene> scene_;
	bool isTransitioning_ = false;

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	SceneManager();

	/// <summary>
	/// デストラクタ
	/// </summary>
	~SceneManager() = default;

	/// <summary>
	/// シーン管理のImGuiデバッグUI描画
	/// </summary>
	void ImGui();

	/// <summary>
	/// 初期シーンの生成と初期化
	/// </summary>
	void Initialize();
	
	/// <summary>
	/// 更新前の事前処理
	/// </summary>
	void PreUpdate();

	/// <summary>
	/// シーンおよび画面遷移の更新処理
	/// </summary>
	void Update();

	/// <summary>
	/// 現在のシーンの3Dオブジェクト描画処理
	/// </summary>
	/// <param name="draw">描画マネージャ</param>
	void Draw(class Draw& draw);

	/// <summary>
	/// 現在のシーンの2D UI描画処理
	/// </summary>
	/// <param name="draw">描画マネージャ</param>
	void DrawUI(class Draw& draw);

	/// <summary>
	/// 毎フレームの更新と描画の一括実行
	/// </summary>
	/// <param name="draw">描画マネージャ</param>
	void Run(class Draw& draw);

	/// <summary>
	/// 指定IDに応じたシーンインスタンスを生成
	/// </summary>
	/// <param name="sceneID">シーンID</param>
	/// <returns>生成されたISceneのunique_ptr</returns>
	std::unique_ptr<IScene> CreateScene(int sceneID);

	/// <summary>
	/// 氷結・破砕演出付きのシーン切り替え
	/// </summary>
	/// <param name="nextSceneID">遷移先シーンID</param>
	void ChangeScene(int nextSceneID);

};


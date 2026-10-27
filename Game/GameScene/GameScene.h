#pragma once
#include "../../Editer/EditorUI.h"
#include "../Direction/FreezeTransition.h"
#include "../IScene.h"
#include "Camera/GameCameraController.h"
#include "Effect/CrashEffect.h"
#include "Effect/GameSceneEffect.h"
#include "Effect/RankingCutinEffect.h"
#include "GameObjectManager.h"
#include "RenderObject.h"
#include "System/AchievementManager.h"
#include "System/BossBattleController.h"
#include "System/ScoreManager.h"
#include "System/TitleDirector.h"
#include "UI/GameHUD.h"
#include <Engine.h>
#include <Entity/Boss.h>
#include <Entity/Collision.h>
#include <Entity/Player.h>
#include <Stage/StageSettings.h>
#include <memory>
#include <string>

class GameScene : public IScene {
public:
  enum GameState {
    Title,
    Playing,
    Paused,
    PlayerHit,
    FirstPlaceCutIn,
    GameClear,
    GameOver,
    Editor
  };

  enum class PlayingState { ThreeLane, OneLane, Boss };

private:
  // 基本システム・アセット
  std::unique_ptr<Texture> texture_ = std::make_unique<Texture>();

  // ゲーム状態
  GameState gameState_ = GameState::Title;
  PlayingState playingState_ = PlayingState::ThreeLane;

  // 分離したモジュールクラス群
  std::unique_ptr<GameCameraController> cameraController_ =
      std::make_unique<GameCameraController>();
  std::unique_ptr<ScoreManager> scoreManager_ =
      std::make_unique<ScoreManager>();
  std::unique_ptr<AchievementManager> achievementManager_ =
      std::make_unique<AchievementManager>();
  std::unique_ptr<CrashEffect> crashEffect_ = std::make_unique<CrashEffect>();
  std::unique_ptr<RankingCutinEffect> rankingCutinEffect_ =
      std::make_unique<RankingCutinEffect>();
  bool forceFirstPlaceCutInTest_ = false;
  int testCutinType_ = 0; // 0: Score, 1: Distance, 2: Double
  std::unique_ptr<BossBattleController> bossBattleController_ =
      std::make_unique<BossBattleController>();
  std::unique_ptr<TitleDirector> titleDirector_ =
      std::make_unique<TitleDirector>();
  std::unique_ptr<GameHUD> gameHUD_ = std::make_unique<GameHUD>();

  // 実績トラッキング用変数
  float playTime_ = 0.0f;
  int jumpCount_ = 0;
  int rollCount_ = 0;
  int laneChangeCount_ = 0;
  float unMovedTime_ = 0.0f;
  bool wasJumping_ = false;
  bool wasRolling_ = false;
  int lastPlayerLane_ = 0;

  // 定数
  static constexpr Vector3 kDefaultSkyBoxScale = {100.0f, 100.0f, 100.0f};
  static constexpr float kOneLaneTotalDistance = 200.0f;
  static constexpr float kDustGroundThresholdY = 3.01f;
  static constexpr float kInvertedCameraThresholdRotateY = 1.57f;
  static constexpr float kStartObstacleClearDistance = 12.0f;
  static constexpr float kStartItemSpawnChance = 0.15f;
  static constexpr float kPlayBaseScrollSpeed = 0.2f;
  static constexpr float kPlayScrollAcceleration = 0.0001f;
  static constexpr float kTitleBaseScrollSpeed = 0.22f;
  static constexpr float kResultFreezeDuration = 0.75f;
  static constexpr float kResultShatterDuration = 0.75f;
  static constexpr float kHitRumbleDuration = 0.35f;
  static constexpr int kHitRumbleMotorSpeed = 48000;
  static constexpr int kThreeLaneCount = 3;
  static constexpr int kOneLaneCount = 1;

  std::shared_ptr<Cube> skyBox_ = std::make_shared<Cube>();
  int skyBoxTexture_ = -1;
  Transform skyBoxTransform_{
      kDefaultSkyBoxScale, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

  // エンティティ・ワールド管理
  std::shared_ptr<Player> player_ = std::make_shared<Player>();
  std::shared_ptr<Boss> boss_ = std::make_shared<Boss>();
  std::unique_ptr<GameObjectManager> gameObjectManager_ =
      std::make_unique<GameObjectManager>();
  std::unique_ptr<StageSettings> stageSettings_ =
      std::make_unique<StageSettings>();
  std::unique_ptr<GameSceneEffect> effectManager_ =
      std::make_unique<GameSceneEffect>();
  std::unique_ptr<EditorUI> editorUI_ = std::make_unique<EditorUI>();

  // 右サイドカメラ（アイテム取得後）の管理
  bool isRightSideMode_ = false;
  float rightSideDistance_ = 0.0f;

  // ボス戦終了後のクールタイム開始用フラグ
  bool wasBossBattle_ = false;

  // 画面遷移
  enum class ResultTransition { None, Restart, Title };
  ResultTransition resultTransition_ = ResultTransition::None;

  float uiTimer_ = 0.0f;
  class Draw *draw_ = nullptr;

  /// <summary>
  /// ワールド内オブジェクトの一括更新
  /// </summary>
  void UpdateWorldEntities(float timeScale, float speedMultiplier);

  /// <summary>
  /// プレイヤーと障害物の当たり判定処理
  /// </summary>
  void CheckCollisions();

  /// <summary>
  /// スライディング継続判定処理
  /// </summary>
  void CheckKeepRolling();

  /// <summary>
  /// 画面遷移演出（氷結・破砕）の開始
  /// </summary>
  /// <param name="target">遷移先（リスタート / タイトル復帰）</param>
  void StartResultTransition(ResultTransition target);

public:
  /// <summary>
  /// デフォルトコンストラクタ
  /// </summary>
  GameScene() = default;

  /// <summary>
  /// デストラクタ。コールバック登録の解除を行います。
  /// </summary>
  ~GameScene() override;

  /// <summary>
  /// ゲームシーンの初期化
  /// </summary>
  void Initialize() override;

  /// <summary>
  /// 毎フレーム更新処理
  /// </summary>
  void Update() override;

  /// <summary>
  /// 3Dオブジェクトの描画処理
  /// </summary>
  /// <param name="draw">描画マネージャ</param>
  void Draw(class Draw &draw) override;

  /// <summary>
  /// 2D UIおよび演出オーバーレイの描画処理
  /// </summary>
  /// <param name="draw">描画マネージャ</param>
  void DrawUI(class Draw &draw) override;

  /// <summary>
  /// ImGuiによるパラメータ調整ウィンドウの描画
  /// </summary>
  void ImGui() override;

  /// <summary>
  /// プレイ状態（3レーン/1レーン/ボス戦）の切り替え
  /// </summary>
  /// <param name="newState">変更先の状態</param>
  /// <param name="force">強制切り替えフラグ</param>
  void ChangePlayingState(PlayingState newState, bool force = false);

  /// <summary>
  /// ゲームプレイ状態のリセット
  /// </summary>
  void ResetGame();

  /// <summary>
  /// タイトルからゲーム開始カウントダウンへの移行
  /// </summary>
  void StartGame();

  /// <summary>
  /// カウントダウン終了後のプレイ開始
  /// </summary>
  void StartPlaying();

  /// <summary>
  /// タイトル画面への復帰
  /// </summary>
  void ReturnToTitle();

  /// <summary>
  /// タイトル画面中の更新処理
  /// </summary>
  void TitleUpdate();

  /// <summary>
  /// ゲームプレイ中の更新処理
  /// </summary>
  void PlayingUpdate();

  /// <summary>
  /// ポーズ中の更新処理
  /// </summary>
  void PausedUpdate();

  /// <summary>
  /// プレイヤー被弾中の更新処理
  /// </summary>
  void PlayerHitUpdate();

  /// <summary>
  /// 1位更新カットイン演出中の更新処理
  /// </summary>
  void FirstPlaceCutInUpdate();

  /// <summary>
  /// エディターモード中の更新処理
  /// </summary>
  void EditorUpdate();

  /// <summary>
  /// ゲームシーン設定をJSONファイルへ保存
  /// </summary>
  /// <param name="fileName">保存ファイル名</param>
  /// <returns>保存成功時true</returns>
  bool SaveSettingsToJson(const std::string &fileName = "GameSceneSettings");

  /// <summary>
  /// JSONファイルからゲームシーン設定を読み込み
  /// </summary>
  /// <param name="fileName">読み込みファイル名</param>
  /// <returns>読み込み成功時true</returns>
  bool LoadSettingsFromJson(const std::string &fileName = "GameSceneSettings");

  /// <summary>
  /// ゲームシーンの全パラメータをデフォルト値へリセット
  /// </summary>
  void ResetSettingsToDefault();

  char settingsFileName_[128] = "GameSceneSettings";
  std::string settingsStatusMessage_ = "";
  float settingsStatusTimer_ = 0.0f;

public:
  enum class TitleMenuItem {
    PlayerName = 0,
    GameStart,
    Ranking,
    HowToPlay,
    Exit,
    Count
  };

  enum class TitleSubView { None = 0, Ranking, HowToPlay };

private:
  TitleMenuItem titleMenuIndex_ = TitleMenuItem::GameStart;
  TitleSubView titleSubView_ = TitleSubView::None;
  std::string titlePlayerName_ = "Player";
  bool isEditingTitlePlayerName_ = false;

  /// <summary>
  /// タイトル画面でのプレイヤー名入力処理
  /// </summary>
  void UpdateTitleTextInput();

public:
  /// <summary>
  /// スコアマネージャーを取得
  /// </summary>
  /// <returns>ScoreManagerへのポインタ</returns>
  class ScoreManager *GetScoreManager() { return scoreManager_.get(); }

  /// <summary>
  /// 実績マネージャーを取得
  /// </summary>
  /// <returns>AchievementManagerへのポインタ</returns>
  class AchievementManager *GetAchievementManager() {
    return achievementManager_.get();
  }

  /// <summary>
  /// エフェクトマネージャーを取得
  /// </summary>
  /// <returns>GameSceneEffectへのポインタ</returns>
  class GameSceneEffect *GetEffectManager() { return effectManager_.get(); }

  /// <summary>
  /// ステージ設定・ギミック管理を取得
  /// </summary>
  /// <returns>StageSettingsへのポインタ</returns>
  class StageSettings *GetStageSettings() { return stageSettings_.get(); }

  /// <summary>
  /// プレイヤーインスタンスを取得
  /// </summary>
  /// <returns>Playerへのポインタ</returns>
  class Player *GetPlayer() { return player_.get(); }

  /// <summary>
  /// ボスインスタンスを取得
  /// </summary>
  /// <returns>Bossへのポインタ</returns>
  class Boss *GetBoss() { return boss_.get(); }

  /// <summary>
  /// ゲームカメラコントローラーを取得
  /// </summary>
  /// <returns>GameCameraControllerへのポインタ</returns>
  class GameCameraController *GetCameraController() {
    return cameraController_.get();
  }

  /// <summary>
  /// 現在のゲームプレイ状態（3レーン/1レーン/ボス戦）を取得
  /// </summary>
  /// <returns>PlayingState</returns>
  PlayingState GetPlayingState() const { return playingState_; }
  /// <summary>
  /// プレイヤー被弾処理（クラッシュ演出、カメラ振動、SE、ランキング送信等）
  /// </summary>
  /// <param name="isTrip">つまずき転倒かどうか</param>
  /// <param name="isLargeObstacle">大型障害物（カメラ激突対象）かどうか</param>
  void TriggerPlayerHit(bool isTrip, bool isLargeObstacle = false);
};

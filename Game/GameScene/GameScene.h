#pragma once
#include "../../Editer/EditorUI.h"
#include "../Direction/FreezeTransition.h"
#include "../IScene.h"
#include "Camera/GameCameraController.h"
#include "DirectionalLight.h"
#include "Effect/CrashEffect.h"
#include "Effect/GameSceneEffect.h"
#include "Effect/RankingCutinEffect.h"
#include "GameObjectManager.h"
#include "PointLight.h"
#include "RenderObject.h"
#include "State/IGameSceneState.h"
#include "State/IPlayingState.h"
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
  // 既存呼び出し元との互換用型エイリアス
  using GameState = ::GameState;
  using PlayingState = ::PlayingState;

  enum class ResultTransition { None, Restart, Title };

  // ステートクラスから参照される共通定数
  static constexpr float kStartObstacleClearDistance = 12.0f;
  static constexpr float kPlayScrollAcceleration = 0.0001f;
  static constexpr int kThreeLaneCount = 3;
  static constexpr int kOneLaneCount = 1;

private:
  // 基本システム・アセット
  std::unique_ptr<Texture> texture_ = std::make_unique<Texture>();

  // ゲーム状態（Stateパターン：ステートインスタンスを唯一の真実とする）
  std::unique_ptr<IGameSceneState> currentGameState_;
  std::unique_ptr<IPlayingState> currentPlayingState_;

  void ResetPlayingStateToThreeLane();

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
  int barrierBlockCount_ = 0;
  int clearBombCount_ = 0;
  int itemGetCount_ = 0;
  int oneLaneClearCount_ = 0;
  bool wasJumping_ = false;
  bool wasRolling_ = false;
  int lastPlayerLane_ = 0;

  // 定数
  static constexpr Vector3 kDefaultSkyBoxScale = {100.0f, 100.0f, 100.0f};
  static constexpr float kDustGroundThresholdY = 3.01f;
  static constexpr float kStartItemSpawnChance = 0.15f;
  static constexpr float kPlayBaseScrollSpeed = 0.2f;
  static constexpr float kTitleBaseScrollSpeed = 0.22f;
  static constexpr float kResultFreezeDuration = 0.75f;
  static constexpr float kResultShatterDuration = 0.75f;
  static constexpr float kHitRumbleDuration = 0.35f;
  static constexpr int kHitRumbleMotorSpeed = 48000;

  std::shared_ptr<Cube> skyBox_ = std::make_shared<Cube>();
  int skyBoxTexture_ = -1;
  Transform skyBoxTransform_{
      kDefaultSkyBoxScale, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

  // エンティティ・ワールド管理
  std::shared_ptr<Player> player_ = std::make_shared<Player>();
  std::shared_ptr<Boss> boss_ = std::make_shared<Boss>();
  std::shared_ptr<DirectionalLight> mainDirLight_ =
      std::make_shared<DirectionalLight>();
  std::shared_ptr<DirectionalLight> subDirLight_ =
      std::make_shared<DirectionalLight>();
  std::shared_ptr<PointLight> coursePointLight_ =
      std::make_shared<PointLight>();
  std::shared_ptr<PointLight> bossPointLight_ = std::make_shared<PointLight>();
  std::unique_ptr<GameObjectManager> gameObjectManager_ =
      std::make_unique<GameObjectManager>();
  std::unique_ptr<StageSettings> stageSettings_ =
      std::make_unique<StageSettings>();
  std::unique_ptr<GameSceneEffect> effectManager_ =
      std::make_unique<GameSceneEffect>();
  std::unique_ptr<EditorUI> editorUI_ = std::make_unique<EditorUI>();

  // 画面遷移
  ResultTransition resultTransition_ = ResultTransition::None;

  float uiTimer_ = 0.0f;
  class Draw *draw_ = nullptr;

  /// <summary>
  /// シーン内のデフォルトライト配置・パラメータ設定
  /// </summary>
  void SetupDefaultLights();

  /// <summary>
  /// 太陽の軌道運動・時間帯に応じたライト方向・色・強度の更新
  /// </summary>
  void UpdateSunLight(float dt);

  /// <summary>
  /// ジャンプ・しゃがみタイミングのガイド線描画処理
  /// </summary>
  void DrawActionGuideLines(class Draw &draw);

  // ジャンプ・しゃがみタイミングガイド線設定
  bool showActionGuideLines_ = true;
  float guideLineHeight_ = 2.18f;
  float jumpGuideTimingDistance_ = 3.0f;
  float rollGuideTimingDistance_ = 3.0f;
  Vector4 jumpGuideColor_ = {0.15f, 0.95f, 0.35f, 1.0f};
  Vector4 rollGuideColor_ = {1.0f, 0.65f, 0.05f, 1.0f};
  bool showPlayerTargetLine_ = true;
  Vector4 playerTargetLineColor_ = {0.35f, 0.85f, 1.0f, 0.7f};

  bool isMasterLightingEnabled_ = true;
  bool isSunOrbitEnabled_ = true;
  bool isSunColorTransitionEnabled_ = true;
  float sunCycleDuration_ = 24.0f;
  float sunAngle_ = 0.45f;
  float sunOrbitRadius_ = 35.0f;
  float sunMaxElevation_ = 28.0f;
  float sunMinElevation_ = 7.0f;
  float sunOrbitOffsetZ_ = -15.0f;

  // 時間帯別ライティング設定
  Vector4 dayMainColor_ = {1.00f, 0.99f, 0.96f, 1.0f};
  Vector4 daySubColor_ = {0.70f, 0.85f, 1.00f, 1.0f};
  Vector4 daySkyColor_ = {1.00f, 1.00f, 1.00f, 1.0f};
  float dayMainIntensity_ = 1.15f;
  float daySubIntensity_ = 0.45f;

  Vector4 sunsetMainColor_ = {1.00f, 0.15f, 0.08f, 1.0f};
  Vector4 sunsetSubColor_ = {0.65f, 0.12f, 0.25f, 1.0f};
  Vector4 sunsetSkyColor_ = {0.75f, 0.18f, 0.16f, 1.0f};
  float sunsetMainIntensity_ = 0.58f;
  float sunsetSubIntensity_ = 0.20f;

  Vector4 nightMainColor_ = {0.30f, 0.45f, 0.80f, 1.0f};
  Vector4 nightSubColor_ = {0.10f, 0.16f, 0.35f, 1.0f};
  float nightMainIntensity_ = 0.16f;
  float nightSubIntensity_ = 0.05f;
  float nightSkyBrightness_ = 0.12f;
  float nightIntensityScale_ = 1.0f;

  Vector4 sunriseMainColor_ = {1.00f, 0.18f, 0.10f, 1.0f};
  Vector4 sunriseSubColor_ = {0.60f, 0.15f, 0.30f, 1.0f};
  Vector4 sunriseSkyColor_ = {0.75f, 0.20f, 0.18f, 1.0f};
  float sunriseMainIntensity_ = 0.58f;
  float sunriseSubIntensity_ = 0.20f;

public:
  void SetAllLighting(bool enabled);
  void ToggleAllLighting();
  bool IsMasterLightingEnabled() const { return isMasterLightingEnabled_; }

public:
  GameScene() = default;
  ~GameScene() override;

  void Initialize() override;
  void Update() override;
  void Draw(class Draw &draw) override;
  void DrawUI(class Draw &draw) override;
  void ImGui() override;

  // ステート管理（GameState / PlayingState）
  GameState GetGameState() const {
    return currentGameState_ ? currentGameState_->GetType() : GameState::Title;
  }
  void ChangeGameState(GameState newState);
  void ChangeGameState(std::unique_ptr<IGameSceneState> newState);

  PlayingState GetPlayingState() const {
    return currentPlayingState_ ? currentPlayingState_->GetType()
                                : PlayingState::ThreeLane;
  }
  void ChangePlayingState(PlayingState newState, bool force = false);
  void ChangePlayingState(std::unique_ptr<IPlayingState> newState,
                          bool force = false);
  void UpdatePlayingState(float timeScale, float scoreScrollSpeed);
  bool ShouldInvertControls() const {
    return currentPlayingState_ &&
           currentPlayingState_->ShouldInvertControls(*this);
  }
  bool IsOneLaneMode() const {
    return currentPlayingState_ && currentPlayingState_->IsOneLaneMode();
  }
  float GetRightSideDistance() const {
    return currentPlayingState_ ? currentPlayingState_->GetRightSideDistance()
                                : 0.0f;
  }

  // ゲーム進行制御
  void ResetGame();
  void StartGame();
  void StartPlaying();
  void ReturnToTitle();

  // ステートから利用するワールド更新・判定処理
  void UpdateWorldEntities(float timeScale, float speedMultiplier);
  void CheckCollisions();
  void CheckKeepRolling();
  void UpdateAchievementTracking(float timeScale);
  void StartResultTransition(ResultTransition target);
  bool IsResultTransitioning() const {
    return resultTransition_ != ResultTransition::None;
  }

  // JSON設定の保存・読み込み
  bool SaveSettingsToJson(const std::string &fileName = "GameSceneSettings");
  bool LoadSettingsFromJson(const std::string &fileName = "GameSceneSettings");
  void ResetSettingsToDefault();

public:
  enum class TitleMenuItem {
    PlayerName = 0,
    GameStart,
    Ranking,
    Achievements,
    HowToPlay,
    Settings,
    Exit,
    Count
  };

  enum class TitleSubView {
    None = 0,
    Ranking,
    HowToPlay,
    Achievements,
    Settings
  };

private:
  TitleMenuItem titleMenuIndex_ = TitleMenuItem::GameStart;
  TitleSubView titleSubView_ = TitleSubView::None;
  int titleAchievementPage_ = 0;
  std::string titlePlayerName_ = "Player";
  bool isEditingTitlePlayerName_ = false;

  // 設定ファイル（JSON）用
  char settingsFileName_[128] = "GameSceneSettings";
  std::string settingsStatusMessage_ = "";
  float settingsStatusTimer_ = 0.0f;

  // 設定画面（音量調整）用
  int settingsMenuIndex_ =
      0; // 0: Master, 1: BGM, 2: SE, 3: Guide, 4: Reset, 5: Close
  void CloseSettings();

public:
  void UpdateSettingsInput();
  void UpdateTitleTextInput();

  // タイトルメニュー・設定画面アクセサ（TitleSceneState / PausedSceneState用）
  TitleMenuItem GetTitleMenuIndex() const { return titleMenuIndex_; }
  void SetTitleMenuIndex(TitleMenuItem index) { titleMenuIndex_ = index; }
  TitleSubView GetTitleSubView() const { return titleSubView_; }
  void SetTitleSubView(TitleSubView view) { titleSubView_ = view; }
  int GetTitleAchievementPage() const { return titleAchievementPage_; }
  void SetTitleAchievementPage(int page) { titleAchievementPage_ = page; }
  const std::string &GetTitlePlayerName() const { return titlePlayerName_; }
  void SetTitlePlayerName(const std::string &name) { titlePlayerName_ = name; }
  bool IsEditingTitlePlayerName() const { return isEditingTitlePlayerName_; }
  void SetEditingTitlePlayerName(bool editing) {
    isEditingTitlePlayerName_ = editing;
  }
  void SetSettingsMenuIndex(int index) { settingsMenuIndex_ = index; }

  // 1位カットインテスト設定アクセサ
  bool IsForceFirstPlaceCutInTest() const { return forceFirstPlaceCutInTest_; }
  int GetTestCutinType() const { return testCutinType_; }

  // モジュールアクセサ
  class ScoreManager *GetScoreManager() { return scoreManager_.get(); }
  class AchievementManager *GetAchievementManager() {
    return achievementManager_.get();
  }
  class GameSceneEffect *GetEffectManager() { return effectManager_.get(); }
  class StageSettings *GetStageSettings() { return stageSettings_.get(); }
  class Player *GetPlayer() { return player_.get(); }
  class Boss *GetBoss() { return boss_.get(); }
  class BossBattleController *GetBossBattleController() {
    return bossBattleController_.get();
  }
  class TitleDirector *GetTitleDirector() { return titleDirector_.get(); }
  class CrashEffect *GetCrashEffect() { return crashEffect_.get(); }
  class RankingCutinEffect *GetRankingCutinEffect() {
    return rankingCutinEffect_.get();
  }
  const class RankingCutinEffect *GetRankingCutinEffect() const {
    return rankingCutinEffect_.get();
  }
  class GameObjectManager *GetGameObjectManager() {
    return gameObjectManager_.get();
  }
  class GameCameraController *GetCameraController() {
    return cameraController_.get();
  }
  const class GameCameraController *GetCameraController() const {
    return cameraController_.get();
  }

  Camera *GetCamera() override;

  void TriggerPlayerHit(bool isTrip, bool isLargeObstacle = false);

  // 実績トラッキング加算
  void AddOneLaneClearCount() { oneLaneClearCount_++; }

  void AddItemGetCount() {
    if (GetGameState() == GameState::Playing) {
      itemGetCount_++;
    }
  }

  void AddBarrierBlockCount() {
    if (GetGameState() == GameState::Playing) {
      barrierBlockCount_++;
    }
  }

  void AddClearBombCount() {
    if (GetGameState() == GameState::Playing) {
      clearBombCount_++;
    }
  }
};

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
  std::unique_ptr<CrashEffect> crashEffect_ =
      std::make_unique<CrashEffect>();
  std::unique_ptr<RankingCutinEffect> rankingCutinEffect_ =
      std::make_unique<RankingCutinEffect>();
  bool forceFirstPlaceCutInTest_ = false;
  std::unique_ptr<BossBattleController> bossBattleController_ =
      std::make_unique<BossBattleController>();
  std::unique_ptr<TitleDirector> titleDirector_ =
      std::make_unique<TitleDirector>();
  std::unique_ptr<GameHUD> gameHUD_ =
      std::make_unique<GameHUD>();

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

  void UpdateWorldEntities(float timeScale, float speedMultiplier);
  void CheckCollisions();
  void CheckKeepRolling();
  void StartResultTransition(ResultTransition target);

public:
  GameScene() = default;
  ~GameScene() override;

  void Initialize() override;
  void Update() override;
  void Draw(class Draw &draw) override;
  void DrawUI(class Draw &draw) override;
  void ImGui() override;

  void ChangePlayingState(PlayingState newState, bool force = false);
  void ResetGame();
  void StartGame();
  void StartPlaying();
  void ReturnToTitle();

  void TitleUpdate();
  void PlayingUpdate();
  void PausedUpdate();
  void PlayerHitUpdate();
  void FirstPlaceCutInUpdate();
  void EditorUpdate();

  // 設定のJSON保存・読み込み
  bool SaveSettingsToJson(const std::string &fileName = "GameSceneSettings");
  bool LoadSettingsFromJson(const std::string &fileName = "GameSceneSettings");
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

  enum class TitleSubView {
    None = 0,
    Ranking,
    HowToPlay
  };

private:
  TitleMenuItem titleMenuIndex_ = TitleMenuItem::GameStart;
  TitleSubView titleSubView_ = TitleSubView::None;
  std::string titlePlayerName_ = "Player";
  bool isEditingTitlePlayerName_ = false;
  void UpdateTitleTextInput();

public:
  class ScoreManager* GetScoreManager() { return scoreManager_.get(); }
  class GameSceneEffect* GetEffectManager() { return effectManager_.get(); }
  class StageSettings* GetStageSettings() { return stageSettings_.get(); }
  class Player* GetPlayer() { return player_.get(); }
  class Boss* GetBoss() { return boss_.get(); }
  class GameCameraController* GetCameraController() { return cameraController_.get(); }
  PlayingState GetPlayingState() const { return playingState_; }
  void TriggerPlayerHit(bool isTrip);
};

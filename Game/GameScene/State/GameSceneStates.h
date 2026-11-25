#pragma once
#include "../Effect/RankingCutinEffect.h"
#include "IGameSceneState.h"

/// <summary>
/// タイトル画面ステート
/// </summary>
class TitleSceneState final : public IGameSceneState {
public:
  GameState GetType() const override { return GameState::Title; }
  std::unique_ptr<IGameSceneState> Update(GameScene &scene) override;
  void UpdateCamera(GameScene &scene, float dt) override;
};

/// <summary>
/// ゲームプレイ中ステート
/// </summary>
class PlayingSceneState final : public IGameSceneState {
public:
  GameState GetType() const override { return GameState::Playing; }
  std::unique_ptr<IGameSceneState> Update(GameScene &scene) override;
  void UpdateCamera(GameScene &scene, float dt) override;
};

/// <summary>
/// ポーズ中ステート
/// </summary>
class PausedSceneState final : public IGameSceneState {
private:
  bool isSettingsOpen_ = false;

public:
  GameState GetType() const override { return GameState::Paused; }
  void Enter(GameScene &scene) override;
  std::unique_ptr<IGameSceneState> Update(GameScene &scene) override;
  void Exit(GameScene &scene, GameState nextState) override;
  float GetSunLightDeltaTime(float timeScale) const override;
  bool IsPauseSettingsOpen() const override { return isSettingsOpen_; }
  void ClosePauseSettings() override { isSettingsOpen_ = false; }
};

/// <summary>
/// プレイヤー被弾演出中ステート
/// </summary>
class PlayerHitSceneState final : public IGameSceneState {
public:
  GameState GetType() const override { return GameState::PlayerHit; }
  std::unique_ptr<IGameSceneState> Update(GameScene &scene) override;
};

/// <summary>
/// ランキング1位更新カットイン演出中ステート
/// </summary>
class FirstPlaceCutInSceneState final : public IGameSceneState {
private:
  RankingCutinEffect::CutinType cutinType_ =
      RankingCutinEffect::CutinType::Score;
  bool autoTriggerOnEnter_ = false;

public:
  FirstPlaceCutInSceneState() = default;
  explicit FirstPlaceCutInSceneState(RankingCutinEffect::CutinType type)
      : cutinType_(type), autoTriggerOnEnter_(true) {}

  GameState GetType() const override { return GameState::FirstPlaceCutIn; }
  void Enter(GameScene &scene) override;
  std::unique_ptr<IGameSceneState> Update(GameScene &scene) override;
  bool ShouldRenderRankingCutIn(const GameScene &scene) const override {
    (void)scene;
    return true;
  }
  bool ShouldDrawAchievementPopup() const override { return false; }
};

/// <summary>
/// ゲームオーバー（リザルト）画面ステート
/// </summary>
class GameOverSceneState final : public IGameSceneState {
private:
  float resultTimer_ = 0.0f;
  int resultAchievementScrollOffset_ = 0;
  float resultScrollRepeatTimer_ = 0.0f;

public:
  GameState GetType() const override { return GameState::GameOver; }
  void Enter(GameScene &scene) override;
  std::unique_ptr<IGameSceneState> Update(GameScene &scene) override;
  bool ShouldDrawAchievementPopup() const override { return false; }
  float GetResultTimer() const override { return resultTimer_; }
  int GetResultAchievementScrollOffset() const override {
    return resultAchievementScrollOffset_;
  }
};

/// <summary>
/// エディターモードステート
/// </summary>
class EditorSceneState final : public IGameSceneState {
public:
  GameState GetType() const override { return GameState::Editor; }
  void Enter(GameScene &scene) override;
  std::unique_ptr<IGameSceneState> Update(GameScene &scene) override;
  float GetSunLightDeltaTime(float timeScale) const override;
  void UpdateCamera(GameScene &scene, float dt) override;
};

/// <summary>
/// GameState列挙値から対応するステートインスタンスを生成するファクトリ関数
/// </summary>
std::unique_ptr<IGameSceneState> CreateGameSceneState(GameState state);


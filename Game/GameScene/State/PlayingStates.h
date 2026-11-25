#pragma once
#include "IPlayingState.h"

/// <summary>
/// 通常3レーン走行モードステート
/// </summary>
class ThreeLanePlayingState final : public IPlayingState {
private:
  bool needsBossItemCooldownReset_ = false;

public:
  explicit ThreeLanePlayingState(bool fromBossBattle = false)
      : needsBossItemCooldownReset_(fromBossBattle) {}

  PlayingState GetType() const override { return PlayingState::ThreeLane; }
  void Enter(GameScene &scene, PlayingState prevState, Transform &outTarget,
             int &outLaneCount) override;
  void OnCameraTransitionComplete(GameScene &scene) override;
};

/// <summary>
/// 1レーン（右サイドビュー）モードステート
/// </summary>
class OneLanePlayingState final : public IPlayingState {
private:
  static constexpr float kOneLaneTotalDistance = 200.0f;
  float rightSideDistance_ = 0.0f;

public:
  PlayingState GetType() const override { return PlayingState::OneLane; }
  void Enter(GameScene &scene, PlayingState prevState, Transform &outTarget,
             int &outLaneCount) override;
  std::unique_ptr<IPlayingState> Update(GameScene &scene, float timeScale,
                                        float scoreScrollSpeed) override;
  bool IsOneLaneMode() const override { return true; }
  float GetRightSideDistance() const override { return rightSideDistance_; }
};

/// <summary>
/// ボス戦モードステート
/// </summary>
class BossPlayingState final : public IPlayingState {
private:
  static constexpr float kInvertedCameraThresholdRotateY = 1.57f;

public:
  PlayingState GetType() const override { return PlayingState::Boss; }
  void Enter(GameScene &scene, PlayingState prevState, Transform &outTarget,
             int &outLaneCount) override;
  void Exit(GameScene &scene, PlayingState nextState) override;
  std::unique_ptr<IPlayingState> Update(GameScene &scene, float timeScale,
                                        float scoreScrollSpeed) override;
  void OnCameraTransitionComplete(GameScene &scene) override;
  bool ShouldInvertControls(const GameScene &scene) const override;
};

/// <summary>
/// PlayingState列挙値から対応するステートインスタンスを生成するファクトリ関数
/// </summary>
std::unique_ptr<IPlayingState>
CreatePlayingState(PlayingState state,
                   PlayingState prevState = PlayingState::ThreeLane);


#pragma once
#include "IPlayerState.h"
#include <Math/Calculation.h>

/// <summary>
/// プレイヤーの走行（通常移動）ステート
/// </summary>
class PlayerRunState final : public IPlayerState {
public:
  PlayerActionType GetType() const override { return PlayerActionType::Run; }
  void Enter(Player &player) override;
  PlayerStateTransition Update(Player &player, float speedMultiplier) override;
};

/// <summary>
/// プレイヤーのジャンプ（跳躍・滞空）ステート
/// </summary>
class PlayerJumpState final : public IPlayerState {
private:
  float velocityY_ = 0.0f;

public:
  PlayerActionType GetType() const override { return PlayerActionType::Jump; }
  void Enter(Player &player) override;
  PlayerStateTransition Update(Player &player, float speedMultiplier) override;
};

/// <summary>
/// プレイヤーのロール（スライディング・低姿勢回避）ステート
/// </summary>
class PlayerRollState final : public IPlayerState {
private:
  float rollTimer_ = 0.0f;

public:
  PlayerActionType GetType() const override { return PlayerActionType::Roll; }
  void Enter(Player &player) override;
  PlayerStateTransition Update(Player &player, float speedMultiplier) override;
  void Exit(Player &player) override;
  bool NeedsRollingHeightOffset() const override { return true; }
};

/// <summary>
/// プレイヤーの被弾（ノックバック・カメラ激突・腹滑り雪玉化）ステート
/// </summary>
class PlayerHitState final : public IPlayerState {
private:
  // ヒット演出物理定数
  static constexpr float kDefaultHitDuration = 90.0f;
  static constexpr float kKnockbackRandXScale = 0.1f;
  static constexpr float kKnockbackRandCenterOffset = 0.5f;
  static constexpr Vector3 kHitKnockbackVelocity = {0.0f, 0.4f, -0.6f};
  static constexpr float kHitGravityMultiplier = 2.0f;
  static constexpr float kHitRotationSpeed = 0.1f;
  static constexpr float kHitLandingFriction = 0.8f;

  // カメラ激突（Camera Slam）演出定数
  static constexpr float kCameraSlamDurationFrames = 20.0f;
  static constexpr float kCameraStickSlideSpeedY = -0.005f;
  static constexpr float kCameraSlamSpinSpeedX = 0.25f;
  static constexpr float kCameraSlamSpinSpeedZ = 0.15f;

  // 腹滑り雪玉化（Snowball Hit）演出定数
  static constexpr float kTripSnowballDuration = 110.0f;
  static constexpr float kSnowballFormDelay = 12.0f;
  static constexpr float kSnowballFullFormFrames = 42.0f;
  static constexpr float kSnowballMaxScale = 2.0f;
  static constexpr float kSnowballInitialSpeedZ = 0.40f;
  static constexpr float kSnowballMaxSpeedZ = 0.85f;
  static constexpr float kSnowballRollSpeed = 0.30f;

  bool isTrip_ = false;
  bool isCameraSlam_ = false;
  bool hasHitCamera_ = false;
  float hitTimer_ = 0.0f;
  float hitDuration_ = kDefaultHitDuration;
  Vector3 knockbackVelocity_{0.0f, 0.0f, 0.0f};
  Vector3 cameraTargetPos_{0.0f, 3.0f, -12.5f};
  float snowballRotationX_ = 0.0f;

public:
  PlayerHitState(bool isTrip = false, bool isLargeObstacle = false,
                 const Vector3 &cameraPos = {0.0f, 3.0f, -12.5f});

  PlayerActionType GetType() const override { return PlayerActionType::Hit; }
  void Enter(Player &player) override;
  PlayerStateTransition Update(Player &player, float speedMultiplier) override;
  void Exit(Player &player) override;
  bool CanAct() const override { return false; }
  bool UsesFixedTimeScale() const override { return true; }
  bool NeedsRollingHeightOffset() const override { return isTrip_; }
  bool IsTripHit() const override { return isTrip_; }
  bool IsHitAnimationFinished() const override {
    return hitTimer_ >= hitDuration_;
  }
};

/// <summary>
/// プレイヤーの中央レーン強制移動ステート
/// </summary>
class PlayerForcedCenterState final : public IPlayerState {
private:
  static constexpr float kDefaultDuration = 30.0f;

  float timer_ = 0.0f;
  float duration_ = kDefaultDuration;
  float startX_ = 0.0f;

public:
  explicit PlayerForcedCenterState(float duration = kDefaultDuration);

  PlayerActionType GetType() const override {
    return PlayerActionType::ForcedCenter;
  }
  void Enter(Player &player) override;
  PlayerStateTransition Update(Player &player, float speedMultiplier) override;
  bool CanAct() const override { return false; }
};

/// <summary>
/// PlayerActionType列挙値から対応するステートインスタンスを生成するファクトリ関数
/// </summary>
std::unique_ptr<IPlayerState> CreatePlayerState(PlayerActionType type);


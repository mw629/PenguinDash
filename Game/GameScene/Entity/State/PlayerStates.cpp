#include "PlayerStates.h"
#include "../../GameSceneManager.h"
#include "../../System/SoundManager.h"
#include "../Player.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

// ==========================================
// PlayerRunState
// ==========================================

void PlayerRunState::Enter(Player &player) {
  Transform t = player.GetTransform();
  t.scale.y = 1.0f;
  t.translate.y = player.GetBaseHeight();
  player.SetTransform(t);

  if (player.GetModel()) {
    player.GetModel()->SetAnimation("walk", player.GetWalkTransitionDuration());
  }
}

PlayerStateTransition PlayerRunState::Update(Player &player,
                                             float speedMultiplier) {
  if (speedMultiplier <= 0.0f) {
    return PlayerStateTransition::None();
  }

  // レーン入力処理
  player.ProcessLaneInput();

  // アクション判定（先行入力バッファまたはキー押下）
  if (player.CanAct() && !player.GetAutoPilot()) {
    if (player.ConsumeBufferedJump() ||
        GameSceneManager::GetInstance()->IsPushJump()) {
      return PlayerStateTransition::To(std::make_unique<PlayerJumpState>(),
                                       true);
    }
    if (player.ConsumeBufferedRoll() ||
        GameSceneManager::GetInstance()->IsPushRoll()) {
      return PlayerStateTransition::To(std::make_unique<PlayerRollState>(),
                                       true);
    }
  }

  // 通常レーン移動更新
  player.UpdateLaneMovement(speedMultiplier);
  return PlayerStateTransition::None();
}

// ==========================================
// PlayerJumpState
// ==========================================

void PlayerJumpState::Enter(Player &player) {
  float mult = player.GetCurrentSpeedMultiplier() > 0.0f
                   ? player.GetCurrentSpeedMultiplier()
                   : 1.0f;
  velocityY_ = player.GetJumpPower() * mult;
  SoundManager::GetInstance()->PlaySE(SoundManager::SE::Jump);
}

PlayerStateTransition PlayerJumpState::Update(Player &player,
                                              float speedMultiplier) {
  if (speedMultiplier <= 0.0f) {
    return PlayerStateTransition::None();
  }

  // 空中でもレーン移動入力・更新は有効
  player.ProcessLaneInput();
  player.UpdateLaneMovement(speedMultiplier);

  // 垂直ジャンプ物理
  Transform t = player.GetTransform();
  t.translate.y += velocityY_;
  velocityY_ -= player.GetGravity() * (speedMultiplier * speedMultiplier);

  // 地面に着地
  if (t.translate.y <= player.GetBaseHeight()) {
    t.translate.y = player.GetBaseHeight();
    velocityY_ = 0.0f;
    player.SetTransform(t);
    player.SetCurrentRecoveryTimer(player.GetJumpRecovery());
    SoundManager::GetInstance()->PlaySE(SoundManager::SE::Land);

    // 着地時カメラシェイク演出コールバック
    if (player.GetOnLandCallback()) {
      player.GetOnLandCallback()();
    }

    // 先行入力バッファ判定（着地直後の連続ジャンプまたはスライディング）
    if (!player.GetAutoPilot()) {
      if (player.ConsumeBufferedJump()) {
        return PlayerStateTransition::To(std::make_unique<PlayerJumpState>(),
                                         true);
      }
      if (player.ConsumeBufferedRoll()) {
        return PlayerStateTransition::To(std::make_unique<PlayerRollState>(),
                                         true);
      }
    }

    return PlayerStateTransition::To(std::make_unique<PlayerRunState>(), false);
  }

  player.SetTransform(t);
  return PlayerStateTransition::None();
}

// ==========================================
// PlayerRollState
// ==========================================

void PlayerRollState::Enter(Player &player) {
  rollTimer_ = player.GetRollDuration();
  SoundManager::GetInstance()->PlaySE(SoundManager::SE::Slide);
  if (player.GetModel()) {
    player.GetModel()->SetAnimation("sneakWalk",
                                    player.GetRollTransitionDuration());
  }
  Transform t = player.GetTransform();
  t.translate.y = player.GetBaseHeight() - Player::kDefaultRollingHeightOffset;
  player.SetTransform(t);
}

PlayerStateTransition PlayerRollState::Update(Player &player,
                                              float speedMultiplier) {
  if (speedMultiplier <= 0.0f) {
    return PlayerStateTransition::None();
  }

  // 転がり中でもジャンプでキャンセル可能（先行入力バッファまたはキー押下）
  if (player.CanAct() && !player.GetAutoPilot()) {
    if (player.ConsumeBufferedJump() ||
        GameSceneManager::GetInstance()->IsPushJump()) {
      if (!player.GetKeepRolling()) {
        return PlayerStateTransition::To(std::make_unique<PlayerJumpState>(),
                                         true);
      }
    }
  }

  // レーン入力処理（横移動が開始されたら転がりを解除）
  int prevTarget = player.GetTargetLaneIndex();
  player.ProcessLaneInput();
  if (player.GetTargetLaneIndex() != prevTarget && !player.GetKeepRolling()) {
    player.UpdateLaneMovement(speedMultiplier);
    return PlayerStateTransition::To(std::make_unique<PlayerRunState>(), false);
  }
  player.UpdateLaneMovement(speedMultiplier);

  // 転がりタイマー減算
  rollTimer_ -= speedMultiplier;
  if (rollTimer_ <= 0.0f && !player.GetKeepRolling()) {
    player.SetCurrentRecoveryTimer(player.GetRollRecovery());

    // ロール終了時の先行入力判定
    if (!player.GetAutoPilot()) {
      if (player.ConsumeBufferedJump()) {
        return PlayerStateTransition::To(std::make_unique<PlayerJumpState>(),
                                         true);
      }
      if (player.ConsumeBufferedRoll()) {
        return PlayerStateTransition::To(std::make_unique<PlayerRollState>(),
                                         true);
      }
    }

    return PlayerStateTransition::To(std::make_unique<PlayerRunState>(), false);
  }

  return PlayerStateTransition::None();
}

void PlayerRollState::Exit(Player &player) {
  Transform t = player.GetTransform();
  t.scale.y = 1.0f;
  t.translate.y = player.GetBaseHeight();
  player.SetTransform(t);

  if (player.GetModel()) {
    player.GetModel()->SetAnimation("walk", player.GetWalkTransitionDuration());
  }
}

// ==========================================
// PlayerHitState
// ==========================================

PlayerHitState::PlayerHitState(bool isTrip, bool isLargeObstacle,
                               const Vector3 &cameraPos)
    : isTrip_(isTrip), isCameraSlam_(!isTrip && isLargeObstacle),
      hitDuration_(isTrip ? kTripSnowballDuration : kDefaultHitDuration),
      cameraTargetPos_(cameraPos) {}

void PlayerHitState::Enter(Player &player) {
  Transform t = player.GetTransform();
  t.scale = {1.0f, 1.0f, 1.0f};
  player.SetTransform(t);
  player.SetCurrentRecoveryTimer(0.0f);
  hitTimer_ = 0.0f;
  hasHitCamera_ = false;
  snowballRotationX_ = 0.0f;

  if (isCameraSlam_) {
    if (player.GetModel()) {
      player.GetModel()->SetAnimation("walk",
                                      player.GetWalkTransitionDuration());
    }
    Vector3 startPos = t.translate;
    float frames = kCameraSlamDurationFrames > 0.0f ? kCameraSlamDurationFrames
                                                    : 20.0f;

    Vector3 vel;
    vel.x = (cameraTargetPos_.x - startPos.x) / frames;
    vel.z = (cameraTargetPos_.z - startPos.z) / frames;
    // 重力を考慮して目標Yへ到達する放物線初速
    vel.y = (cameraTargetPos_.y - startPos.y) / frames +
            0.5f * (player.GetGravity() * kHitGravityMultiplier) * frames;
    knockbackVelocity_ = vel;
  } else if (isTrip_) {
    // === 腹滑り雪玉化（Belly Slide & Snowball Roll）演出の開始 ===
    player.SetSnowballActive(true);
    player.SetSnowballHidingPenguin(false);
    player.SetSnowballScale(0.0f);

    // お腹ばいスライディング姿勢（sneakWalk）に素早く遷移
    if (player.GetModel()) {
      player.GetModel()->SetAnimation("sneakWalk", 0.04f);
    }
    SoundManager::GetInstance()->PlaySE(SoundManager::SE::Slide);

    // お腹が地面に接地する高さに設定し、前方に滑り出す初速を与える
    t.translate.y =
        player.GetBaseHeight() - Player::kDefaultRollingHeightOffset;
    t.rotate.x = 0.05f;
    t.rotate.z = 0.08f;
    player.SetTransform(t);
    knockbackVelocity_ = {0.0f, 0.0f, kSnowballInitialSpeedZ};
  } else {
    // 通常被弾
    if (player.GetModel()) {
      player.GetModel()->SetAnimation("walk",
                                      player.GetWalkTransitionDuration());
    }
    float randX = (static_cast<float>(rand()) / RAND_MAX -
                   kKnockbackRandCenterOffset) *
                  kKnockbackRandXScale;
    knockbackVelocity_ = {randX, kHitKnockbackVelocity.y,
                          kHitKnockbackVelocity.z};
  }
}

PlayerStateTransition PlayerHitState::Update(Player &player,
                                             float speedMultiplier) {
  if (speedMultiplier == 0.0f) {
    return PlayerStateTransition::None();
  }

  hitTimer_ += 1.0f;
  Transform transform = player.GetTransform();

  if (isCameraSlam_) {
    if (!hasHitCamera_) {
      // カメラに向かって吹き飛び中
      transform.translate.x += knockbackVelocity_.x;
      transform.translate.y += knockbackVelocity_.y;
      transform.translate.z += knockbackVelocity_.z;

      knockbackVelocity_.y -= player.GetGravity() * kHitGravityMultiplier;

      // コミカルにきりもみ回転
      transform.rotate.x += kCameraSlamSpinSpeedX;
      transform.rotate.z += kCameraSlamSpinSpeedZ;

      bool reachedZ =
          (knockbackVelocity_.z < 0.0f &&
           transform.translate.z <= cameraTargetPos_.z) ||
          (knockbackVelocity_.z > 0.0f &&
           transform.translate.z >= cameraTargetPos_.z);
      if (reachedZ || hitTimer_ >= kCameraSlamDurationFrames) {
        // カメラ画面にドカンと激突
        hasHitCamera_ = true;
        transform.translate.z = cameraTargetPos_.z;
        transform.translate.x = cameraTargetPos_.x;
        knockbackVelocity_.x = 0.0f;
        knockbackVelocity_.z = 0.0f;
        knockbackVelocity_.y = kCameraStickSlideSpeedY;

        // カメラ（正面）を向いて画面にペタッと張り付く姿勢
        transform.rotate.x = 0.0f;
        transform.rotate.y = 3.14159f;
        transform.rotate.z = 0.12f;

        player.SetTransform(transform);
        // 激突コールバック実行（激突シェイク・SE・振動）
        player.ExecuteCameraHitCallback();
        return PlayerStateTransition::None();
      }
    } else {
      // カメラ画面に張り付いて滑り落ち中
      transform.translate.y += knockbackVelocity_.y;
      transform.rotate.z += 0.002f;
    }
    player.SetTransform(transform);
  } else if (isTrip_) {
    // === 腹滑り雪玉化の毎フレーム更新 ===
    float speedT = std::clamp(hitTimer_ / 60.0f, 0.0f, 1.0f);
    float currentSpeedZ =
        Lerp(kSnowballInitialSpeedZ, kSnowballMaxSpeedZ, speedT);
    transform.translate.z += currentSpeedZ;

    // 腹ばいスライディング時のコミカルな揺れ
    if (hitTimer_ < kSnowballFullFormFrames) {
      transform.rotate.z = std::sin(hitTimer_ * 0.35f) * 0.12f;
      transform.rotate.x = 0.05f;
    }

    // 雪玉の生成・成長・転がり処理
    if (hitTimer_ >= kSnowballFormDelay) {
      float formProgress = (hitTimer_ - kSnowballFormDelay) /
                           (kSnowballFullFormFrames - kSnowballFormDelay);
      formProgress = std::clamp(formProgress, 0.0f, 1.0f);

      float scale = formProgress * kSnowballMaxScale;
      player.SetSnowballScale(scale);

      snowballRotationX_ +=
          kSnowballRollSpeed * (currentSpeedZ / kSnowballInitialSpeedZ);

      float groundY =
          player.GetBaseHeight() + player.GetModelOffset().translate.y;

      Transform sbTrans;
      sbTrans.scale = {scale, scale, scale};
      sbTrans.rotate = {snowballRotationX_, 0.0f,
                        std::sin(hitTimer_ * 0.25f) * 0.08f};
      sbTrans.translate = {transform.translate.x, groundY + scale,
                           transform.translate.z};
      player.SetSnowballTransform(sbTrans);

      transform.translate.y = groundY + scale * 0.5f;

      if (hitTimer_ >= kSnowballFullFormFrames) {
        player.SetSnowballHidingPenguin(true);
      }
    } else {
      player.SetSnowballScale(0.0f);
      Transform sbTrans = transform;
      sbTrans.scale = {0.0f, 0.0f, 0.0f};
      player.SetSnowballTransform(sbTrans);
    }
    player.SetTransform(transform);
  } else {
    // 通常被弾
    transform.translate.x += knockbackVelocity_.x;
    transform.translate.y += knockbackVelocity_.y;
    transform.translate.z += knockbackVelocity_.z;

    knockbackVelocity_.y -= player.GetGravity() * kHitGravityMultiplier;
    transform.rotate.x -= kHitRotationSpeed;

    if (transform.translate.y <= player.GetBaseHeight() &&
        knockbackVelocity_.y < 0.0f) {
      transform.translate.y = player.GetBaseHeight();
      knockbackVelocity_.y = 0.0f;
      knockbackVelocity_.x *= kHitLandingFriction;
      knockbackVelocity_.z *= kHitLandingFriction;
    }
    player.SetTransform(transform);
  }

  return PlayerStateTransition::None();
}

void PlayerHitState::Exit(Player &player) {
  Transform t = player.GetTransform();
  t.rotate = {0.0f, 0.0f, 0.0f};
  player.SetTransform(t);

  player.SetSnowballActive(false);
  player.SetSnowballHidingPenguin(false);
  player.SetSnowballScale(0.0f);
  Transform sbTrans{};
  sbTrans.scale = {0.0f, 0.0f, 0.0f};
  player.SetSnowballTransform(sbTrans);
}

// ==========================================
// PlayerForcedCenterState
// ==========================================

PlayerForcedCenterState::PlayerForcedCenterState(float duration)
    : duration_(duration > 0.0f ? duration : kDefaultDuration) {}

void PlayerForcedCenterState::Enter(Player &player) {
  timer_ = 0.0f;
  startX_ = player.GetTransform().translate.x;
}

PlayerStateTransition PlayerForcedCenterState::Update(Player &player,
                                                      float speedMultiplier) {
  if (speedMultiplier <= 0.0f) {
    return PlayerStateTransition::None();
  }

  timer_ += speedMultiplier;
  float t = timer_ / duration_;
  if (t > 1.0f) {
    t = 1.0f;
  }

  float easeT = t * t * (3.0f - 2.0f * t);
  Transform transform = player.GetTransform();
  transform.translate.x = Lerp(startX_, 0.0f, easeT);
  player.SetTransform(transform);

  if (t >= 1.0f) {
    player.SetLaneIndex(0);
    player.SetTargetLaneIndex(0);
    return PlayerStateTransition::To(std::make_unique<PlayerRunState>(), false);
  }
  return PlayerStateTransition::None();
}

// ==========================================
// Factory
// ==========================================

std::unique_ptr<IPlayerState> CreatePlayerState(PlayerActionType type) {
  switch (type) {
  case PlayerActionType::Run:
    return std::make_unique<PlayerRunState>();
  case PlayerActionType::Jump:
    return std::make_unique<PlayerJumpState>();
  case PlayerActionType::Roll:
    return std::make_unique<PlayerRollState>();
  case PlayerActionType::Hit:
    return std::make_unique<PlayerHitState>();
  case PlayerActionType::ForcedCenter:
    return std::make_unique<PlayerForcedCenterState>();
  default:
    return std::make_unique<PlayerRunState>();
  }
}


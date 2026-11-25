#include "Player.h"
#include "../GameSceneManager.h"
#include "../System/SoundManager.h"
#include "AssetManager.h"
#include "Collision.h"
#include "Graphics/Render/Draw.h"
#include "State/PlayerStates.h"
#include <algorithm>

#ifdef _USE_IMGUI
#include <imgui.h>
#endif

Player::Player() {}

Player::~Player() {}

void Player::UpdateDrawTransform(float speedMultiplier) {
  (void)speedMultiplier;
  Transform drawTransform = transform_;

  // しゃがみ時のモデルのサイズ変化（潰れ）を防ぐため、モデルスケールは常に
  // modelOffset_.scale を維持
  drawTransform.scale = modelOffset_.scale;

  // 平行移動オフセットの適用
  const bool needsRollingOffset =
      currentState_ && currentState_->NeedsRollingHeightOffset();
  drawTransform.translate.x += modelOffset_.translate.x;
  drawTransform.translate.y +=
      modelOffset_.translate.y +
      (needsRollingOffset ? kDefaultRollingHeightOffset : 0.0f);
  drawTransform.translate.z += modelOffset_.translate.z;

  // 回転オフセットの適用
  drawTransform.rotate.x += modelOffset_.rotate.x;
  drawTransform.rotate.y += modelOffset_.rotate.y;
  drawTransform.rotate.z += modelOffset_.rotate.z;

  model_->SetTransform(drawTransform);
}

void Player::Initialize(ModelData modelData) {
  (void)modelData;
  transform_.translate.y = baseHeight_;

  // GLTF ペンギンアニメーションモデルの初期化
  ModelData animModel =
      AssimpLoadObjFile("Resources/gltf/Penguin", "RunPenguin.gltf");
  int texIndex =
      AssetManager::LoadTexture("Resources/gltf/Penguin/Penguin_basecolor.jpg");
  animModel.textureIndex = texIndex;
  for (auto &subMesh : animModel.subMeshes) {
    subMesh.textureIndex = texIndex;
  }

  model_->Initialize(animModel, "Resources/gltf/Penguin", "RunPenguin.gltf");
  model_->LoadAdditionalAnimation("Resources/gltf/Penguin", "RunPenguin.gltf",
                                  "walk");
  model_->LoadAdditionalAnimation("Resources/gltf/Penguin", "SlidePenguin.gltf",
                                  "sneakWalk");
  model_->SetAnimation("walk");
  model_->SetName("Player Penguin Animation Model");
  model_->SetVisibleBones(false);
  model_->SetLighting(lightingEnabled_);

  // コライダーの初期化
  auto collider = AddComponent<BoxColliderComponent>();
  collider->SetSize({kDefaultColliderWidth, kDefaultColliderHeightStand,
                     kDefaultColliderDepth});

  // 腹滑り雪玉化演出用球体モデルの初期化
  ModelData sphereData =
      AssetManager::LoadModel("Resources/Model/obj", "sphere.obj");
  int whiteTex = AssetManager::LoadTexture("Resources/Texture/white64x64.png");
  sphereData.textureIndex = whiteTex;
  for (auto &subMesh : sphereData.subMeshes) {
    subMesh.textureIndex = whiteTex;
  }
  snowballModel_->Initialize(sphereData);
  snowballModel_->SetShader("ObjectShader");
  snowballModel_->SetLighting(lightingEnabled_);
  snowballModel_->SetName("Player Snowball Model");
  if (snowballModel_->GetMaterial()) {
    snowballModel_->GetMaterial()->SetColor({0.96f, 0.98f, 1.0f, 1.0f});
  }
  // 雪玉用スフィアコライダーを初期化（Double Dispatchによる異種形状判定用）
  snowballModel_->AddComponent<SphereColliderComponent>();

  UpdateDrawTransform(0.0f);
}

void Player::Reset() {
  transform_.scale = {1.0f, 1.0f, 1.0f};
  transform_.rotate = {0.0f, 0.0f, 0.0f};
  transform_.translate = {0.0f, baseHeight_, 0.0f};
  MarkDirty();

  laneIndex_ = 0;
  targetLaneIndex_ = 0;
  lerpTime_ = 0.0f;
  startX_ = 0.0f;
  moveDirection_ = MoveDirection::None;

  keepRolling_ = false;
  currentRecoveryTimer_ = 0.0f;

  isSnowballActive_ = false;
  isSnowballHidingPenguin_ = false;
  snowballScale_ = 0.0f;
  snowballTransform_.scale = {0.0f, 0.0f, 0.0f};

  ClearInputBuffer();

  SetHasBarrier(false);
  isInvertedControls_ = false;
  isAutoPilot_ = false;
  isInputEnabled_ = true;
  isVisible_ = true;

  ChangeActionState(PlayerActionType::Run);

  if (model_) {
    model_->SetAnimation("walk", 0.0f);
  }

  UpdateDrawTransform(0.0f);
}

void Player::ChangeActionState(PlayerActionType nextType) {
  ChangeActionState(CreatePlayerState(nextType));
}

void Player::ChangeActionState(std::unique_ptr<IPlayerState> newState) {
  if (currentState_) {
    currentState_->Exit(*this);
  }
  currentState_ = std::move(newState);
  if (currentState_) {
    currentState_->Enter(*this);
  }
}

void Player::StepActionState(float speedMultiplier) {
  for (int i = 0; i < kMaxStateTransitionsPerFrame && currentState_; ++i) {
    const float effectiveSpeed =
        currentState_->UsesFixedTimeScale() ? 1.0f : speedMultiplier;
    PlayerStateTransition transition =
        currentState_->Update(*this, effectiveSpeed);
    if (!transition.nextState) {
      break;
    }
    ChangeActionState(std::move(transition.nextState));
    if (!transition.updateImmediately) {
      break;
    }
  }
}

void Player::UpdateInputBuffer(float speedMultiplier) {
  if (speedMultiplier <= 0.0f) {
    return;
  }

  float decay = speedMultiplier;
  if (bufferedJumpTimer_ > 0.0f) {
    bufferedJumpTimer_ -= decay;
    if (bufferedJumpTimer_ < 0.0f) {
      bufferedJumpTimer_ = 0.0f;
    }
  }
  if (bufferedRollTimer_ > 0.0f) {
    bufferedRollTimer_ -= decay;
    if (bufferedRollTimer_ < 0.0f) {
      bufferedRollTimer_ = 0.0f;
    }
  }
  if (bufferedLaneTimer_ > 0.0f) {
    bufferedLaneTimer_ -= decay;
    if (bufferedLaneTimer_ < 0.0f) {
      bufferedLaneTimer_ = 0.0f;
      bufferedLaneDirection_ = 0;
    }
  }

  if (isAutoPilot_ || !isInputEnabled_) {
    return;
  }

  // アクション先行入力（ジャンプ・ロール）
  if (GameSceneManager::GetInstance()->IsPushJump()) {
    bufferedJumpTimer_ = kDefaultInputBufferFrames;
  }
  if (GameSceneManager::GetInstance()->IsPushRoll()) {
    bufferedRollTimer_ = kDefaultInputBufferFrames;
  }
}

void Player::ProcessLaneInput() {
  if (!CanAct() || isAutoPilot_) {
    return;
  }

  bool pushLeft = GameSceneManager::GetInstance()->IsPushLeft();
  bool pushRight = GameSceneManager::GetInstance()->IsPushRight();

  int inputDir = 0;
  if (pushLeft && !pushRight) {
    inputDir = isInvertedControls_ ? +1 : -1;
  } else if (pushRight && !pushLeft) {
    inputDir = isInvertedControls_ ? -1 : +1;
  }

  if (inputDir == 0) {
    return;
  }

  // 1. レーン移動中（逆方向入力時のクイック切り返しのみ受付）
  if (laneIndex_ != targetLaneIndex_) {
    int currentDir = (targetLaneIndex_ > laneIndex_) ? 1 : -1;
    if (inputDir == -currentDir) {
      int oldTarget = targetLaneIndex_;
      int oldOrigin = laneIndex_;
      targetLaneIndex_ = oldOrigin;
      laneIndex_ = oldTarget;

      startX_ = static_cast<float>(laneIndex_) * laneWidth_;
      lerpTime_ = std::clamp(1.0f - lerpTime_, 0.0f, 1.0f);

      SoundManager::GetInstance()->PlaySE(SoundManager::SE::LaneChange);

      bufferedLaneTimer_ = 0.0f;
      bufferedLaneDirection_ = 0;
    }
    return;
  }

  // 2. 停止中（通常入力によるレーン移動開始）
  bufferedLaneTimer_ = 0.0f;
  bufferedLaneDirection_ = 0;

  int nextTarget = std::clamp(laneIndex_ + inputDir, minLane_, maxLane_);
  if (nextTarget != laneIndex_) {
    targetLaneIndex_ = nextTarget;
    startX_ = transform_.translate.x;
    lerpTime_ = 0.0f;
    SoundManager::GetInstance()->PlaySE(SoundManager::SE::LaneChange);
  }
}

void Player::UpdateLaneMovement(float speedMultiplier) {
  if (laneIndex_ == targetLaneIndex_) {
    return;
  }

  lerpTime_ += laneChangeSpeed_ * speedMultiplier;
  if (lerpTime_ >= 1.0f) {
    lerpTime_ = 1.0f;
    laneIndex_ = targetLaneIndex_;
    transform_.translate.x = static_cast<float>(laneIndex_) * laneWidth_;
    currentRecoveryTimer_ = laneChangeRecovery_;

    bufferedLaneTimer_ = 0.0f;
    bufferedLaneDirection_ = 0;
  } else {
    float targetX = static_cast<float>(targetLaneIndex_) * laneWidth_;
    transform_.translate.x = Lerp(startX_, targetX, lerpTime_);
  }
  MarkDirty();
}

void Player::Update(Matrix4x4 view, float speedMultiplier) {
  currentSpeedMultiplier_ = speedMultiplier;

  // ゲーム一時停止中または描画専用パス（speedMultiplier <= 0.0f）では論理更新を行わない
  if (speedMultiplier > 0.0f) {
    // 先行入力バッファの更新
    UpdateInputBuffer(speedMultiplier);

    // 硬直タイマーの更新
    if (currentRecoveryTimer_ > 0.0f) {
      currentRecoveryTimer_ -= speedMultiplier;
      if (currentRecoveryTimer_ < 0.0f) {
        currentRecoveryTimer_ = 0.0f;
      }
    }

    // ステート更新（遷移と即時更新を安全にステップ実行）
    StepActionState(speedMultiplier);
    // ステート内でのTransform更新を子オブジェクト（HexBarrier等）へDirty伝播
    MarkDirty();
  }

  // モデルの配置更新
  UpdateDrawTransform(speedMultiplier);

  // アニメーション更新
  if (speedMultiplier > 0.0f) {
    model_->UpdateWithDelta(view, speedMultiplier * kAnimationSpeedFactor);
  } else {
    model_->UpdateWithDelta(view, 0.0f);
  }

  model_->SettingWvp(view);

  // 雪玉モデルのTransform設定とWVP・コライダー半径更新
  if (snowballModel_) {
    snowballModel_->SetTransform(snowballTransform_);
    snowballModel_->SettingWvp(view);
    if (auto sphereCol = snowballModel_->GetComponent<SphereColliderComponent>()) {
      sphereCol->SetRadius((std::max)(snowballScale_ * 1.15f, 1.2f));
    }
  }

  // コライダーサイズ設定
  if (auto collider = GetComponent<ColliderComponent>()) {
    collider->SetSize({kDefaultColliderWidth,
                       GetIsRolling() ? kDefaultColliderHeightRoll
                                      : kDefaultColliderHeightStand,
                       kDefaultColliderDepth});
  }

  GameObject::Update(view, speedMultiplier);
}

void Player::Draw(class Draw &draw) {
  if (!isVisible_) {
    return;
  }
  // 雪玉に完全に包まれる前はペンギンモデルを描画
  if (!isSnowballHidingPenguin_) {
    draw.DrawAnimation(model_.get());
  }
  // 雪玉がアクティブな場合は雪玉を描画
  if (GetIsTrip() && isSnowballActive_ && snowballModel_ &&
      snowballScale_ > 0.01f) {
    draw.DrawModel(snowballModel_.get());
  }
  GameObject::Draw(draw);
}

void Player::ImGuiInnerComponents() {
#ifdef _USE_IMGUI
  if (model_) {
    model_->ImGui(false);
  }
  ImGui::Separator();
  ImGui::Text("Penguin Model Settings");
  ImGui::DragFloat3("Model Scale", &modelOffset_.scale.x, 0.5f, 10.0f, 500.0f);
  ImGui::DragFloat3("Model Rotate", &modelOffset_.rotate.x, 0.02f, -3.14f,
                    3.14f);
  ImGui::DragFloat3("Model Offset", &modelOffset_.translate.x, 0.02f, -5.0f,
                    5.0f);

  ImGui::Separator();
  const char *stateNames[] = {"Run", "Jump", "Roll", "Hit", "ForcedCenter"};
  ImGui::Text("Player State: %s",
              stateNames[static_cast<int>(GetActionType())]);
  ImGui::Text("Rolling: %s", GetIsRolling() ? "YES" : "NO");
  ImGui::Text("Jumping: %s", GetIsJumping() ? "YES" : "NO");
  ImGui::Text("Can Act: %s", CanAct() ? "YES" : "NO");
  ImGui::Text("Current Recovery: %.2f", currentRecoveryTimer_);

  ImGui::Separator();
  ImGui::Text("Player Movement Parameters");
  ImGui::SliderFloat("Jump Power", &jumpPower_, 0.10f, 0.40f, "%.3f");
  ImGui::SliderFloat("Gravity", &gravity_, 0.005f, 0.040f, "%.4f");
  ImGui::SliderFloat("Lane Change Speed", &laneChangeSpeed_, 0.05f, 0.50f,
                     "%.2f");
  ImGui::SliderFloat("Roll Duration", &rollDuration_, 10.0f, 60.0f, "%.0f f");
  ImGui::SliderFloat("Roll Transition", &rollTransitionDuration_, 0.01f, 0.50f,
                     "%.2f s");
  ImGui::SliderFloat("Walk Transition", &walkTransitionDuration_, 0.01f, 0.50f,
                     "%.2f s");

  float estAirFrames =
      gravity_ > 0.0f ? ((2.0f * jumpPower_ / gravity_) + 1.0f) : 0.0f;
  float estMaxHeight =
      gravity_ > 0.0f ? ((jumpPower_ * jumpPower_) / (2.0f * gravity_)) : 0.0f;
  ImGui::Text("Est. Jump Air Time: %.0f frames (%.2f s)", estAirFrames,
              estAirFrames / 60.0f);
  ImGui::Text("Est. Max Jump Height: +%.2f m", estMaxHeight);
  ImGui::Text("Lane Move Frames: %.0f frames",
              laneChangeSpeed_ > 0.0f ? (1.0f / laneChangeSpeed_) : 0.0f);
  ImGui::Text("Roll Duration: %.0f frames (%.2f s)", rollDuration_,
              rollDuration_ / 60.0f);
  ImGui::Separator();
  ImGui::Text("Snowball Hit Settings");
  ImGui::Text("Snowball Active: %s", isSnowballActive_ ? "YES" : "NO");
  ImGui::Text("Snowball Scale: %.2f", snowballScale_);
  if (ImGui::Button("Test Trip Snowball Hit")) {
    OnHit(true, false);
  }
#endif
}

void Player::OnHit(bool isTrip, bool isLargeObstacle,
                   const Vector3 &cameraPos) {
  ChangeActionState(
      std::make_unique<PlayerHitState>(isTrip, isLargeObstacle, cameraPos));
}

void Player::StartForceToCenter(float duration) {
  if (GetIsForcedCentering() || targetLaneIndex_ == 0) {
    return;
  }
  ChangeActionState(std::make_unique<PlayerForcedCenterState>(duration));
}

void Player::SetLighting(bool enabled) {
  lightingEnabled_ = enabled;
  if (model_) {
    model_->SetLighting(enabled);
  }
  if (snowballModel_) {
    snowballModel_->SetLighting(enabled);
  }
}

void Player::SetHasBarrier(bool hasBarrier) {
  hasBarrier_ = hasBarrier;
  if (model_) {
    if (hasBarrier_) {
      model_->SetColor(kBarrierColor);
    } else {
      model_->SetColor(kDefaultPlayerColor);
    }
  }
}

bool Player::TriggerJump() {
  if (CanAct() && !(GetIsRolling() && keepRolling_)) {
    ChangeActionState(PlayerActionType::Jump);
    StepActionState(currentSpeedMultiplier_ > 0.0f ? currentSpeedMultiplier_
                                                   : 1.0f);
    return true;
  }
  return false;
}

bool Player::TriggerRoll() {
  if (CanAct() && !GetIsRolling() && !GetIsJumping()) {
    ChangeActionState(PlayerActionType::Roll);
    StepActionState(currentSpeedMultiplier_ > 0.0f ? currentSpeedMultiplier_
                                                   : 1.0f);
    return true;
  }
  return false;
}

bool Player::TriggerMoveLeft() {
  if (!CanAct() || GetIsForcedCentering()) {
    return false;
  }
  // 右移動中ならクイック切り返し
  if (laneIndex_ != targetLaneIndex_) {
    if (targetLaneIndex_ > laneIndex_) {
      int oldTarget = targetLaneIndex_;
      int oldOrigin = laneIndex_;
      targetLaneIndex_ = oldOrigin;
      laneIndex_ = oldTarget;
      startX_ = static_cast<float>(laneIndex_) * laneWidth_;
      lerpTime_ = std::clamp(1.0f - lerpTime_, 0.0f, 1.0f);
      SoundManager::GetInstance()->PlaySE(SoundManager::SE::LaneChange);
      if (GetIsRolling() && !keepRolling_) {
        ChangeActionState(PlayerActionType::Run);
      }
      return true;
    }
    return false;
  }

  int nextLane = laneIndex_ - 1;
  if (nextLane >= minLane_) {
    targetLaneIndex_ = nextLane;
    startX_ = transform_.translate.x;
    lerpTime_ = 0.0f;
    SoundManager::GetInstance()->PlaySE(SoundManager::SE::LaneChange);

    if (GetIsRolling() && !keepRolling_) {
      ChangeActionState(PlayerActionType::Run);
    }
    return true;
  }
  return false;
}

bool Player::TriggerMoveRight() {
  if (!CanAct() || GetIsForcedCentering()) {
    return false;
  }
  // 左移動中ならクイック切り返し
  if (laneIndex_ != targetLaneIndex_) {
    if (targetLaneIndex_ < laneIndex_) {
      int oldTarget = targetLaneIndex_;
      int oldOrigin = laneIndex_;
      targetLaneIndex_ = oldOrigin;
      laneIndex_ = oldTarget;
      startX_ = static_cast<float>(laneIndex_) * laneWidth_;
      lerpTime_ = std::clamp(1.0f - lerpTime_, 0.0f, 1.0f);
      SoundManager::GetInstance()->PlaySE(SoundManager::SE::LaneChange);
      if (GetIsRolling() && !keepRolling_) {
        ChangeActionState(PlayerActionType::Run);
      }
      return true;
    }
    return false;
  }

  int nextLane = laneIndex_ + 1;
  if (nextLane <= maxLane_) {
    targetLaneIndex_ = nextLane;
    startX_ = transform_.translate.x;
    lerpTime_ = 0.0f;
    SoundManager::GetInstance()->PlaySE(SoundManager::SE::LaneChange);

    if (GetIsRolling() && !keepRolling_) {
      ChangeActionState(PlayerActionType::Run);
    }
    return true;
  }
  return false;
}

AABB Player::GetColliderAABB() const {
  float height =
      GetIsRolling() ? kDefaultColliderHeightRoll : kDefaultColliderHeightStand;
  return Collision::MakeAABB(transform_, kDefaultColliderWidth, height,
                             kDefaultColliderDepth);
}

AABB Player::GetStandingColliderAABB() const {
  return Collision::MakeAABB(transform_, kDefaultColliderWidth,
                             kDefaultColliderHeightStand,
                             kDefaultColliderDepth);
}

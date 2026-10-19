#include "Player.h"
#include "Collision.h"
#include "../GameSceneManager.h"
#include "../System/SoundManager.h"
#include "AssetManager.h"
#include "Graphics/Render/Draw.h"

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
  drawTransform.translate.x += modelOffset_.translate.x;
  drawTransform.translate.y +=
      modelOffset_.translate.y + (GetIsRolling() ? kDefaultRollingHeightOffset : 0.0f);
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

  // 手のボーンを登録
  model_->SetBoneMapping(BoneType::RightHand, "mixamorig:RightHand");
  model_->SetBoneMapping(BoneType::LeftHand, "mixamorig:LeftHand");

  // Axeの初期化
  ModelData axeData = AssimpLoadObjFile("Resources/Model/Axe", "Axe.obj");
  axe_->Initialize(axeData);
  axe_->SetName("Player Axe Model");
  axeOffset_.scale = kDefaultAxeScale;
  axeOffset_.rotate = {0.0f, 0.0f, 0.0f};
  axeOffset_.translate = {0.0f, 0.0f, 0.0f};

  // コライダーの初期化（判定サイズは一切変更しない！）
  auto collider = AddComponent<BoxColliderComponent>();
  collider->SetSize({kDefaultColliderWidth, kDefaultColliderHeightStand, kDefaultColliderDepth});

  UpdateDrawTransform(0.0f);
}

// ==========================================
// Player States Implementation
// ==========================================

// --- PlayerRunState ---
void PlayerRunState::Enter(Player &player) {
  player.GetTransformRef().scale.y = 1.0f;
  player.GetTransformRef().translate.y = player.GetBaseHeight();
  if (player.GetModel()) {
    player.GetModel()->SetAnimation("walk", player.GetWalkTransitionDuration());
  }
}

void PlayerRunState::Update(Player &player, float speedMultiplier) {
  if (speedMultiplier <= 0.0f) {
    return;
  }

  // レーン入力処理
  player.ProcessLaneInput();

  // アクション判定 (先行入力バッファまたはキー押下)
  if (player.CanAct() && !player.GetAutoPilot()) {
    if (player.ConsumeBufferedJump() || GameSceneManager::GetInstance()->IsPushJump()) {
      player.ChangeActionState(PlayerActionType::Jump);
      if (player.GetCurrentActionState()) {
        player.GetCurrentActionState()->Update(player, speedMultiplier);
      }
      return;
    }
    if (player.ConsumeBufferedRoll() || GameSceneManager::GetInstance()->IsPushRoll()) {
      player.ChangeActionState(PlayerActionType::Roll);
      if (player.GetCurrentActionState()) {
        player.GetCurrentActionState()->Update(player, speedMultiplier);
      }
      return;
    }
  }

  // 通常レーン移動更新
  player.UpdateLaneMovement(speedMultiplier);
}

// --- PlayerJumpState ---
void PlayerJumpState::Enter(Player &player) {
  float mult = player.GetCurrentSpeedMultiplier() > 0.0f ? player.GetCurrentSpeedMultiplier() : 1.0f;
  player.GetVelocityYRef() = player.GetJumpPower() * mult;
  SoundManager::GetInstance()->PlaySE(SoundManager::SE::Jump);
}

void PlayerJumpState::Update(Player &player, float speedMultiplier) {
  if (speedMultiplier <= 0.0f) {
    return;
  }

  // 空中でもレーン移動入力・更新は有効
  player.ProcessLaneInput();
  player.UpdateLaneMovement(speedMultiplier);

  // 垂直ジャンプ物理
  Transform &t = player.GetTransformRef();
  t.translate.y += player.GetVelocityYRef();
  player.GetVelocityYRef() -= player.GetGravity() * (speedMultiplier * speedMultiplier);

  // 地面に着地
  if (t.translate.y <= player.GetBaseHeight()) {
    t.translate.y = player.GetBaseHeight();
    player.GetVelocityYRef() = 0.0f;
    player.GetCurrentRecoveryTimerRef() = player.GetJumpRecovery();
    SoundManager::GetInstance()->PlaySE(SoundManager::SE::Land);

    // 着地時カメラシェイク演出コールバック
    if (player.GetOnLandCallback()) {
      player.GetOnLandCallback()();
    }

    // 先行入力バッファ判定（着地直後の連続ジャンプまたはスライディング）
    if (!player.GetAutoPilot()) {
      if (player.ConsumeBufferedJump()) {
        player.ChangeActionState(PlayerActionType::Jump);
        if (player.GetCurrentActionState()) {
          player.GetCurrentActionState()->Update(player, speedMultiplier);
        }
        return;
      }
      if (player.ConsumeBufferedRoll()) {
        player.ChangeActionState(PlayerActionType::Roll);
        if (player.GetCurrentActionState()) {
          player.GetCurrentActionState()->Update(player, speedMultiplier);
        }
        return;
      }
    }

    player.ChangeActionState(PlayerActionType::Run);
  }
}

// --- PlayerRollState ---
void PlayerRollState::Enter(Player &player) {
  player.GetRollTimerRef() = player.GetRollDuration();
  SoundManager::GetInstance()->PlaySE(SoundManager::SE::Slide);
  if (player.GetModel()) {
    player.GetModel()->SetAnimation("sneakWalk", player.GetRollTransitionDuration());
  }
  player.GetTransformRef().translate.y = player.GetBaseHeight() - Player::kDefaultRollingHeightOffset;
}

void PlayerRollState::Update(Player &player, float speedMultiplier) {
  if (speedMultiplier <= 0.0f) {
    return;
  }

  // 転がり中でもジャンプでキャンセル可能（先行入力バッファまたはキー押下）
  if (player.CanAct() && !player.GetAutoPilot()) {
    if (player.ConsumeBufferedJump() || GameSceneManager::GetInstance()->IsPushJump()) {
      if (!player.GetKeepRolling()) {
        player.ChangeActionState(PlayerActionType::Jump);
        if (player.GetCurrentActionState()) {
          player.GetCurrentActionState()->Update(player, speedMultiplier);
        }
        return;
      }
    }
  }

  // レーン入力処理（横移動が開始されたら転がりを解除）
  int prevTarget = player.GetTargetLaneIndex();
  player.ProcessLaneInput();
  if (player.GetTargetLaneIndex() != prevTarget && !player.GetKeepRolling()) {
    player.ChangeActionState(PlayerActionType::Run);
    player.UpdateLaneMovement(speedMultiplier);
    return;
  }
  player.UpdateLaneMovement(speedMultiplier);

  // 転がりタイマー減算
  player.GetRollTimerRef() -= speedMultiplier;
  if (player.GetRollTimerRef() <= 0.0f && !player.GetKeepRolling()) {
    player.GetCurrentRecoveryTimerRef() = player.GetRollRecovery();

    // ロール終了時の先行入力判定
    if (!player.GetAutoPilot()) {
      if (player.ConsumeBufferedJump()) {
        player.ChangeActionState(PlayerActionType::Jump);
        if (player.GetCurrentActionState()) {
          player.GetCurrentActionState()->Update(player, speedMultiplier);
        }
        return;
      }
      if (player.ConsumeBufferedRoll()) {
        player.ChangeActionState(PlayerActionType::Roll);
        if (player.GetCurrentActionState()) {
          player.GetCurrentActionState()->Update(player, speedMultiplier);
        }
        return;
      }
    }

    player.ChangeActionState(PlayerActionType::Run);
  }
}

void PlayerRollState::Exit(Player &player) {
  player.GetTransformRef().scale.y = 1.0f;
  player.GetTransformRef().translate.y = player.GetBaseHeight();
  if (player.GetModel()) {
    player.GetModel()->SetAnimation("walk", player.GetWalkTransitionDuration());
  }
}

// --- PlayerHitState ---
void PlayerHitState::Enter(Player &player) {
  player.GetTransformRef().scale = {1.0f, 1.0f, 1.0f};
  if (player.GetModel()) {
    player.GetModel()->SetAnimation("walk", player.GetWalkTransitionDuration());
  }
  player.GetCurrentRecoveryTimerRef() = 0.0f;
  player.GetHitTimerRef() = 0.0f;

  float randX = ((float)rand() / RAND_MAX - Player::kKnockbackRandCenterOffset) * Player::kKnockbackRandXScale;
  if (player.GetIsTrip()) {
    player.GetKnockbackVelocityRef() = {randX, Player::kTripKnockbackVelocity.y, Player::kTripKnockbackVelocity.z};
  } else {
    player.GetKnockbackVelocityRef() = {randX, Player::kHitKnockbackVelocity.y, Player::kHitKnockbackVelocity.z};
  }
}

void PlayerHitState::Update(Player &player, float speedMultiplier) {
  if (speedMultiplier == 0.0f) {
    return;
  }

  player.GetHitTimerRef() += 1.0f;
  Transform &transform = player.GetTransformRef();
  Vector3 &knockback = player.GetKnockbackVelocityRef();

  transform.translate.x += knockback.x;
  transform.translate.y += knockback.y;
  transform.translate.z += knockback.z;

  knockback.y -= player.GetGravity() * Player::kHitGravityMultiplier;
  if (player.GetIsTrip()) {
    transform.rotate.x += Player::kTripRotationSpeed;
  } else {
    transform.rotate.x -= Player::kHitRotationSpeed;
  }

  if (transform.translate.y <= player.GetBaseHeight() && knockback.y < 0.0f) {
    transform.translate.y = player.GetBaseHeight();
    knockback.y = 0.0f;
    knockback.x *= Player::kHitLandingFriction;
    knockback.z *= Player::kHitLandingFriction;
  }
}

void PlayerHitState::Exit(Player &player) {
  player.GetTransformRef().rotate.x = 0.0f;
  player.GetKnockbackVelocityRef() = {0.0f, 0.0f, 0.0f};
}

// --- PlayerForcedCenterState ---
void PlayerForcedCenterState::Enter(Player &player) {
  player.GetForcedCenterTimerRef() = 0.0f;
  player.GetForcedCenterStartXRef() = player.GetTransformRef().translate.x;
}

void PlayerForcedCenterState::Update(Player &player, float speedMultiplier) {
  if (speedMultiplier <= 0.0f) {
    return;
  }

  player.GetForcedCenterTimerRef() += speedMultiplier;
  float t = player.GetForcedCenterTimerRef() / player.GetForcedCenterDuration();
  if (t > 1.0f) {
    t = 1.0f;
  }

  float easeT = t * t * (3.0f - 2.0f * t);
  player.GetTransformRef().translate.x = Lerp(player.GetForcedCenterStartXRef(), 0.0f, easeT);

  if (t >= 1.0f) {
    player.SetLaneIndex(0);
    player.SetTargetLaneIndex(0);
    player.ChangeActionState(PlayerActionType::Run);
  }
}

// ==========================================
// Player Implementation
// ==========================================

void Player::Reset() {
  transform_.scale = {1.0f, 1.0f, 1.0f};
  transform_.rotate = {0.0f, 0.0f, 0.0f};
  transform_.translate = {0.0f, baseHeight_, 0.0f};

  laneIndex_ = 0;
  targetLaneIndex_ = 0;
  lerpTime_ = 0.0f;
  startX_ = 0.0f;
  moveDirection_ = MoveDirection::None;

  velocityY_ = 0.0f;
  rollTimer_ = 0.0f;
  keepRolling_ = false;
  currentRecoveryTimer_ = 0.0f;

  isTrip_ = false;
  hitTimer_ = 0.0f;
  knockbackVelocity_ = {0.0f, 0.0f, 0.0f};

  forcedCenterTimer_ = 0.0f;
  forcedCenterDuration_ = kDefaultForcedCenterDuration;
  forcedCenterStartX_ = 0.0f;

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
  switch (nextType) {
  case PlayerActionType::Run:
    ChangeActionState(std::make_unique<PlayerRunState>());
    break;
  case PlayerActionType::Jump:
    ChangeActionState(std::make_unique<PlayerJumpState>());
    break;
  case PlayerActionType::Roll:
    ChangeActionState(std::make_unique<PlayerRollState>());
    break;
  case PlayerActionType::Hit:
    ChangeActionState(std::make_unique<PlayerHitState>());
    break;
  case PlayerActionType::ForcedCenter:
    ChangeActionState(std::make_unique<PlayerForcedCenterState>());
    break;
  }
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

void Player::UpdateInputBuffer(float speedMultiplier) {
  float decay = (speedMultiplier > 0.0f) ? speedMultiplier : 1.0f;
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

  if (GameSceneManager::GetInstance()->IsPushJump()) {
    bufferedJumpTimer_ = kDefaultInputBufferFrames;
  }
  if (GameSceneManager::GetInstance()->IsPushRoll()) {
    bufferedRollTimer_ = kDefaultInputBufferFrames;
  }

  bool pushLeft = GameSceneManager::GetInstance()->IsPushLeft();
  bool pushRight = GameSceneManager::GetInstance()->IsPushRight();
  if (pushLeft || pushRight) {
    int dir = 0;
    if (isInvertedControls_) {
      dir = pushLeft ? +1 : -1;
    } else {
      dir = pushLeft ? -1 : +1;
    }
    bufferedLaneDirection_ = dir;
    bufferedLaneTimer_ = kDefaultInputBufferFrames;
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

  // 1. レーン移動中 (クイック切り返し & 同方向先行入力)
  if (laneIndex_ != targetLaneIndex_) {
    if (inputDir != 0) {
      int currentDir = (targetLaneIndex_ > laneIndex_) ? 1 : -1;
      if (inputDir == -currentDir) {
        // === クイック切り返し (Quick Reverse) ===
        // 移動途中で逆方向の入力があった場合、即座に反転して元のレーンへ戻る
        int oldTarget = targetLaneIndex_;
        int oldOrigin = laneIndex_;
        targetLaneIndex_ = oldOrigin;
        laneIndex_ = oldTarget;

        startX_ = static_cast<float>(laneIndex_) * laneWidth_;
        lerpTime_ = 1.0f - lerpTime_;
        if (lerpTime_ < 0.0f) lerpTime_ = 0.0f;
        if (lerpTime_ > 1.0f) lerpTime_ = 1.0f;

        SoundManager::GetInstance()->PlaySE(SoundManager::SE::LaneChange);

        bufferedLaneTimer_ = 0.0f;
        bufferedLaneDirection_ = 0;
      } else {
        // 同方向の入力は先行入力バッファに保持
        bufferedLaneDirection_ = inputDir;
        bufferedLaneTimer_ = kDefaultInputBufferFrames;
      }
    }
    return;
  }

  // 2. 停止中 (即時入力またはバッファの消費)
  if (inputDir == 0 && bufferedLaneTimer_ > 0.0f && bufferedLaneDirection_ != 0) {
    inputDir = ConsumeBufferedLane();
  } else if (inputDir != 0) {
    bufferedLaneTimer_ = 0.0f;
    bufferedLaneDirection_ = 0;
  }

  if (inputDir != 0) {
    int nextTarget = laneIndex_ + inputDir;
    nextTarget = std::clamp(nextTarget, minLane_, maxLane_);

    if (nextTarget != laneIndex_) {
      targetLaneIndex_ = nextTarget;
      startX_ = transform_.translate.x;
      lerpTime_ = 0.0f;
      SoundManager::GetInstance()->PlaySE(SoundManager::SE::LaneChange);
    }
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

    // 到着直後の先行入力バッファチェック (連打による連続移動)
    if (bufferedLaneTimer_ > 0.0f && bufferedLaneDirection_ != 0 && CanAct() && !isAutoPilot_) {
      int nextTarget = std::clamp(laneIndex_ + bufferedLaneDirection_, minLane_, maxLane_);
      if (nextTarget != laneIndex_) {
        targetLaneIndex_ = nextTarget;
        startX_ = transform_.translate.x;
        lerpTime_ = 0.0f;
        bufferedLaneTimer_ = 0.0f;
        bufferedLaneDirection_ = 0;
        SoundManager::GetInstance()->PlaySE(SoundManager::SE::LaneChange);
      }
    }
  } else {
    float targetX = static_cast<float>(targetLaneIndex_) * laneWidth_;
    transform_.translate.x = Lerp(startX_, targetX, lerpTime_);
  }
}

void Player::Update(Matrix4x4 view, float speedMultiplier) {
  currentSpeedMultiplier_ = speedMultiplier;

  // 先行入力バッファの更新
  UpdateInputBuffer(speedMultiplier);

  // 硬直タイマーの更新
  if (currentRecoveryTimer_ > 0.0f) {
    currentRecoveryTimer_ -= (speedMultiplier > 0.0f ? speedMultiplier : 1.0f);
    if (currentRecoveryTimer_ < 0.0f) {
      currentRecoveryTimer_ = 0.0f;
    }
  }

  // ステート更新
  if (currentState_) {
    if (GetIsHit()) {
      currentState_->Update(*this, speedMultiplier > 0.0f ? 1.0f : 0.0f);
    } else {
      currentState_->Update(*this, speedMultiplier);
    }
  }

  // モデルの配置更新
  UpdateDrawTransform(speedMultiplier);

  // アニメーション更新（走っている際）
  if (speedMultiplier > 0.0f) {
    model_->UpdateWithDelta(view, speedMultiplier * kAnimationSpeedFactor);
  } else {
    model_->UpdateWithDelta(view, 0.0f);
  }

  model_->SettingWvp(view);

  // Axeのアタッチ処理
  if (isDrawAxe_) {
    Transform handTransform = model_->GetBoneTransform(BoneType::RightHand);
    Matrix4x4 boneMatrix = MakeAffineMatrix(
        handTransform.translate, handTransform.scale, handTransform.rotate);
    Matrix4x4 offsetMatrix = MakeAffineMatrix(
        axeOffset_.translate, axeOffset_.scale, axeOffset_.rotate);
    Matrix4x4 finalMatrix = MultiplyMatrix4x4(offsetMatrix, boneMatrix);
    axe_->SetTransform(DecomposeMatrix(finalMatrix));
    axe_->SettingWvp(view);
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
  draw.DrawAnimation(model_.get());
  if (isDrawAxe_) {
    draw.DrawModel(axe_.get());
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
  ImGui::Checkbox("Draw Axe", &isDrawAxe_);

  ImGui::Separator();
  const char *stateNames[] = {"Run", "Jump", "Roll", "Hit", "ForcedCenter"};
  ImGui::Text("Player State: %s", stateNames[static_cast<int>(GetActionType())]);
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
#endif
}

void Player::OnHit(bool isTrip) {
  isTrip_ = isTrip;
  ChangeActionState(PlayerActionType::Hit);
}

bool Player::IsHitAnimationFinished() const {
  return GetIsHit() && hitTimer_ >= hitDuration_;
}

void Player::StartForceToCenter(float duration) {
  if (GetIsForcedCentering() || targetLaneIndex_ == 0) {
    return;
  }
  forcedCenterDuration_ = duration;
  ChangeActionState(PlayerActionType::ForcedCenter);
}

void Player::SetHasBarrier(bool hasBarrier) {
  hasBarrier_ = hasBarrier;
  if (model_ && model_->GetMaterial()) {
    if (hasBarrier_) {
      model_->GetMaterial()->SetColor(kBarrierColor); // シアン
    } else {
      model_->GetMaterial()->SetColor(kDefaultPlayerColor); // 白（デフォルト）
    }
  }
}

bool Player::TriggerJump() {
  if (CanAct() && !(GetIsRolling() && keepRolling_)) {
    ChangeActionState(PlayerActionType::Jump);
    if (currentState_) {
      currentState_->Update(*this, currentSpeedMultiplier_ > 0.0f ? currentSpeedMultiplier_ : 1.0f);
    }
    return true;
  }
  return false;
}

bool Player::TriggerRoll() {
  if (CanAct() && !GetIsRolling() && !GetIsJumping()) {
    ChangeActionState(PlayerActionType::Roll);
    if (currentState_) {
      currentState_->Update(*this, currentSpeedMultiplier_ > 0.0f ? currentSpeedMultiplier_ : 1.0f);
    }
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
      lerpTime_ = 1.0f - lerpTime_;
      if (lerpTime_ < 0.0f) lerpTime_ = 0.0f;
      if (lerpTime_ > 1.0f) lerpTime_ = 1.0f;
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
      lerpTime_ = 1.0f - lerpTime_;
      if (lerpTime_ < 0.0f) lerpTime_ = 0.0f;
      if (lerpTime_ > 1.0f) lerpTime_ = 1.0f;
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
  float height = GetIsRolling() ? kDefaultColliderHeightRoll : kDefaultColliderHeightStand;
  return Collision::MakeAABB(transform_, kDefaultColliderWidth, height, kDefaultColliderDepth);
}

AABB Player::GetStandingColliderAABB() const {
  return Collision::MakeAABB(transform_, kDefaultColliderWidth, kDefaultColliderHeightStand, kDefaultColliderDepth);
}


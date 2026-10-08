#include "Player.h"
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
      modelOffset_.translate.y + (isRolling_ ? kDefaultRollingHeightOffset : 0.0f);
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
  model_->name_ = "Player Penguin Animation Model";
  model_->SetVisibleBones(false);

  // 手のボーンを登録
  model_->SetBoneMapping(BoneType::RightHand, "mixamorig:RightHand");
  model_->SetBoneMapping(BoneType::LeftHand, "mixamorig:LeftHand");

  // Axeの初期化
  ModelData axeData = AssimpLoadObjFile("Resources/Model/Axe", "Axe.obj");
  axe_->Initialize(axeData);
  axe_->name_ = "Player Axe Model";
  axeOffset_.scale = kDefaultAxeScale;
  axeOffset_.rotate = {0.0f, 0.0f, 0.0f};
  axeOffset_.translate = {0.0f, 0.0f, 0.0f};

  // コライダーの初期化（判定サイズは一切変更しない！）
  auto collider = AddComponent<ColliderComponent>();
  collider->SetShape(ColliderShape::Box);
  collider->SetSize({kDefaultColliderWidth, kDefaultColliderHeightStand, kDefaultColliderDepth});

  UpdateDrawTransform(0.0f);
}

void Player::Reset() {
  transform_.scale = {1.0f, 1.0f, 1.0f};
  transform_.rotate = {0.0f, 0.0f, 0.0f};
  transform_.translate = {0.0f, baseHeight_, 0.0f};

  laneIndex_ = 0;
  targetLaneIndex_ = 0;
  lerpTime_ = 0.0f;
  startX_ = 0.0f;
  moveDirection_ = MoveDirection::None;

  isJumping_ = false;
  velocityY_ = 0.0f;
  isRolling_ = false;
  rollTimer_ = 0.0f;
  keepRolling_ = false;
  currentRecoveryTimer_ = 0.0f;

  isHit_ = false;
  isTrip_ = false;
  hitTimer_ = 0.0f;
  knockbackVelocity_ = {0.0f, 0.0f, 0.0f};

  isForcedCentering_ = false;
  forcedCenterTimer_ = 0.0f;
  forcedCenterDuration_ = kDefaultForcedCenterDuration;
  forcedCenterStartX_ = 0.0f;

  SetHasBarrier(false);
  isInvertedControls_ = false;
  isAutoPilot_ = false;
  isVisible_ = true;

  model_->SetAnimation("walk", 0.0f);

  UpdateDrawTransform(0.0f);
}

void Player::Update(Matrix4x4 view, float speedMultiplier) {
  if (isHit_) {
    // SpeedMultiplier is ignored for hit update so animation plays consistently
    // even if the game scroll stops.
    if (speedMultiplier > 0.0f) {
      HitUpdate(1.0f);
    } else {
      HitUpdate(0.0f);
    }
  } else {
    PlayerMove(speedMultiplier);
  }

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

  if (auto collider = GetComponent<ColliderComponent>()) {
    collider->SetSize({kDefaultColliderWidth,
                       isRolling_ ? kDefaultColliderHeightRoll
                                  : kDefaultColliderHeightStand,
                       kDefaultColliderDepth});
  }
  GameObject::Update(view, speedMultiplier);
}

void Player::PlayerMove(float speedMultiplier) {
  if (speedMultiplier <= 0.0f) {
    UpdateDrawTransform(0.0f);
    return;
  }

  // 硬直タイマーの更新
  if (currentRecoveryTimer_ > 0.0f) {
    currentRecoveryTimer_ -= speedMultiplier;
    if (currentRecoveryTimer_ < 0.0f) {
      currentRecoveryTimer_ = 0.0f;
    }
  }

  bool canAct = (currentRecoveryTimer_ <= 0.0f);

  // 強制中央移動の処理
  if (isForcedCentering_) {
    forcedCenterTimer_ += speedMultiplier;
    float t = forcedCenterTimer_ / forcedCenterDuration_;
    if (t > 1.0f) {
      t = 1.0f;
    }

    // スムーズな補間（EaseInOut）
    float easeT = t * t * (3.0f - 2.0f * t);
    transform_.translate.x = Lerp(forcedCenterStartX_, 0.0f, easeT);

    if (t >= 1.0f) {
      isForcedCentering_ = false;
      laneIndex_ = 0; // 中央レーンに確定
      targetLaneIndex_ = 0;
    }
  }
  // レーンの移動中ではなかったら（強制移動中でない時のみ入力受付）
  else if (laneIndex_ == targetLaneIndex_) {
    // キー入力で目標レーンを設定
    if (canAct && !isAutoPilot_) {
      bool pushLeft = GameSceneManager::GetInstance()->IsPushLeft();
      bool pushRight = GameSceneManager::GetInstance()->IsPushRight();
      if (isInvertedControls_) {
        // ボス戦等でカメラが正面（180度反転）を向いている場合、
        // 画面の見た目通りに動くように操作を反転（左入力で画面左/ワールド+Xへ、右入力で画面右/ワールド-Xへ）
        if (pushLeft) {
          targetLaneIndex_ = laneIndex_ + 1;
        }
        if (pushRight) {
          targetLaneIndex_ = laneIndex_ - 1;
        }
      } else {
        if (pushLeft) {
          targetLaneIndex_ = laneIndex_ - 1;
        }
        if (pushRight) {
          targetLaneIndex_ = laneIndex_ + 1;
        }
      }
    }

    // レーンの範囲制限
    if (targetLaneIndex_ < minLane_)
      targetLaneIndex_ = minLane_;
    if (targetLaneIndex_ > maxLane_)
      targetLaneIndex_ = maxLane_;

    // 移動が開始される場合、初期値を保存
    if (targetLaneIndex_ != laneIndex_) {
      startX_ = transform_.translate.x;
      lerpTime_ = 0.0f;
      SoundManager::GetInstance()->PlaySE(SoundManager::SE::LaneChange);

      // 横移動時にしゃがみ（転がり）をキャンセルして硬直をなくす
      if (isRolling_ && !keepRolling_) {
        isRolling_ = false;
        transform_.scale.y = 1.0f;
        transform_.translate.y = baseHeight_;
        model_->SetAnimation("walk", walkTransitionDuration_);
      }
    }
  }
  // レーンの移動中だったら
  else {
    // 線形補間で移動
    lerpTime_ += laneChangeSpeed_ * speedMultiplier;
    if (lerpTime_ > 1.0f) {
      lerpTime_ = 1.0f;
    }

    float targetX = static_cast<float>(targetLaneIndex_) * laneWidth_;
    transform_.translate.x = Lerp(startX_, targetX, lerpTime_);

    // 移動が完了したら現在のレーンを更新
    if (lerpTime_ >= 1.0f) {
      laneIndex_ = targetLaneIndex_;
      currentRecoveryTimer_ = laneChangeRecovery_;
    }
  }

  // === アクション（ジャンプと転がり） ===
  // 地上にいてジャンプ中でなければアクション可能（転がり中でもジャンプでキャンセル可能）
  if (!isJumping_ && canAct && !isAutoPilot_) {
    if (GameSceneManager::GetInstance()->IsPushJump()) {
      if (!(isRolling_ && keepRolling_)) {
        isJumping_ = true;
        velocityY_ = jumpPower_ * speedMultiplier;
        SoundManager::GetInstance()->PlaySE(SoundManager::SE::Jump);

        // ジャンプ時にしゃがみをキャンセル
        if (isRolling_) {
          isRolling_ = false;
          transform_.scale.y = 1.0f;
          transform_.translate.y = baseHeight_;
          model_->SetAnimation("walk", walkTransitionDuration_);
        }
      }
    } else if (!isRolling_ && GameSceneManager::GetInstance()->IsPushRoll()) {
      isRolling_ = true;
      rollTimer_ = rollDuration_;
      SoundManager::GetInstance()->PlaySE(SoundManager::SE::Slide);
      model_->SetAnimation("sneakWalk", rollTransitionDuration_);
      // 重心が変わる分、Y座標を少し下げる（原点が中心の場合）
      transform_.translate.y = baseHeight_ - 0.5f;
    }
  }

  // ジャンプ処理
  if (isJumping_) {
    transform_.translate.y += velocityY_;
    velocityY_ -= gravity_ * (speedMultiplier * speedMultiplier);

    // 地面に着地
    if (transform_.translate.y <= baseHeight_) {
      transform_.translate.y = baseHeight_;
      isJumping_ = false;
      velocityY_ = 0.0f;
      currentRecoveryTimer_ = jumpRecovery_;
      SoundManager::GetInstance()->PlaySE(SoundManager::SE::Land);
    }
  }

  // 転がり処理
  if (isRolling_) {
    rollTimer_ -= speedMultiplier;
    if (rollTimer_ <= 0.0f && !keepRolling_) {
      isRolling_ = false;
      // 姿勢を元に戻す
      transform_.scale.y = 1.0f;
      transform_.translate.y = baseHeight_;
      model_->SetAnimation("walk", walkTransitionDuration_);
      currentRecoveryTimer_ = rollRecovery_;
    }
  }

  // トランスフォームをモデルに適用
  UpdateDrawTransform(speedMultiplier);
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

void Player::HitUpdate(float speedMultiplier) {
  // ノックバック処理
  if (isHit_) {
    if (speedMultiplier == 0.0f) {
      UpdateDrawTransform(0.0f);
      return;
    }

    hitTimer_ += 1.0f;

    transform_.translate.x += knockbackVelocity_.x;
    transform_.translate.y += knockbackVelocity_.y;
    transform_.translate.z += knockbackVelocity_.z;

    // 重力と回転（後ろに飛ぶか前に転がるか）
    knockbackVelocity_.y -= gravity_ * 2.0f;
    if (isTrip_) {
      transform_.rotate.x += 0.2f; // 前に転がる回転
    } else {
      transform_.rotate.x -= 0.1f; // 後ろに飛ぶ回転
    }

    // 地面に着地したらバウンドなどを抑える
    if (transform_.translate.y <= baseHeight_ && knockbackVelocity_.y < 0.0f) {
      transform_.translate.y = baseHeight_;
      knockbackVelocity_.y = 0.0f;
      knockbackVelocity_.x *= 0.8f;
      knockbackVelocity_.z *= 0.8f;
    }

    UpdateDrawTransform(0.0f);
  }
}

void Player::OnHit(bool isTrip) {
  isHit_ = true;
  isTrip_ = isTrip;
  hitTimer_ = 0.0f;

  // 姿勢をリセット
  isRolling_ = false;
  isJumping_ = false;
  transform_.scale = {1.0f, 1.0f, 1.0f};
  model_->SetAnimation("walk", walkTransitionDuration_);
  currentRecoveryTimer_ = 0.0f;

  float randX = ((float)rand() / RAND_MAX - 0.5f) * 0.1f;

  if (isTrip_) {
    // Lowに当たってつまずいた場合、少し前に転がるようなノックバック
    knockbackVelocity_ = {randX, 0.3f, 0.8f};
  } else {
    // 少し後ろと上に飛ぶノックバック
    knockbackVelocity_ = {randX, 0.4f, -0.6f};
  }
}

bool Player::IsHitAnimationFinished() const {
  return isHit_ && hitTimer_ >= hitDuration_;
}

void Player::StartForceToCenter(float duration) {
  if (isForcedCentering_ || targetLaneIndex_ == 0) {
    return; // 既に移動中、または既に中央目標の場合は何もしない
  }
  isForcedCentering_ = true;
  forcedCenterTimer_ = 0.0f;
  forcedCenterDuration_ = duration;
  forcedCenterStartX_ = transform_.translate.x;

  // 転がり中なら解除する（安全のため）
  if (isRolling_ && !keepRolling_) {
    isRolling_ = false;
    transform_.scale.y = 1.0f;
    transform_.translate.y = baseHeight_;
    model_->SetAnimation("walk", walkTransitionDuration_);
  }
}

void Player::SetHasBarrier(bool hasBarrier) {
  hasBarrier_ = hasBarrier;
  if (model_ && model_->GetMaterial()) {
    if (hasBarrier_) {
      model_->GetMaterial()->SetColor({0.0f, 1.0f, 1.0f, 1.0f}); // シアン
    } else {
      model_->GetMaterial()->SetColor(
          {1.0f, 1.0f, 1.0f, 1.0f}); // 白（デフォルト）
    }
  }
}

bool Player::TriggerJump() {
  if (!isJumping_ && (currentRecoveryTimer_ <= 0.0f)) {
    if (!(isRolling_ && keepRolling_)) {
      isJumping_ = true;
      velocityY_ = jumpPower_;
      SoundManager::GetInstance()->PlaySE(SoundManager::SE::Jump);

      if (isRolling_) {
        isRolling_ = false;
        transform_.scale.y = 1.0f;
        transform_.translate.y = baseHeight_;
        model_->SetAnimation("walk", walkTransitionDuration_);
      }
      return true;
    }
  }
  return false;
}

bool Player::TriggerRoll() {
  if (!isJumping_ && !isRolling_ && (currentRecoveryTimer_ <= 0.0f)) {
    isRolling_ = true;
    rollTimer_ = rollDuration_;
    SoundManager::GetInstance()->PlaySE(SoundManager::SE::Slide);
    model_->SetAnimation("sneakWalk", rollTransitionDuration_);
    transform_.translate.y = baseHeight_ - 0.5f;
    return true;
  }
  return false;
}

bool Player::TriggerMoveLeft() {
  if (laneIndex_ == targetLaneIndex_ && (currentRecoveryTimer_ <= 0.0f) && !isForcedCentering_) {
    int nextLane = laneIndex_ - 1;
    if (nextLane >= minLane_) {
      targetLaneIndex_ = nextLane;
      startX_ = transform_.translate.x;
      lerpTime_ = 0.0f;
      SoundManager::GetInstance()->PlaySE(SoundManager::SE::LaneChange);

      if (isRolling_ && !keepRolling_) {
        isRolling_ = false;
        transform_.scale.y = 1.0f;
        transform_.translate.y = baseHeight_;
        model_->SetAnimation("walk", walkTransitionDuration_);
      }
      return true;
    }
  }
  return false;
}

bool Player::TriggerMoveRight() {
  if (laneIndex_ == targetLaneIndex_ && (currentRecoveryTimer_ <= 0.0f) && !isForcedCentering_) {
    int nextLane = laneIndex_ + 1;
    if (nextLane <= maxLane_) {
      targetLaneIndex_ = nextLane;
      startX_ = transform_.translate.x;
      lerpTime_ = 0.0f;
      SoundManager::GetInstance()->PlaySE(SoundManager::SE::LaneChange);

      if (isRolling_ && !keepRolling_) {
        isRolling_ = false;
        transform_.scale.y = 1.0f;
        transform_.translate.y = baseHeight_;
        model_->SetAnimation("walk", walkTransitionDuration_);
      }
      return true;
    }
  }
  return false;
}


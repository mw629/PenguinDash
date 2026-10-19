#include "GameCameraController.h"
#include <Entity/Player.h>
#include <Stage/StageSettings.h>
#include <cmath>
#include <imgui.h>

void GameCameraController::Initialize() {
  camera_->SetDebugName("GameScene MainCamera");
  camera_->SetTransform(cameraTransform_);
  camera_->Update();

  gameCamera_->SetDebugName("GameScene GameCamera");
  gameCamera_->SetDebugCamera(false);
  gameCamera_->SetTransform(cameraTransform_);
  gameCamera_->Update();

  viewMatrix_ = camera_->GetViewMatrix();
}

void GameCameraController::Reset(const Transform &defaultTransform) {
  isCameraTransitionPending_ = false;
  isCameraTransitioning_ = false;
  cameraTransitionTimer_ = 0.0f;

  cameraTransform_ = defaultTransform;
  camera_->SetTransform(cameraTransform_);
  gameCamera_->SetTransform(cameraTransform_);
}

void GameCameraController::Update() {
  camera_->Update();
  gameCamera_->Update();
  viewMatrix_ = camera_->GetViewMatrix();
}

void GameCameraController::SetTransform(const Transform &transform) {
  cameraTransform_ = transform;
  camera_->SetTransform(cameraTransform_);
  gameCamera_->SetTransform(cameraTransform_);
}

void GameCameraController::SetDebugCamera(bool debug) {
  camera_->SetDebugCamera(debug);
}

void GameCameraController::StartTransition(const Transform &targetTransform,
                                           int laneCount,
                                           StageSettings *stageSettings) {
  if (stageSettings) {
    stageSettings->SetSpawningPaused(true);
    stageSettings->SetLaneCount(laneCount);
  }
  isCameraTransitionPending_ = true;
  pendingCameraTargetTransform_ = targetTransform;
  pendingLaneCount_ = laneCount;
}

void GameCameraController::UpdateTransition(
    StageSettings *stageSettings, std::function<void()> onFinished) {
  if (isCameraTransitionPending_ && stageSettings) {
    bool hasActiveObstacles = false;
    for (int i = 0; i < stageSettings->GetMaxObstacles(); i++) {
      Obstacle *obstacle = stageSettings->GetObstacle(i);
      if (obstacle && obstacle->GetIsActive() && !obstacle->GetIsHit()) {
        hasActiveObstacles = true;
        break;
      }
    }

    if (!hasActiveObstacles) {
      isCameraTransitionPending_ = false;
      isCameraTransitioning_ = true;
      cameraTransitionTimer_ = 0.0f;
      startCameraTransform_ = cameraTransform_;
      targetCameraTransform_ = pendingCameraTargetTransform_;
      stageSettings->SetLaneCount(pendingLaneCount_);
    }
  }

  if (!isCameraTransitioning_)
    return;

  cameraTransitionTimer_ += Engine::kDefaultDeltaTime;
  float t = cameraTransitionTimer_ / cameraTransitionDuration_;

  if (t >= 1.0f) {
    t = 1.0f;
    isCameraTransitioning_ = false;
    if (onFinished) {
      onFinished();
    }
  }

  float easeT = t * t * (3.0f - 2.0f * t);

  cameraTransform_.translate = Lerp(startCameraTransform_.translate,
                                    targetCameraTransform_.translate, easeT);
  cameraTransform_.translate.y += std::sin(easeT * kPi) * kTransitionArcHeight;

  cameraTransform_.rotate =
      Lerp(startCameraTransform_.rotate, targetCameraTransform_.rotate, easeT);
  cameraTransform_.scale =
      Lerp(startCameraTransform_.scale, targetCameraTransform_.scale, easeT);

  camera_->SetTransform(cameraTransform_);
  gameCamera_->SetTransform(cameraTransform_);
}

void GameCameraController::UpdateFirstPersonCamera(Player *player) {
  if (!player) return;

  Vector3 playerPos = player->GetTransform().translate;
  Vector3 playerRot = player->GetTransform().rotate;

  Transform fpTransform;
  fpTransform.scale = {1.0f, 1.0f, 1.0f};
  fpTransform.rotate = {
      playerRot.x + firstPersonRotate_.x,
      playerRot.y + firstPersonRotate_.y,
      playerRot.z + firstPersonRotate_.z,
  };
  fpTransform.translate = {
      playerPos.x + firstPersonOffset_.x,
      playerPos.y + firstPersonOffset_.y,
      playerPos.z + firstPersonOffset_.z,
  };

  camera_->SetTransform(fpTransform);
  gameCamera_->SetTransform(fpTransform);

  player->SetVisible(drawPlayerInFirstPerson_);
}

void GameCameraController::ToggleFirstPerson(Player *player) {
  isFirstPersonView_ = !isFirstPersonView_;
  if (!isFirstPersonView_) {
    camera_->SetTransform(cameraTransform_);
    gameCamera_->SetTransform(cameraTransform_);
    if (player) {
      player->SetVisible(true);
    }
  }
}

void GameCameraController::UpdateGameplayCamera(Player *player, float dt) {
  if (!player || isFirstPersonView_ || isCameraTransitioning_ || isCameraTransitionPending_) {
    return;
  }

  if (isFollowPlayer_) {
    Vector3 playerPos = player->GetTransform().translate;
    float targetOffsetX = playerPos.x * followWeightX_;
    float targetOffsetY = (playerPos.y > 0.0f ? playerPos.y : 0.0f) * followWeightY_;

    float t = std::clamp(followSmoothSpeed_ * dt, 0.0f, 1.0f);
    currentFollowOffset_.x = Lerp(currentFollowOffset_.x, targetOffsetX, t);
    currentFollowOffset_.y = Lerp(currentFollowOffset_.y, targetOffsetY, t);
    currentBankRoll_ = 0.0f;

    Transform followTransform = cameraTransform_;
    followTransform.translate.x += currentFollowOffset_.x;
    followTransform.translate.y += currentFollowOffset_.y;

    camera_->SetTransform(followTransform);
    gameCamera_->SetTransform(followTransform);
  } else {
    currentFollowOffset_ = {0.0f, 0.0f, 0.0f};
    currentBankRoll_ = 0.0f;
    camera_->SetTransform(cameraTransform_);
    gameCamera_->SetTransform(cameraTransform_);
  }
}

// =================================================================
// カメラ演出の同期委譲 (camera_ と gameCamera_ の両方に適用)
// =================================================================
void GameCameraController::Shake(float intensity, float duration, float frequency) {
  if (camera_) camera_->Shake(intensity, duration, frequency);
  if (gameCamera_) gameCamera_->Shake(intensity, duration, frequency);
}

void GameCameraController::ShakeDirectional(const Vector3 &direction, float intensity, float duration, float frequency) {
  if (camera_) camera_->ShakeDirectional(direction, intensity, duration, frequency);
  if (gameCamera_) gameCamera_->ShakeDirectional(direction, intensity, duration, frequency);
}

void GameCameraController::StopShake() {
  if (camera_) camera_->StopShake();
  if (gameCamera_) gameCamera_->StopShake();
}

void GameCameraController::ShakeImpact() {
  Shake(0.85f, 0.4f, 25.0f);
}

void GameCameraController::ShakeDefeat() {
  Shake(0.42f, 0.22f, 30.0f);
}

void GameCameraController::ShakeBossDefeat() {
  Shake(1.2f, 0.6f, 20.0f);
}

void GameCameraController::ShakeLand() {
  ShakeDirectional({0.0f, 1.0f, 0.0f}, 0.22f, 0.18f, 30.0f);
}

void GameCameraController::ShakeReflect() {
  Shake(0.35f, 0.15f, 32.0f);
}

void GameCameraController::AnimateFov(float targetFov, float duration) {
  if (camera_) camera_->AnimateFov(targetFov, duration);
  if (gameCamera_) gameCamera_->AnimateFov(targetFov, duration);
}

void GameCameraController::AddFovImpulse(float deltaFov, float duration) {
  if (camera_) camera_->AddFovImpulse(deltaFov, duration);
  if (gameCamera_) gameCamera_->AddFovImpulse(deltaFov, duration);
}

void GameCameraController::ResetFov(float duration) {
  if (camera_) camera_->ResetFov(duration);
  if (gameCamera_) gameCamera_->ResetFov(duration);
}

void GameCameraController::SetPositionOffset(const Vector3 &offset) {
  if (camera_) camera_->SetPositionOffset(offset);
  if (gameCamera_) gameCamera_->SetPositionOffset(offset);
}

void GameCameraController::SetRotationOffset(const Vector3 &rotOffset) {
  if (camera_) camera_->SetRotationOffset(rotOffset);
  if (gameCamera_) gameCamera_->SetRotationOffset(rotOffset);
}

void GameCameraController::ClearOffsets() {
  if (camera_) camera_->ClearOffsets();
  if (gameCamera_) gameCamera_->ClearOffsets();
}

void GameCameraController::StartDollyZoom(const Vector3 &focusPoint, float targetDistance, float duration) {
  if (camera_) camera_->StartDollyZoom(focusPoint, targetDistance, duration);
  if (gameCamera_) gameCamera_->StartDollyZoom(focusPoint, targetDistance, duration);
}

void GameCameraController::StopDollyZoom() {
  if (camera_) camera_->StopDollyZoom();
  if (gameCamera_) gameCamera_->StopDollyZoom();
}

void GameCameraController::ImGui(Player *player,
                                 std::function<void(int)> onPresetSelected) {
#ifdef _USE_IMGUI
  if (ImGui::CollapsingHeader("GameScene Camera Settings",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    bool cameraChanged = false;
    if (ImGui::DragFloat3("Translate", &cameraTransform_.translate.x, 0.1f))
      cameraChanged = true;
    if (ImGui::DragFloat3("Rotate", &cameraTransform_.rotate.x, 0.01f))
      cameraChanged = true;
    if (ImGui::DragFloat3("Scale", &cameraTransform_.scale.x, 0.01f))
      cameraChanged = true;

    if (cameraChanged) {
      camera_->SetTransform(cameraTransform_);
      gameCamera_->SetTransform(cameraTransform_);
    }

    ImGui::Separator();
    // プレイヤー追従設定
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Player Follow Camera:");
    ImGui::Checkbox("Enable Follow Player", &isFollowPlayer_);
    if (isFollowPlayer_) {
      ImGui::SliderFloat("Follow Speed", &followSmoothSpeed_, 1.0f, 30.0f);
      ImGui::SliderFloat("Lane X Weight", &followWeightX_, 0.0f, 1.0f);
      ImGui::SliderFloat("Jump Y Weight", &followWeightY_, 0.0f, 1.0f);
      ImGui::SliderFloat("Bank Roll", &bankRollMultiplier_, 0.0f, 0.1f);
    }

    ImGui::Separator();
    // カメラ演出テスト (Game View & Scene View 同期)
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Synchronized Camera Effects (Game View Sync):");
    static float gShakeInt = 0.7f;
    static float gShakeDur = 0.35f;
    ImGui::SliderFloat("Shake Intensity##gc", &gShakeInt, 0.05f, 2.0f);
    ImGui::SliderFloat("Shake Duration##gc", &gShakeDur, 0.05f, 1.5f);
    if (ImGui::Button("Impact Shake##gc")) {
      ShakeImpact();
    }
    ImGui::SameLine();
    if (ImGui::Button("Defeat Shake##gc")) {
      ShakeDefeat();
    }
    ImGui::SameLine();
    if (ImGui::Button("Landing Shake##gc")) {
      ShakeLand();
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop Shake##gc")) {
      StopShake();
    }

    if (ImGui::Button("Speed FOV (0.60)")) {
      AnimateFov(0.60f, 0.4f);
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset FOV##gc")) {
      ResetFov(0.3f);
    }
    ImGui::SameLine();
    if (ImGui::Button("Hit Zoom (-0.08)")) {
      AddFovImpulse(-0.08f, 0.15f);
    }

    ImGui::Separator();
    ImGui::Text("Presets:");
    if (ImGui::Button("Behind View") && onPresetSelected) {
      onPresetSelected(static_cast<int>(Preset::BehindView));
    }
    ImGui::SameLine();
    if (ImGui::Button("Left Side View")) {
      cameraTransform_ = kDefaultLeftSideViewTransform;
      camera_->SetTransform(cameraTransform_);
      gameCamera_->SetTransform(cameraTransform_);
    }
    ImGui::SameLine();
    if (ImGui::Button("Right Side View") && onPresetSelected) {
      onPresetSelected(static_cast<int>(Preset::RightSideView));
    }
    ImGui::SameLine();
    if (ImGui::Button("Boss View") && onPresetSelected) {
      onPresetSelected(static_cast<int>(Preset::BossView));
    }
    ImGui::SameLine();
    if (ImGui::Button(isFirstPersonView_ ? "Disable FPV (F2)" : "First Person View (F2)")) {
      ToggleFirstPerson(player);
    }

    ImGui::Separator();
    if (ImGui::Button("Reset Debug Camera to Game Camera")) {
      camera_->ResetDebugCamera(cameraTransform_);
    }
  }

  if (ImGui::CollapsingHeader("First Person Camera Settings (F2)",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    if (ImGui::Checkbox("Enable First Person View (F2)", &isFirstPersonView_)) {
      if (!isFirstPersonView_) {
        camera_->SetTransform(cameraTransform_);
        gameCamera_->SetTransform(cameraTransform_);
        if (player) {
          player->SetVisible(true);
        }
      }
    }
    ImGui::Checkbox("Draw Player Body in First Person", &drawPlayerInFirstPerson_);
    ImGui::DragFloat3("FPV Camera Offset", &firstPersonOffset_.x, 0.02f);
    ImGui::DragFloat3("FPV Camera Rotate", &firstPersonRotate_.x, 0.01f);
  }
#endif
}

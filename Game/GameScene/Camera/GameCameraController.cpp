#include "GameCameraController.h"
#include <Entity/Player.h>
#include <Stage/StageSettings.h>
#include <cmath>
#include <imgui.h>

void GameCameraController::Initialize() {
  camera_->SetTransform(cameraTransform_);
  camera_->Update();

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

void GameCameraController::ImGui(Player *player,
                                 std::function<void(int)> onPresetSelected) {
#ifdef _USE_IMGUI
  camera_->ImGui();

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

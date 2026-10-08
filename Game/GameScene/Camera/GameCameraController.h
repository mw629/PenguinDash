#pragma once
#include "Camera.h"
#include <Engine.h>
#include <Math/Calculation.h>
#include <functional>
#include <memory>

class StageSettings;
class Player;

class GameCameraController {
private:
  std::unique_ptr<Camera> camera_ = std::make_unique<Camera>();
  std::unique_ptr<Camera> gameCamera_ = std::make_unique<Camera>();
  Transform cameraTransform_{{1.0f, 1.0f, 1.0f},
                             {0.3f, 0.0f, 0.0f},
                             {0.0f, 8.0f, -15.0f}};
  Matrix4x4 viewMatrix_;

  // トランジション
  bool isCameraTransitionPending_ = false;
  Transform pendingCameraTargetTransform_;
  int pendingLaneCount_ = 3;

  bool isCameraTransitioning_ = false;
  float cameraTransitionTimer_ = 0.0f;
  float cameraTransitionDuration_ = 1.0f;
  Transform startCameraTransform_;
  Transform targetCameraTransform_;

  // 一人称視点 (FPV)
  bool isFirstPersonView_ = false;
  bool drawPlayerInFirstPerson_ = false;
  Vector3 firstPersonOffset_{0.0f, 0.5f, 0.0f};
  Vector3 firstPersonRotate_{0.0f, 0.0f, 0.0f};

public:
  GameCameraController() = default;
  ~GameCameraController() = default;

  void Initialize();
  void Reset(const Transform &defaultTransform);
  void Update();

  void StartTransition(const Transform &targetTransform, int laneCount,
                       StageSettings *stageSettings);
  void UpdateTransition(StageSettings *stageSettings,
                        std::function<void()> onFinished = nullptr);

  void UpdateFirstPersonCamera(Player *player);
  void ToggleFirstPerson(Player *player);

  void SetTransform(const Transform &transform);
  const Transform &GetTransform() const { return cameraTransform_; }
  Transform &GetTransformRef() { return cameraTransform_; }

  Camera *GetCamera() const { return camera_.get(); }
  Camera *GetGameCamera() const { return gameCamera_.get(); }
  const Matrix4x4 &GetViewMatrix() const { return viewMatrix_; }
  Matrix4x4 GetGameViewMatrix() const { return gameCamera_->GetViewMatrix(); }

  void SetDebugCamera(bool debug);
  bool IsFirstPerson() const { return isFirstPersonView_; }
  bool IsTransitioning() const { return isCameraTransitioning_; }
  bool IsTransitionPending() const { return isCameraTransitionPending_; }

  void ImGui(Player *player, std::function<void(int)> onPresetSelected);
};

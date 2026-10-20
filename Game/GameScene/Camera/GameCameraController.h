#pragma once
#include "Camera.h"
#include <Engine.h>
#include <Math/Calculation.h>
#include <functional>
#include <memory>

class StageSettings;
class Player;

class GameCameraController {
public:
  enum class Preset {
    BehindView = 0,
    RightSideView = 1,
    BossView = 2,
  };

  static constexpr Transform kDefaultCameraTransform{
      {1.0f, 1.0f, 1.0f}, {0.3f, 0.0f, 0.0f}, {0.0f, 8.0f, -15.0f}};
  static constexpr Transform kDefaultLeftSideViewTransform{
      {1.0f, 1.0f, 1.0f}, {0.3f, 1.0472f, 0.0f}, {-20.0f, 8.0f, -5.0f}};
  static constexpr Transform kDefaultRightSideViewTransform{
      {1.0f, 1.0f, 1.0f}, {0.3f, -1.0472f, 0.0f}, {30.0f, 15.0f, -5.0f}};
  static constexpr Vector3 kDefaultFirstPersonOffset{0.0f, 0.5f, 0.0f};
  static constexpr float kDefaultTransitionDuration = 1.0f;
  static constexpr int kDefaultLaneCount = 3;

private:
  static constexpr float kTransitionArcHeight = 10.0f;
  static constexpr float kPi = 3.14159265f;

  std::unique_ptr<Camera> camera_ = std::make_unique<Camera>();
  std::unique_ptr<Camera> gameCamera_ = std::make_unique<Camera>();
  Transform cameraTransform_ = kDefaultCameraTransform;
  Matrix4x4 viewMatrix_;

  // トランジション
  bool isCameraTransitionPending_ = false;
  Transform pendingCameraTargetTransform_;
  int pendingLaneCount_ = kDefaultLaneCount;

  bool isCameraTransitioning_ = false;
  float cameraTransitionTimer_ = 0.0f;
  float cameraTransitionDuration_ = kDefaultTransitionDuration;
  Transform startCameraTransform_;
  Transform targetCameraTransform_;

  // 一人称視点 (FPV)
  bool isFirstPersonView_ = false;
  bool drawPlayerInFirstPerson_ = false;
  Vector3 firstPersonOffset_ = kDefaultFirstPersonOffset;
  Vector3 firstPersonRotate_{0.0f, 0.0f, 0.0f};

  // プレイヤー追従・バンク設定
  bool isFollowPlayer_ = true;
  float followSmoothSpeed_ = 10.0f;
  float followWeightX_ = 0.45f;
  float followWeightY_ = 0.25f;
  float bankRollMultiplier_ = 0.0f;
  Vector3 currentFollowOffset_{0.0f, 0.0f, 0.0f};
  float currentBankRoll_ = 0.0f;

public:
  GameCameraController() = default;
  ~GameCameraController() = default;

  void Initialize();
  void Reset(const Transform &defaultTransform);
  void Reset() { Reset(kDefaultCameraTransform); }
  void Update();

  // ゲームプレイ時のプレイヤー追従カメラ更新
  void UpdateGameplayCamera(Player *player, float dt);

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
  bool GetDebugCamera() const { return camera_ ? camera_->GetDebugCamera() : false; }
  bool IsDebugCamera() const { return GetDebugCamera(); }
  bool IsFirstPerson() const { return isFirstPersonView_; }
  bool IsTransitioning() const { return isCameraTransitioning_; }
  bool IsTransitionPending() const { return isCameraTransitionPending_; }

  // プレイヤー追従設定
  void SetFollowPlayer(bool follow) { isFollowPlayer_ = follow; }
  bool IsFollowPlayer() const { return isFollowPlayer_; }
  float GetFollowSmoothSpeed() const { return followSmoothSpeed_; }
  void SetFollowSmoothSpeed(float speed) { followSmoothSpeed_ = speed; }
  float GetFollowWeightX() const { return followWeightX_; }
  void SetFollowWeightX(float weight) { followWeightX_ = weight; }
  float GetFollowWeightY() const { return followWeightY_; }
  void SetFollowWeightY(float weight) { followWeightY_ = weight; }
  float GetBankRollMultiplier() const { return bankRollMultiplier_; }
  void SetBankRollMultiplier(float mult) { bankRollMultiplier_ = mult; }

  // =================================================================
  // カメラ演出 (camera_ と gameCamera_ の両方に同時に適用・Game View同期)
  // =================================================================
  void Shake(float intensity, float duration, float frequency = 25.0f);
  void ShakeDirectional(const Vector3 &direction, float intensity, float duration, float frequency = 25.0f);
  void StopShake();

  // 演出用プリセットシェイク (激突・敵撃破・着地など)
  void ShakeImpact();
  void ShakeDefeat();
  void ShakeBossDefeat();
  void ShakeLand();
  void ShakeReflect();

  void AnimateFov(float targetFov, float duration);
  void AddFovImpulse(float deltaFov, float duration);
  void ResetFov(float duration = 0.2f);

  void SetPositionOffset(const Vector3 &offset);
  void SetRotationOffset(const Vector3 &rotOffset);
  void ClearOffsets();

  void StartDollyZoom(const Vector3 &focusPoint, float targetDistance, float duration);
  void StopDollyZoom();

  void ImGui(Player *player, std::function<void(int)> onPresetSelected);
};

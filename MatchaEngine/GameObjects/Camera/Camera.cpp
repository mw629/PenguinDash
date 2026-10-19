#include "Camera.h"
#include "Graphics/GraphicsDevice.h"
#include <Engine.h>
#include <algorithm>
#include <cmath>


#ifdef _USE_IMGUI
#include <imgui.h>
#endif // _USE_IMGUI

float Camera::s_screenWidth_ = Camera::kDefaultScreenWidth;
float Camera::s_screenHeight_ = Camera::kDefaultScreenHeight;
std::vector<Camera *> Camera::s_instances_;

void Camera::SetScreenSize(Vector2 screenSize) {
  s_screenWidth_ = screenSize.x;
  s_screenHeight_ = screenSize.y;

  if (s_screenWidth_ > 0.0f && s_screenHeight_ > 0.0f) {
    float aspect = s_screenWidth_ / s_screenHeight_;
    for (auto *cam : s_instances_) {
      if (cam && cam->autoAspectRatio_) {
        cam->aspectRatio_ = aspect;
      }
    }
  }
}

Vector2 Camera::GetScreenSize() { return {s_screenWidth_, s_screenHeight_}; }

Camera::Camera() {
  if (s_screenWidth_ > 0.0f && s_screenHeight_ > 0.0f) {
    aspectRatio_ = s_screenWidth_ / s_screenHeight_;
  }
  s_instances_.push_back(this);
  debugName_ = "Camera_" + std::to_string(s_instances_.size());
  Initialize();
}

Camera::~Camera() {
  auto it = std::find(s_instances_.begin(), s_instances_.end(), this);
  if (it != s_instances_.end()) {
    s_instances_.erase(it);
  }
}

void Camera::ImGui() {
#ifdef _USE_IMGUI
  std::string headerName = debugName_ + " Settings";
  if (ImGui::CollapsingHeader(headerName.c_str(),
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    DrawDebugDetails();
  }
#endif // _USE_IMGUI
}

void Camera::DrawDebugDetails() {
#ifdef _USE_IMGUI
  ImGui::PushID(this);

  // 1. Transform 情報
  if (ImGui::TreeNodeEx("Transform (Base & Final)",
                        ImGuiTreeNodeFlags_DefaultOpen)) {
    bool changed = false;
    if (ImGui::DragFloat3("Translate", &transform_.translate.x, 0.1f))
      changed = true;
    if (ImGui::DragFloat3("Rotate", &transform_.rotate.x, 0.01f))
      changed = true;
    if (ImGui::DragFloat3("Scale", &transform_.scale.x, 0.01f))
      changed = true;
    if (changed) {
      SetTransform(transform_);
    }

    ImGui::Separator();
    ImGui::Text("Final Position: (%.2f, %.2f, %.2f)",
                finalTransform_.translate.x, finalTransform_.translate.y,
                finalTransform_.translate.z);
    ImGui::Text("Final Rotation: (%.2f, %.2f, %.2f)", finalTransform_.rotate.x,
                finalTransform_.rotate.y, finalTransform_.rotate.z);
    ImGui::TreePop();
  }

  // 2. Debug Camera 切り替え
  if (ImGui::TreeNodeEx("Debug Camera Mode", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Checkbox("Enable Debug Camera", &isDebugCamera_);
    if (isDebugCamera_) {
      ImGui::SameLine();
      if (ImGui::Button("Reset to Game Camera")) {
        ResetDebugCameraToGameCamera();
      }
      ImGui::Text("Eye: (%.2f, %.2f, %.2f)", debugCamera_.GetEye().x,
                  debugCamera_.GetEye().y, debugCamera_.GetEye().z);
      ImGui::Text("Target: (%.2f, %.2f, %.2f)", debugCamera_.GetTarget().x,
                  debugCamera_.GetTarget().y, debugCamera_.GetTarget().z);
    }
    ImGui::TreePop();
  }

  // 3. Camera Effects (演出コントロール)
  if (ImGui::TreeNodeEx("Camera Effects (Shake / FOV / Offset / Dolly)",
                        ImGuiTreeNodeFlags_DefaultOpen)) {
    // --- シェイク ---
    static float testIntensity = 0.6f;
    static float testDuration = 0.4f;
    static float testFrequency = 25.0f;
    ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.3f, 1.0f), "[Camera Shake]");
    ImGui::SliderFloat("Shake Intensity", &testIntensity, 0.05f, 3.0f);
    ImGui::SliderFloat("Shake Duration", &testDuration, 0.05f, 2.0f);
    ImGui::SliderFloat("Shake Frequency", &testFrequency, 5.0f, 60.0f);
    if (ImGui::Button("Trigger Shake")) {
      Shake(testIntensity, testDuration, testFrequency);
    }
    ImGui::SameLine();
    if (ImGui::Button("Landing Shake")) {
      ShakeDirectional({0.0f, -1.0f, 0.0f}, 0.5f, 0.25f, 30.0f);
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop Shake")) {
      StopShake();
    }
    ImGui::Text("Status: %s | Shake Offset: (%.2f, %.2f, %.2f)",
                isShaking_ ? "SHAKING" : "Idle", shakeOffset_.x, shakeOffset_.y,
                shakeOffset_.z);

    ImGui::Separator();
    // --- FOV ---
    ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.9f, 1.0f), "[Dynamic FOV / Zoom]");
    ImGui::DragFloat("Current FOV", &fovY_, 0.01f, 0.1f, 3.0f);
    ImGui::DragFloat("Base FOV", &baseFovY_, 0.01f, 0.1f, 3.0f);
    if (ImGui::Button("Speed Warp FOV (0.60)")) {
      AnimateFov(0.60f, 0.5f);
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset FOV")) {
      ResetFov(0.3f);
    }
    ImGui::SameLine();
    if (ImGui::Button("Hit Impulse (-0.08)")) {
      AddFovImpulse(-0.08f, 0.15f);
    }

    ImGui::Separator();
    // --- レイヤーオフセット ---
    ImGui::TextColored(ImVec4(0.5f, 0.9f, 0.5f, 1.0f), "[Layered Offsets]");
    ImGui::DragFloat3("Position Offset", &positionOffset_.x, 0.05f);
    ImGui::DragFloat3("Rotation Offset", &rotationOffset_.x, 0.01f);
    if (ImGui::Button("Clear Offsets")) {
      ClearOffsets();
    }

    ImGui::Separator();
    // --- ドリーズーム ---
    static Vector3 dollyFocusPoint{0.0f, 1.0f, 0.0f};
    static float dollyTargetDist = 5.0f;
    static float dollyDuration = 1.5f;
    ImGui::TextColored(ImVec4(0.9f, 0.5f, 0.8f, 1.0f),
                       "[Dolly Zoom (Vertigo Effect)]");
    ImGui::DragFloat3("Focus Point", &dollyFocusPoint.x, 0.1f);
    ImGui::DragFloat("Target Distance", &dollyTargetDist, 0.1f, 1.0f, 100.0f);
    ImGui::DragFloat("Duration", &dollyDuration, 0.1f, 0.2f, 10.0f);
    if (ImGui::Button("Start Dolly Zoom")) {
      StartDollyZoom(dollyFocusPoint, dollyTargetDist, dollyDuration);
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop Dolly Zoom")) {
      StopDollyZoom();
    }
    ImGui::Text("Dolly Zooming: %s", isDollyZooming_ ? "ACTIVE" : "Inactive");

    ImGui::TreePop();
  }

  // 4. Projection & Frustum
  if (ImGui::TreeNode("Projection & Frustum")) {
    ImGui::Checkbox("Auto Aspect Ratio", &autoAspectRatio_);
    if (!autoAspectRatio_) {
      ImGui::DragFloat("Aspect Ratio", &aspectRatio_, 0.01f, 0.1f, 10.0f);
    } else {
      ImGui::Text("Aspect: %.3f (Auto)", aspectRatio_);
    }
    ImGui::DragFloat("Near Clip", &nearClip_, 0.01f, 0.01f, 10.0f);
    ImGui::DragFloat("Far Clip", &farClip_, 1.0f, 10.0f, 50000.0f);
    ImGui::Text("Total Render Time: %.2f s", totalTime_);
    ImGui::TreePop();
  }

  ImGui::PopID();
#endif // _USE_IMGUI
}

void Camera::Initialize() {
  gameTransform_ = transform_;
  finalTransform_ = transform_;
  baseFovY_ = fovY_;
  debugCamera_.Initialize();
  viewMatrix_ = Inverse(MakeAffineMatrix(transform_.translate, transform_.scale,
                                         transform_.rotate));
  projectionMatrix_ =
      MakePerspectiveFovMatrix(fovY_, aspectRatio_, nearClip_, farClip_);

  // カメラ用のConstantBufferを作成
  cameraResource_ = GraphicsDevice::CreateBufferResource(sizeof(CameraForGPU));
  cameraResource_->Map(0, nullptr, reinterpret_cast<void **>(&cameraData_));

  // 初期値を設定
  cameraData_->worldPosition = transform_.translate;
  cameraData_->time = 0.0f;
  frustum_ = Frustum::FromViewProjection(GetViewProjectionMatrix());
}

void Camera::SetTransform(Transform transform) {
  gameTransform_ = transform;
  transform_ = transform;
  finalTransform_ = transform;
  if (isDebugCamera_) {
    debugCamera_.SetEye(transform.translate);
  }
}

void Camera::ResetDebugCameraToGameCamera() {
  ResetDebugCamera(gameTransform_);
}

void Camera::ResetDebugCamera(const Transform &transform) {
  debugCamera_.ResetToCamera(transform.translate, transform.rotate);
  transform_.translate = transform.translate;
  finalTransform_ = transform_;
  viewMatrix_ = debugCamera_.GetViewMatrix();
}

void Camera::Update() {
  debugCamera_.Update();

  // 演出の更新
  UpdateShake(kDefaultDeltaTime);
  UpdateFov(kDefaultDeltaTime);
  UpdateDollyZoom(kDefaultDeltaTime);

  // カメラの位置を決める
  Vector3 eye;

  if (isDebugCamera_) {
    viewMatrix_ = debugCamera_.GetViewMatrix();
    eye = debugCamera_.GetEye();
    transform_.translate = eye;
    finalTransform_ = transform_;
  } else {
    // ベース Transform に演出オフセットとシェイクを合成
    finalTransform_.scale = transform_.scale;
    finalTransform_.rotate = {
        transform_.rotate.x + rotationOffset_.x + shakeRotationOffset_.x,
        transform_.rotate.y + rotationOffset_.y + shakeRotationOffset_.y,
        transform_.rotate.z + rotationOffset_.z + shakeRotationOffset_.z};
    finalTransform_.translate = {
        transform_.translate.x + positionOffset_.x + shakeOffset_.x,
        transform_.translate.y + positionOffset_.y + shakeOffset_.y,
        transform_.translate.z + positionOffset_.z + shakeOffset_.z};

    Matrix4x4 cameraMatrix =
        MakeAffineMatrix(finalTransform_.translate, finalTransform_.scale,
                         finalTransform_.rotate);
    viewMatrix_ = Inverse(cameraMatrix);
    eye = finalTransform_.translate;
  }

  totalTime_ += kDefaultDeltaTime;
  cameraData_->worldPosition = eye;
  cameraData_->time = totalTime_;

  if (autoAspectRatio_ && s_screenWidth_ > 0.0f && s_screenHeight_ > 0.0f) {
    aspectRatio_ = s_screenWidth_ / s_screenHeight_;
  }

  // FOV インパルスの合成
  float activeFov = fovY_;
  if (fovImpulseDuration_ > 0.0f) {
    float p = fovImpulseTimer_ / fovImpulseDuration_;
    float decay = (1.0f - p) * (1.0f - p);
    activeFov += fovImpulse_ * decay;
  }
  activeFov = std::clamp(activeFov, 0.05f, 3.1f);

  projectionMatrix_ =
      MakePerspectiveFovMatrix(activeFov, aspectRatio_, nearClip_, farClip_);
  frustum_ = Frustum::FromViewProjection(GetViewProjectionMatrix());
}

// =================================================================
// カメラ演出機能の実装
// =================================================================

// --- 1. カメラシェイク ---
void Camera::Shake(float intensity, float duration, float frequency) {
  ShakeParams params;
  params.intensity = intensity;
  params.duration = duration;
  params.frequency = frequency;
  Shake(params);
}

void Camera::Shake(const ShakeParams &params) {
  currentShakeParams_ = params;
  shakeTimer_ = 0.0f;
  isShaking_ = true;
}

void Camera::ShakeDirectional(const Vector3 &direction, float intensity,
                              float duration, float frequency) {
  ShakeParams params;
  params.intensity = intensity;
  params.duration = duration;
  params.frequency = frequency;
  Vector3 dir = direction;
  float len = Length(dir);
  if (len > 0.0001f) {
    dir.x /= len;
    dir.y /= len;
    dir.z /= len;
    params.positionWeight = {std::abs(dir.x) * 1.5f + 0.1f,
                             std::abs(dir.y) * 1.5f + 0.1f,
                             std::abs(dir.z) * 1.5f + 0.1f};
  }
  Shake(params);
}

void Camera::StopShake() {
  isShaking_ = false;
  shakeTimer_ = 0.0f;
  shakeOffset_ = {0.0f, 0.0f, 0.0f};
  shakeRotationOffset_ = {0.0f, 0.0f, 0.0f};
}

void Camera::UpdateShake(float dt) {
  if (!isShaking_) {
    shakeOffset_ = {0.0f, 0.0f, 0.0f};
    shakeRotationOffset_ = {0.0f, 0.0f, 0.0f};
    return;
  }

  shakeTimer_ += dt;
  if (shakeTimer_ >= currentShakeParams_.duration) {
    StopShake();
    return;
  }

  float progress = shakeTimer_ / currentShakeParams_.duration;
  float decay =
      currentShakeParams_.decay ? (1.0f - progress) * (1.0f - progress) : 1.0f;
  float currentIntensity = currentShakeParams_.intensity * decay;

  float freq = currentShakeParams_.frequency;
  float t = shakeTimer_ * freq * 6.2831853f;

  float nx = std::sin(t * 1.0f) * 0.7f + std::sin(t * 2.31f + 1.2f) * 0.3f;
  float ny =
      std::sin(t * 1.37f + 0.8f) * 0.7f + std::sin(t * 2.87f + 2.5f) * 0.3f;
  float nz =
      std::sin(t * 0.73f + 1.9f) * 0.7f + std::sin(t * 1.93f + 0.4f) * 0.3f;

  shakeOffset_.x = nx * currentIntensity * currentShakeParams_.positionWeight.x;
  shakeOffset_.y = ny * currentIntensity * currentShakeParams_.positionWeight.y;
  shakeOffset_.z = nz * currentIntensity * currentShakeParams_.positionWeight.z;

  shakeRotationOffset_.x =
      ny * currentIntensity * currentShakeParams_.rotationWeight.x;
  shakeRotationOffset_.y =
      nx * currentIntensity * currentShakeParams_.rotationWeight.y;
  shakeRotationOffset_.z = (nx - ny) * 0.5f * currentIntensity *
                           currentShakeParams_.rotationWeight.z;
}

// --- 2. 画角制御 ---
void Camera::SetFovY(float fovY) {
  fovY_ = fovY;
  isFovTransitioning_ = false;
}

void Camera::SetBaseFovY(float baseFovY) { baseFovY_ = baseFovY; }

void Camera::AnimateFov(float targetFov, float duration) {
  if (duration <= 0.0f) {
    fovY_ = targetFov;
    isFovTransitioning_ = false;
    return;
  }
  fovStart_ = fovY_;
  fovTarget_ = targetFov;
  fovTransitionDuration_ = duration;
  fovTransitionTimer_ = 0.0f;
  isFovTransitioning_ = true;
}

void Camera::AddFovImpulse(float deltaFov, float duration) {
  fovImpulse_ = deltaFov;
  fovImpulseDuration_ = (duration > 0.0f) ? duration : 0.1f;
  fovImpulseTimer_ = 0.0f;
}

void Camera::ResetFov(float duration) {
  if (duration <= 0.0f) {
    fovY_ = baseFovY_;
    isFovTransitioning_ = false;
  } else {
    AnimateFov(baseFovY_, duration);
  }
}

void Camera::UpdateFov(float dt) {
  if (isFovTransitioning_) {
    fovTransitionTimer_ += dt;
    float t = fovTransitionTimer_ / fovTransitionDuration_;
    if (t >= 1.0f) {
      t = 1.0f;
      isFovTransitioning_ = false;
      fovY_ = fovTarget_;
    } else {
      // EaseInOutCubic
      float easeT = (t < 0.5f)
                        ? (4.0f * t * t * t)
                        : (1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) * 0.5f);
      fovY_ = Lerp(fovStart_, fovTarget_, easeT);
    }
  }

  if (fovImpulseDuration_ > 0.0f) {
    fovImpulseTimer_ += dt;
    if (fovImpulseTimer_ >= fovImpulseDuration_) {
      fovImpulse_ = 0.0f;
      fovImpulseDuration_ = 0.0f;
      fovImpulseTimer_ = 0.0f;
    }
  }
}

// --- 4. スムーズ追従 ---
void Camera::SmoothFollow(const Vector3 &targetPosition, float smoothSpeed,
                          float deltaTime) {
  float t = std::clamp(smoothSpeed * deltaTime, 0.0f, 1.0f);
  transform_.translate = Lerp(transform_.translate, targetPosition, t);
}

void Camera::SmoothRotate(const Vector3 &targetRotate, float smoothSpeed,
                          float deltaTime) {
  float t = std::clamp(smoothSpeed * deltaTime, 0.0f, 1.0f);
  transform_.rotate.x = LerpAngle(transform_.rotate.x, targetRotate.x, t);
  transform_.rotate.y = LerpAngle(transform_.rotate.y, targetRotate.y, t);
  transform_.rotate.z = LerpAngle(transform_.rotate.z, targetRotate.z, t);
}

// --- 5. ドリーズーム ---
void Camera::StartDollyZoom(const Vector3 &focusPoint, float targetDistance,
                            float duration) {
  dollyFocusPoint_ = focusPoint;
  Vector3 toCam = transform_.translate - focusPoint;
  dollyStartDistance_ = Length(toCam);
  if (dollyStartDistance_ < 0.001f)
    return;

  dollyTargetDistance_ = targetDistance;
  dollyDuration_ = (duration > 0.0f) ? duration : 1.0f;
  dollyTimer_ = 0.0f;

  dollyTargetFrustumHeight_ =
      2.0f * dollyStartDistance_ * std::tan(fovY_ * 0.5f);
  isDollyZooming_ = true;
}

void Camera::UpdateDollyZoom(float dt) {
  if (!isDollyZooming_)
    return;

  dollyTimer_ += dt;
  float t = dollyTimer_ / dollyDuration_;
  if (t >= 1.0f) {
    t = 1.0f;
    isDollyZooming_ = false;
  }

  float easeT = t * t * (3.0f - 2.0f * t);
  float curDist = Lerp(dollyStartDistance_, dollyTargetDistance_, easeT);

  Vector3 toCam = transform_.translate - dollyFocusPoint_;
  float len = Length(toCam);
  if (len > 0.0001f) {
    Vector3 dir = {toCam.x / len, toCam.y / len, toCam.z / len};
    transform_.translate = dollyFocusPoint_ + dir * curDist;
  }

  if (curDist > 0.001f) {
    float halfTan = (dollyTargetFrustumHeight_ * 0.5f) / curDist;
    fovY_ = 2.0f * std::atan(halfTan);
    fovY_ = std::clamp(fovY_, 0.05f, 3.0f);
  }
}

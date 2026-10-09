#include "DebugCamera.h"
#include <cmath>
#include <numbers>
#include "Math/Calculation.h"

// ============================================================
// Blender-style Debug Camera Controls:
//   RMB drag           : Orbit (rotate around target)
//   Shift + RMB drag   : Pan (translate camera & target)
//   MMB drag           : Pan (same as Shift+RMB)
//   Mouse wheel        : Zoom (adjust distance to target)
//   Numpad 0           : Reset to initial position
//   Numpad 1           : Front view  (+Z axis)
//   Numpad 3           : Right view  (+X axis)
//   Numpad 7           : Top view    (+Y axis)
// ============================================================

void DebugCamera::Initialize() {
    eye_    = kDefaultEye;
    target_ = kDefaultTarget;
    up_     = kDefaultUp;

    Vector3 diff = eye_ - target_;
    radius_ = Length(diff);
    phi_    = std::atan2(diff.y, std::sqrt(diff.x * diff.x + diff.z * diff.z));
    theta_  = std::atan2(diff.x, diff.z);

    mousePrevPos_              = { 0, 0 };
    isRightMouseButtonPressed_ = false;
    isMiddleMouseButtonPressed_= false;
}

void DebugCamera::SetEye(Vector3 eye) {
    eye_ = eye;
    Vector3 diff = eye_ - target_;
    radius_ = Length(diff);
    if (radius_ > kMinLengthThreshold) {
        phi_ = std::atan2(diff.y, std::sqrt(diff.x * diff.x + diff.z * diff.z));
        theta_ = std::atan2(diff.x, diff.z);
    }
    viewMatrix_ = MakeLookAtLH(eye_, target_, up_);
}

void DebugCamera::SetTarget(Vector3 target) {
    target_ = target;
    Vector3 diff = eye_ - target_;
    radius_ = Length(diff);
    if (radius_ > kMinLengthThreshold) {
        phi_ = std::atan2(diff.y, std::sqrt(diff.x * diff.x + diff.z * diff.z));
        theta_ = std::atan2(diff.x, diff.z);
    }
    viewMatrix_ = MakeLookAtLH(eye_, target_, up_);
}

void DebugCamera::Focus(const Vector3& target, float distance) {
    target_ = target;
    radius_ = (distance > kMinFocusDistanceThreshold) ? distance : kFallbackFocusDistance;
    phi_ = kFocusDefaultPhi;
    theta_ = std::numbers::pi_v<float>; // -Z 方向からターゲットを見る

    eye_.x = target_.x + radius_ * std::sin(theta_) * std::cos(phi_);
    eye_.y = target_.y + radius_ * std::sin(phi_);
    eye_.z = target_.z + radius_ * std::cos(theta_) * std::cos(phi_);

    up_ = kDefaultUp;
    viewMatrix_ = MakeLookAtLH(eye_, target_, up_);
}

void DebugCamera::Orbit(float deltaX, float deltaY) {
    theta_ += deltaX * kRotSpeed;
    phi_   -= deltaY * kRotSpeed;
    phi_ = std::fmaxf(-kPiOver2 + kGimbalLockMargin, std::fminf(kPiOver2 - kGimbalLockMargin, phi_));

    eye_.x = target_.x + radius_ * std::sin(theta_) * std::cos(phi_);
    eye_.y = target_.y + radius_ * std::sin(phi_);
    eye_.z = target_.z + radius_ * std::cos(theta_) * std::cos(phi_);
    viewMatrix_ = MakeLookAtLH(eye_, target_, up_);
}

void DebugCamera::Pan(float deltaX, float deltaY) {
    Vector3 forward  = Normalize(target_ - eye_);
    Vector3 right    = Normalize(Cross(up_, forward));
    Vector3 localUp  = Normalize(Cross(forward, right));

    float panFactor = radius_ * kPanBase;
    Vector3 move = right * ((float)-deltaX * panFactor) + localUp * ((float)deltaY * panFactor);
    eye_    += move;
    target_ += move;
    viewMatrix_ = MakeLookAtLH(eye_, target_, up_);
}

void DebugCamera::Zoom(float wheelDelta) {
    float zoomFactor = (std::max)(radius_ * kZoomFactorScale, kMinZoomFactor);
    radius_ -= wheelDelta * zoomFactor;
    if (radius_ < kMinInteractiveZoomRadius) radius_ = kMinInteractiveZoomRadius;

    eye_.x = target_.x + radius_ * std::sin(theta_) * std::cos(phi_);
    eye_.y = target_.y + radius_ * std::sin(phi_);
    eye_.z = target_.z + radius_ * std::cos(theta_) * std::cos(phi_);
    viewMatrix_ = MakeLookAtLH(eye_, target_, up_);
}

void DebugCamera::ResetToCamera(const Vector3& eye, const Vector3& rotation, float distance) {
    eye_ = eye;
    up_  = kDefaultUp;

    // 回転角からForwardベクトルを計算 (ローカル+Z軸)
    Matrix4x4 rotMat = Rotation(rotation);
    Vector3 forward = { rotMat.m[2][0], rotMat.m[2][1], rotMat.m[2][2] };
    float fLen = Length(forward);
    if (fLen > kMinLengthThreshold) {
        forward = forward / fLen;
    } else {
        forward = kFallbackForward;
    }

    // カメラの注視点を前方 distance の位置に設定
    float dist = (distance > kMinDistanceThreshold) ? distance : kDefaultFocusDistance;
    target_ = eye_ + forward * dist;

    // 球座標（diff = eye_ - target_）を再計算
    Vector3 diff = eye_ - target_;
    radius_ = Length(diff);
    if (radius_ > kMinLengthThreshold) {
        phi_   = std::atan2(diff.y, std::sqrt(diff.x * diff.x + diff.z * diff.z));
        theta_ = std::atan2(diff.x, diff.z);
    }
    phi_ = std::fmaxf(-kPiOver2 + kGimbalLockMargin, std::fminf(kPiOver2 - kGimbalLockMargin, phi_));

    // 球座標から eye_ を再計算
    eye_.x = target_.x + radius_ * std::sin(theta_) * std::cos(phi_);
    eye_.y = target_.y + radius_ * std::sin(phi_);
    eye_.z = target_.z + radius_ * std::cos(theta_) * std::cos(phi_);

    viewMatrix_ = MakeLookAtLH(eye_, target_, up_);
}

void DebugCamera::Update() {
    if (enableInput_) {
        isRightMouseButtonPressed_  = Input::PressMouse(1);  // RMB
        isMiddleMouseButtonPressed_ = Input::PressMouse(2);  // MMB

        Vector2 mouseDelta = Input::GetMouseDelta();

        // ---- Orbit (RMB drag, no shift) ----
        bool wantOrbit = isRightMouseButtonPressed_ && !Input::PressKey(DIK_LSHIFT);
        if (wantOrbit) {
            theta_ += (float)mouseDelta.x * kRotSpeed;
            phi_   -= (float)mouseDelta.y * kRotSpeed;
            // Clamp phi to avoid gimbal flip
            phi_ = std::fmaxf(-kPiOver2 + kGimbalLockMargin, std::fminf(kPiOver2 - kGimbalLockMargin, phi_));
        }

        // ---- Pan (Shift+RMB  or  MMB) ----
        bool wantPan = (isRightMouseButtonPressed_ && Input::PressKey(DIK_LSHIFT))
                     || isMiddleMouseButtonPressed_;
        if (wantPan) {
            Vector3 forward  = Normalize(target_ - eye_);
            Vector3 right    = Normalize(Cross(up_, forward));
            Vector3 localUp  = Normalize(Cross(forward, right));

            float panFactor = radius_ * kPanBase;

            eye_    += right   * ((float)-mouseDelta.x * panFactor);
            target_ += right   * ((float)-mouseDelta.x * panFactor);
            eye_    += localUp * ((float) mouseDelta.y * panFactor);
            target_ += localUp * ((float) mouseDelta.y * panFactor);
        }

        // ---- Zoom (mouse wheel) ----
        float wheelDelta = (float)Input::GetMouseWheel();
        if (wheelDelta != 0.0f) {
            radius_ -= wheelDelta * kZoomSpeed;
            if (radius_ < kMinZoomRadius) radius_ = kMinZoomRadius;
        }

        // ---- Numpad view snaps ----
        // Numpad 0 : reset
        if (Input::PushKey(DIK_NUMPAD0)) {
            target_ = kDefaultTarget;
            radius_ = Length(kDefaultEye - target_);
            phi_    = std::atan2(kDefaultEye.y, std::sqrt(kDefaultEye.x * kDefaultEye.x + kDefaultEye.z * kDefaultEye.z));
            theta_  = std::atan2(kDefaultEye.x, kDefaultEye.z);
        }
        // Numpad 1 : front view (look from +Z toward origin)
        if (Input::PushKey(DIK_NUMPAD1)) {
            theta_ = 0.0f;
            phi_   = 0.0f;
        }
        // Numpad 3 : right view (look from +X toward origin)
        if (Input::PushKey(DIK_NUMPAD3)) {
            theta_ = kPiOver2;
            phi_   = 0.0f;
        }
        // Numpad 7 : top view (look from +Y toward origin)
        if (Input::PushKey(DIK_NUMPAD7)) {
            theta_ = 0.0f;
            phi_   = kPiOver2 - kGimbalLockMargin;
        }
    }

    // ---- Recompute eye position from spherical coords ----
    eye_.x = target_.x + radius_ * std::sin(theta_) * std::cos(phi_);
    eye_.y = target_.y + radius_ * std::sin(phi_);
    eye_.z = target_.z + radius_ * std::cos(theta_) * std::cos(phi_);

    viewMatrix_ = MakeLookAtLH(eye_, target_, up_);
}

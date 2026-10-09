#pragma once
#include "../Core/VariableTypes.h"
#include "../Input/Input.h"


class DebugCamera {
private:
    static constexpr float kDefaultFocusDistance = 15.0f;
    static constexpr float kMinFocusDistanceThreshold = 0.5f;
    static constexpr float kFallbackFocusDistance = 10.0f;
    static constexpr float kFocusDefaultPhi = 0.35f;
    static constexpr float kRotSpeed = 0.007f;
    static constexpr float kPanBase = 0.0015f;
    static constexpr float kZoomSpeed = 0.015f;
    static constexpr float kMinZoomRadius = 0.1f;
    static constexpr float kMinInteractiveZoomRadius = 0.2f;
    static constexpr float kZoomFactorScale = 0.1f;
    static constexpr float kMinZoomFactor = 0.3f;
    static constexpr float kGimbalLockMargin = 0.01f;
    static constexpr float kMinLengthThreshold = 0.0001f;
    static constexpr float kMinDistanceThreshold = 0.1f;
    static constexpr Vector3 kDefaultEye = { 0.0f, 5.0f, -10.0f };
    static constexpr Vector3 kDefaultTarget = { 0.0f, 0.0f, 0.0f };
    static constexpr Vector3 kDefaultUp = { 0.0f, 1.0f, 0.0f };
    static constexpr Vector3 kFallbackForward = { 0.0f, 0.0f, 1.0f };

    Vector3 eye_{};
    Vector3 target_{};
    Vector3 up_{};

    Vector2 mousePrevPos_{};
    bool isRightMouseButtonPressed_ = false;
    bool isMiddleMouseButtonPressed_ = false;

    float radius_=0.0f;
    float phi_=0.0f;
    float theta_=0.0f;

    Matrix4x4 viewMatrix_{};
    bool enableInput_ = true;

public:
    void Initialize();
    void Update();
    void SetEnableInput(bool enable) { enableInput_ = enable; }
    bool IsInputEnabled() const { return enableInput_; }
    Matrix4x4 GetViewMatrix()const { return viewMatrix_; }
    Vector3 GetTarget() const { return target_; }
    Vector3 GetEye() const { return eye_; }
    void SetEye(Vector3 eye);
    void SetTarget(Vector3 target);
    void Focus(const Vector3& target, float distance);
    void Focus(const Vector3& target) { Focus(target, kDefaultFocusDistance); }
    void Orbit(float deltaX, float deltaY);
    void Pan(float deltaX, float deltaY);
    void Zoom(float wheelDelta);
    void ResetToCamera(const Vector3& eye, const Vector3& rotation, float distance);
    void ResetToCamera(const Vector3& eye, const Vector3& rotation) { ResetToCamera(eye, rotation, kDefaultFocusDistance); }
};

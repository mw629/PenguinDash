#pragma once
#include "../Core/VariableTypes.h"
#include "../Input/Input.h"


class DebugCamera {
private:
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
    void Focus(const Vector3& target, float distance = 15.0f);
    void Orbit(float deltaX, float deltaY);
    void Pan(float deltaX, float deltaY);
    void Zoom(float wheelDelta);
    void ResetToCamera(const Vector3& eye, const Vector3& rotation, float distance = 15.0f);
};

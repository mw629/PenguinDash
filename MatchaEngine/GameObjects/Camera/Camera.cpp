#include "Camera.h"
#include <Engine.h>
#include "Graphics/GraphicsDevice.h"
#include <algorithm>

#ifdef _USE_IMGUI
#include <imgui.h>
#endif // _USE_IMGUI

float Camera::s_screenWidth_ = 1280.0f;
float Camera::s_screenHeight_ = 720.0f;
std::vector<Camera*> Camera::s_instances_;

void Camera::SetScreenSize(Vector2 screenSize)
{
	s_screenWidth_ = screenSize.x;
	s_screenHeight_ = screenSize.y;

	if (s_screenWidth_ > 0.0f && s_screenHeight_ > 0.0f) {
		float aspect = s_screenWidth_ / s_screenHeight_;
		for (auto* cam : s_instances_) {
			if (cam && cam->autoAspectRatio_) {
				cam->aspectRatio_ = aspect;
			}
		}
	}
}

Vector2 Camera::GetScreenSize()
{
	return { s_screenWidth_, s_screenHeight_ };
}

Camera::Camera()
{
	if (s_screenWidth_ > 0.0f && s_screenHeight_ > 0.0f) {
		aspectRatio_ = s_screenWidth_ / s_screenHeight_;
	}
	s_instances_.push_back(this);
	Initialize();
}

Camera::~Camera()
{
	auto it = std::find(s_instances_.begin(), s_instances_.end(), this);
	if (it != s_instances_.end()) {
		s_instances_.erase(it);
	}
}

void Camera::ImGui()
{
#ifdef _USE_IMGUI
	if (ImGui::CollapsingHeader("Camera")) {
		ImGui::DragFloat3("CameraPos", &transform_.translate.x, 0.01f, -FLT_MAX, FLT_MAX, "%.2f");
		ImGui::DragFloat3("CameraSize", &transform_.scale.x, 0.01f, -FLT_MAX, FLT_MAX, "%.2f");
		ImGui::DragFloat3("CameraRotate", &transform_.rotate.x, 0.01f, -FLT_MAX, FLT_MAX, "%.2f");
		ImGui::Checkbox("Auto Aspect Ratio", &autoAspectRatio_);
		if (!autoAspectRatio_) {
			ImGui::DragFloat("Aspect Ratio", &aspectRatio_, 0.01f, 0.1f, 10.0f);
		} else {
			ImGui::Text("Aspect: %.3f (Auto)", aspectRatio_);
		}
	}
	ImGui::Checkbox("debugCamera", &isDebugCamera_);
	if (isDebugCamera_) {
		ImGui::SameLine();
		if (ImGui::Button("Reset Debug Camera to Game Camera")) {
			ResetDebugCameraToGameCamera();
		}
	}
#endif // _USE_IMGUI
}

void Camera::Initialize() {
	gameTransform_ = transform_;
	debugCamera_.Initialize();
	viewMatrix_ = Inverse(MakeAffineMatrix(transform_.translate,transform_.scale,transform_.rotate));
	projectionMatrix_ = MakePerspectiveFovMatrix(fovY_, aspectRatio_, nearClip_, farClip_);

	// カメラ用のConstantBufferを作成
	cameraResource_ = GraphicsDevice::CreateBufferResource(sizeof(CameraForGPU));
	cameraResource_->Map(0, nullptr, reinterpret_cast<void**>(&cameraData_));
	
	// 初期値を設定
	cameraData_->worldPosition = transform_.translate;
	cameraData_->time = 0.0f;
	frustum_ = Frustum::FromViewProjection(GetViewProjectionMatrix());
}

void Camera::SetTransform(Transform transform) {
	gameTransform_ = transform;
	transform_ = transform;
	if (isDebugCamera_) {
		debugCamera_.SetEye(transform.translate);
	}
}

void Camera::ResetDebugCameraToGameCamera() {
	ResetDebugCamera(gameTransform_);
}

void Camera::ResetDebugCamera(const Transform& transform) {
	debugCamera_.ResetToCamera(transform.translate, transform.rotate);
	transform_.translate = transform.translate;
	viewMatrix_ = debugCamera_.GetViewMatrix();
}

void Camera::Update() {
    debugCamera_.Update();

    // カメラの位置・回転を決める
    Vector3 eye;
    Vector3 target;
    Vector3 up = { 0.0f, 1.0f, 0.0f };

    if (isDebugCamera_) {
        viewMatrix_ = debugCamera_.GetViewMatrix();
        eye = debugCamera_.GetEye();
        transform_.translate = eye;
    }
    else {
        Matrix4x4 cameraMatrix = MakeAffineMatrix(transform_.translate, transform_.scale, transform_.rotate);
        viewMatrix_ = Inverse(cameraMatrix);
        eye = transform_.translate;
    }

    totalTime_ += 1.0f / 60.0f;
    cameraData_->worldPosition = eye;
    cameraData_->time = totalTime_;

    if (autoAspectRatio_ && s_screenWidth_ > 0.0f && s_screenHeight_ > 0.0f) {
        aspectRatio_ = s_screenWidth_ / s_screenHeight_;
    }

    projectionMatrix_ = MakePerspectiveFovMatrix(fovY_, aspectRatio_, nearClip_, farClip_);
    frustum_ = Frustum::FromViewProjection(GetViewProjectionMatrix());
}

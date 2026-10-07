#pragma once
#include "../Core/VariableTypes.h"
#include "DebugCamera.h"
#include <d3d12.h>
#include <wrl.h>
#include "Calculation.h"
#include <vector>

class Camera
{
private:
	static float s_screenWidth_;
	static float s_screenHeight_;
	static std::vector<Camera*> s_instances_;

	Transform transform_{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	Transform gameTransform_{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };

	Matrix4x4 viewMatrix_{};
	Matrix4x4 projectionMatrix_{};

	float fovY_ = 0.45f;
	float aspectRatio_ = 1280.0f / 720.0f;
	float nearClip_ = 0.1f;
	float farClip_ = 10000.0f;
	bool autoAspectRatio_ = true;

	DebugCamera debugCamera_;

	bool isDebugCamera_=true;
	float totalTime_ = 0.0f;

	Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource_;
	CameraForGPU* cameraData_ = nullptr;

public:
	Camera();
	~Camera();

	static void SetScreenSize(Vector2 screenSize);
	static Vector2 GetScreenSize();

	void ImGui();

	void Initialize();
	void Update();
	Matrix4x4 GetViewMatrix()const { return viewMatrix_; }
	Matrix4x4 GetProjectionMatrix() const { return projectionMatrix_; }
	Matrix4x4 GetViewProjectionMatrix() const { return MultiplyMatrix4x4(viewMatrix_, projectionMatrix_); }

	const Transform& GetTransform() const { return transform_; }
	const Transform& GetGameTransform() const { return gameTransform_; }
	void SetTransform(Transform transform);
	void SetDebugCamera(bool isDebug) { isDebugCamera_ = isDebug; }
	bool GetDebugCamera() const { return isDebugCamera_; }
	void ResetDebugCameraToGameCamera();
	void ResetDebugCamera(const Transform& transform);
	DebugCamera& GetDebugCameraRef() { return debugCamera_; }

	void SetTarget(const Vector3& target) { debugCamera_.SetTarget(target); }
	Vector3 GetTarget() const { return debugCamera_.GetTarget(); }
	void Focus(const Vector3& target, float distance = 15.0f) {
		debugCamera_.Focus(target, distance);
		transform_.translate = debugCamera_.GetEye();
		viewMatrix_ = debugCamera_.GetViewMatrix();
	}

	void SetAspectRatio(float aspectRatio) { aspectRatio_ = aspectRatio; autoAspectRatio_ = false; }
	float GetAspectRatio() const { return aspectRatio_; }
	void SetAutoAspectRatio(bool autoAspect) { autoAspectRatio_ = autoAspect; }
	bool GetAutoAspectRatio() const { return autoAspectRatio_; }
	
	ID3D12Resource* GetCameraResource() const { return cameraResource_.Get(); }

	float GetTotalTime() const { return totalTime_; }

	const Frustum& GetFrustum() const { return frustum_; }

private:
	Frustum frustum_{};
};


#pragma once
#include "../Core/VariableTypes.h"
#include "DebugCamera.h"
#include <d3d12.h>
#include <wrl.h>
#include "Calculation.h"
#include <vector>
#include <string>

class Camera
{
public:
	// シェイク設定パラメータ
	struct ShakeParams {
		float intensity = 0.5f;                         // 揺れの強さ (ワールド単位)
		float duration = 0.3f;                          // 継続時間 (秒)
		float frequency = 25.0f;                        // 振動周波数 (Hz)
		Vector3 positionWeight{ 1.0f, 1.0f, 0.5f };     // 各移動軸の揺れ比率 (X, Y, Z)
		Vector3 rotationWeight{ 0.04f, 0.04f, 0.02f };  // 各回転軸の揺れ比率 (Pitch, Yaw, Roll / rad)
		bool decay = true;                              // 時間経過で減衰するか
	};

private:
	static float s_screenWidth_;
	static float s_screenHeight_;
	static std::vector<Camera*> s_instances_;

	static constexpr Transform kDefaultTransform = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };

	Transform transform_ = kDefaultTransform;
	Transform gameTransform_ = kDefaultTransform;
	Transform finalTransform_ = kDefaultTransform; // シェイク・演出オフセット合成後の最終姿勢

	Matrix4x4 viewMatrix_{};
	Matrix4x4 projectionMatrix_{};

public:
	static constexpr float kDefaultScreenWidth = 1280.0f;
	static constexpr float kDefaultScreenHeight = 720.0f;
	static constexpr float kDefaultFovY = 0.45f;
	static constexpr float kDefaultNearClip = 0.1f;
	static constexpr float kDefaultFarClip = 10000.0f;
	static constexpr float kDefaultAspectRatio = kDefaultScreenWidth / kDefaultScreenHeight;

private:
	static constexpr float kDefaultDeltaTime = 1.0f / 60.0f;
	static constexpr float kDefaultFocusDistance = 15.0f;

	std::string debugName_ = "Camera";

	float baseFovY_ = kDefaultFovY;
	float fovY_ = kDefaultFovY;
	float aspectRatio_ = kDefaultAspectRatio;
	float nearClip_ = kDefaultNearClip;
	float farClip_ = kDefaultFarClip;
	bool autoAspectRatio_ = true;

	DebugCamera debugCamera_;

	bool isDebugCamera_ = false;
	float totalTime_ = 0.0f;

	Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource_;
	CameraForGPU* cameraData_ = nullptr;

	// === 演出機能用変数 ===
	// 1. カメラシェイク
	ShakeParams currentShakeParams_{};
	float shakeTimer_ = 0.0f;
	bool isShaking_ = false;
	Vector3 shakeOffset_{ 0.0f, 0.0f, 0.0f };
	Vector3 shakeRotationOffset_{ 0.0f, 0.0f, 0.0f };

	// 2. 動的 FOV 制御
	bool isFovTransitioning_ = false;
	float fovStart_ = kDefaultFovY;
	float fovTarget_ = kDefaultFovY;
	float fovTransitionDuration_ = 0.0f;
	float fovTransitionTimer_ = 0.0f;

	// FOV インパルス（瞬間ズーム＆自動復帰）
	float fovImpulse_ = 0.0f;
	float fovImpulseDuration_ = 0.0f;
	float fovImpulseTimer_ = 0.0f;

	// 3. レイヤーオフセット（演出用位置・回転の加算）
	Vector3 positionOffset_{ 0.0f, 0.0f, 0.0f };
	Vector3 rotationOffset_{ 0.0f, 0.0f, 0.0f };

	// 4. ドリーズーム
	bool isDollyZooming_ = false;
	Vector3 dollyFocusPoint_{ 0.0f, 0.0f, 0.0f };
	float dollyStartDistance_ = 0.0f;
	float dollyTargetDistance_ = 0.0f;
	float dollyDuration_ = 0.0f;
	float dollyTimer_ = 0.0f;
	float dollyTargetFrustumHeight_ = 0.0f;

public:
	Camera();
	~Camera();

	static void SetScreenSize(Vector2 screenSize);
	static Vector2 GetScreenSize();
	static const std::vector<Camera*>& GetInstances() { return s_instances_; }

	void SetDebugName(const std::string& name) { debugName_ = name; }
	const std::string& GetDebugName() const { return debugName_; }

	void ImGui();
	void DrawDebugDetails();

	void Initialize();
	void Update();
	Matrix4x4 GetViewMatrix() const { return viewMatrix_; }
	Matrix4x4 GetProjectionMatrix() const { return projectionMatrix_; }
	Matrix4x4 GetViewProjectionMatrix() const { return MultiplyMatrix4x4(viewMatrix_, projectionMatrix_); }

	// 基準 Transform（ゲームロジックが設定・取得する本来の位置）
	const Transform& GetTransform() const { return transform_; }
	Transform& GetTransformRef() { return transform_; }
	const Transform& GetGameTransform() const { return gameTransform_; }
	void SetTransform(Transform transform);

	// 演出（シェイク・オフセット）がすべて合成された最終描画 Transform
	const Transform& GetFinalTransform() const { return finalTransform_; }
	Vector3 GetFinalPosition() const { return finalTransform_.translate; }
	Vector3 GetFinalRotation() const { return finalTransform_.rotate; }

	void SetDebugCamera(bool isDebug) { isDebugCamera_ = isDebug; }
	bool GetDebugCamera() const { return isDebugCamera_; }
	void ResetDebugCameraToGameCamera();
	void ResetDebugCamera(const Transform& transform);
	DebugCamera& GetDebugCameraRef() { return debugCamera_; }

	void SetTarget(const Vector3& target) { debugCamera_.SetTarget(target); }
	Vector3 GetTarget() const { return debugCamera_.GetTarget(); }
	void Focus(const Vector3& target, float distance) {
		debugCamera_.Focus(target, distance);
		transform_.translate = debugCamera_.GetEye();
		viewMatrix_ = debugCamera_.GetViewMatrix();
	}
	void Focus(const Vector3& target) {
		Focus(target, kDefaultFocusDistance);
	}

	void SetAspectRatio(float aspectRatio) { aspectRatio_ = aspectRatio; autoAspectRatio_ = false; }
	float GetAspectRatio() const { return aspectRatio_; }
	void SetAutoAspectRatio(bool autoAspect) { autoAspectRatio_ = autoAspect; }
	bool GetAutoAspectRatio() const { return autoAspectRatio_; }
	
	ID3D12Resource* GetCameraResource() const { return cameraResource_.Get(); }

	float GetTotalTime() const { return totalTime_; }

	const Frustum& GetFrustum() const { return frustum_; }

	// =================================================================
	// カメラ演出機能 (Camera Effects)
	// =================================================================

	// --- 1. カメラシェイク (Camera Shake) ---
	/// <summary>
	/// カメラを振動させます。
	/// </summary>
	/// <param name="intensity">揺れの強さ (ワールド単位)</param>
	/// <param name="duration">継続時間 (秒)</param>
	/// <param name="frequency">周波数 (Hz, デフォルト25.0f)</param>
	void Shake(float intensity, float duration, float frequency = 25.0f);

	/// <summary>
	/// 詳細パラメータを指定してシェイクを開始します。
	/// </summary>
	void Shake(const ShakeParams& params);

	/// <summary>
	/// 方向付きシェイク (縦揺れ・着地・前方衝撃など)
	/// </summary>
	void ShakeDirectional(const Vector3& direction, float intensity, float duration, float frequency = 25.0f);

	/// <summary>
	/// シェイクを直ちに停止します。
	/// </summary>
	void StopShake();

	/// <summary>
	/// シェイク中かどうかを取得します。
	/// </summary>
	bool IsShaking() const { return isShaking_; }

	const Vector3& GetShakeOffset() const { return shakeOffset_; }
	const Vector3& GetShakeRotationOffset() const { return shakeRotationOffset_; }

	// --- 2. 画角制御 (FOV / Dynamic Zoom) ---
	/// <summary>
	/// FOVを即座に設定します。
	/// </summary>
	void SetFovY(float fovY);

	/// <summary>
	/// 現在のFOVを取得します。
	/// </summary>
	float GetFovY() const { return fovY_; }

	/// <summary>
	/// 基準FOVを設定します (リセット時の目標値となります)。
	/// </summary>
	void SetBaseFovY(float baseFovY);
	float GetBaseFovY() const { return baseFovY_; }

	/// <summary>
	/// 指定時間をかけて滑らかに指定FOVへ遷移します。
	/// </summary>
	void AnimateFov(float targetFov, float duration);

	/// <summary>
	/// ヒットズームなどの瞬間的なFOV衝撃を与え、自動的に減衰復帰させます。
	/// </summary>
	/// <param name="deltaFov">変化量 (例: -0.05fで瞬間ズームイン、+0.08fで瞬間広角)</param>
	/// <param name="duration">復帰までの時間 (秒)</param>
	void AddFovImpulse(float deltaFov, float duration);

	/// <summary>
	/// FOVを基準値(baseFovY_)へ滑らかに戻します。
	/// </summary>
	void ResetFov(float duration = 0.2f);

	// --- 3. 演出用オフセット (Layered Offsets) ---
	/// <summary>
	/// 演出用の位置オフセットを設定します (しゃがみ、肩越し、一時シフトなど)。
	/// </summary>
	void SetPositionOffset(const Vector3& offset) { positionOffset_ = offset; }
	const Vector3& GetPositionOffset() const { return positionOffset_; }

	/// <summary>
	/// 演出用の回転オフセットを設定します (レーン移動バンク傾き、リコイルなど)。
	/// </summary>
	void SetRotationOffset(const Vector3& rotOffset) { rotationOffset_ = rotOffset; }
	const Vector3& GetRotationOffset() const { return rotationOffset_; }

	/// <summary>
	/// 演出用オフセットをリセットします。
	/// </summary>
	void ClearOffsets() {
		positionOffset_ = { 0.0f, 0.0f, 0.0f };
		rotationOffset_ = { 0.0f, 0.0f, 0.0f };
	}

	// --- 4. スムーズ追従ヘルパー (Smooth Follow / Damping) ---
	/// <summary>
	/// 指数ダンピングを用いて目標位置へ滑らかに追従させます。
	/// </summary>
	void SmoothFollow(const Vector3& targetPosition, float smoothSpeed, float deltaTime);

	/// <summary>
	/// 最短経路補間を用いて目標回転へ滑らかに追従させます。
	/// </summary>
	void SmoothRotate(const Vector3& targetRotate, float smoothSpeed, float deltaTime);

	// --- 5. ドリーズーム (Dolly Zoom / Vertigo Effect) ---
	/// <summary>
	/// 対象被写体の画面上サイズを維持したまま、カメラが前進/後退して背景が歪む演出を開始します。
	/// </summary>
	/// <param name="focusPoint">注視対象のワールド座標</param>
	/// <param name="targetDistance">演出終了時の対象との目標距離</param>
	/// <param name="duration">演出時間 (秒)</param>
	void StartDollyZoom(const Vector3& focusPoint, float targetDistance, float duration);
	void StopDollyZoom() { isDollyZooming_ = false; }
	bool IsDollyZooming() const { return isDollyZooming_; }

private:
	void UpdateShake(float dt);
	void UpdateFov(float dt);
	void UpdateDollyZoom(float dt);

	Frustum frustum_{};
};


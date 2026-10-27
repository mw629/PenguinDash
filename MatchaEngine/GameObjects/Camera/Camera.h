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

	/// <summary>
	/// 画面解像度（スクリーンサイズ）を設定します。
	/// </summary>
	/// <param name="screenSize">幅と高さ (Vector2)</param>
	static void SetScreenSize(Vector2 screenSize);

	/// <summary>
	/// 画面解像度（スクリーンサイズ）を取得します。
	/// </summary>
	/// <returns>画面幅と高さ (Vector2)</returns>
	static Vector2 GetScreenSize();

	/// <summary>
	/// 生成されている全カメラインスタンスのリストを取得します。
	/// </summary>
	static const std::vector<Camera*>& GetInstances() { return s_instances_; }

	/// <summary>
	/// エディタ表示用のデバッグ名を設定します。
	/// </summary>
	void SetDebugName(const std::string& name) { debugName_ = name; }

	/// <summary>
	/// エディタ表示用のデバッグ名を取得します。
	/// </summary>
	const std::string& GetDebugName() const { return debugName_; }

	/// <summary>
	/// カメラ調整用のImGuiデバッグUIを描画します。
	/// </summary>
	void ImGui();

	/// <summary>
	/// カメラの詳細デバッグ情報をImGuiに描画します。
	/// </summary>
	void DrawDebugDetails();

	/// <summary>
	/// カメラリソース（定数バッファ等）を初期化します。
	/// </summary>
	void Initialize();

	/// <summary>
	/// カメラのシェイク・FOVアニメーション・ビュープロジェクション行列を毎フレーム更新します。
	/// </summary>
	void Update();

	/// <summary>
	/// ビュー行列を取得します。
	/// </summary>
	Matrix4x4 GetViewMatrix() const { return viewMatrix_; }

	/// <summary>
	/// プロジェクション行列を取得します。
	/// </summary>
	Matrix4x4 GetProjectionMatrix() const { return projectionMatrix_; }

	/// <summary>
	/// ビュー×プロジェクション合成行列を取得します。
	/// </summary>
	Matrix4x4 GetViewProjectionMatrix() const { return MultiplyMatrix4x4(viewMatrix_, projectionMatrix_); }

	/// <summary>
	/// 基準Transform（ゲームロジックが設定・取得する本来の姿勢）を取得します。
	/// </summary>
	const Transform& GetTransform() const { return transform_; }

	/// <summary>
	/// 基準Transformへの参照を取得します。
	/// </summary>
	Transform& GetTransformRef() { return transform_; }

	/// <summary>
	/// ゲーム本来のTransformを取得します。
	/// </summary>
	const Transform& GetGameTransform() const { return gameTransform_; }

	/// <summary>
	/// カメラの基準Transformを設定します。
	/// </summary>
	/// <param name="transform">カメラの姿勢</param>
	void SetTransform(Transform transform);

	/// <summary>
	/// 演出（シェイク・オフセット）がすべて合成された最終描画Transformを取得します。
	/// </summary>
	const Transform& GetFinalTransform() const { return finalTransform_; }

	/// <summary>
	/// 最終描画位置を取得します。
	/// </summary>
	Vector3 GetFinalPosition() const { return finalTransform_.translate; }

	/// <summary>
	/// 最終描画回転を取得します。
	/// </summary>
	Vector3 GetFinalRotation() const { return finalTransform_.rotate; }

	/// <summary>
	/// デバッグカメラモードの有効/無効を切り替えます。
	/// </summary>
	/// <param name="isDebug">trueならデバッグカメラ有効</param>
	void SetDebugCamera(bool isDebug) {
		if (isDebug && !isDebugCamera_) {
			ResetDebugCameraToGameCamera();
		} else if (!isDebug && isDebugCamera_) {
			transform_ = gameTransform_;
			finalTransform_ = gameTransform_;
		}
		isDebugCamera_ = isDebug;
	}

	/// <summary>
	/// デバッグカメラモード中かどうかを取得します。
	/// </summary>
	bool GetDebugCamera() const { return isDebugCamera_; }

	/// <summary>
	/// デバッグカメラの姿勢を現在のゲームカメラの位置・注視点へリセットします。
	/// </summary>
	void ResetDebugCameraToGameCamera();

	/// <summary>
	/// 指定したTransformに基づいてデバッグカメラの姿勢をリセットします。
	/// </summary>
	/// <param name="transform">リセット基準となるTransform</param>
	void ResetDebugCamera(const Transform& transform);

	/// <summary>
	/// デバッグカメラインスタンスへの参照を取得します。
	/// </summary>
	DebugCamera& GetDebugCameraRef() { return debugCamera_; }

	/// <summary>
	/// デバッグカメラの注視点ターゲットを設定します。
	/// </summary>
	void SetTarget(const Vector3& target) { debugCamera_.SetTarget(target); }

	/// <summary>
	/// デバッグカメラの注視点ターゲットを取得します。
	/// </summary>
	Vector3 GetTarget() const { return debugCamera_.GetTarget(); }

	/// <summary>
	/// 指定ターゲット位置にカメラをフォーカス（注視）させます。
	/// </summary>
	/// <param name="target">注視点座標</param>
	/// <param name="distance">カメラと注視点の距離</param>
	void Focus(const Vector3& target, float distance) {
		debugCamera_.Focus(target, distance);
		transform_.translate = debugCamera_.GetEye();
		viewMatrix_ = debugCamera_.GetViewMatrix();
	}

	/// <summary>
	/// デフォルト距離で指定ターゲット位置にカメラをフォーカスさせます。
	/// </summary>
	/// <param name="target">注視点座標</param>
	void Focus(const Vector3& target) {
		Focus(target, kDefaultFocusDistance);
	}

	/// <summary>
	/// アスペクト比を手動設定します。
	/// </summary>
	/// <param name="aspectRatio">設定するアスペクト比（幅/高さ）</param>
	void SetAspectRatio(float aspectRatio) { aspectRatio_ = aspectRatio; autoAspectRatio_ = false; }

	/// <summary>
	/// アスペクト比を取得します。
	/// </summary>
	float GetAspectRatio() const { return aspectRatio_; }

	/// <summary>
	/// スクリーンサイズに応じた自動アスペクト比計算の有効/無効を設定します。
	/// </summary>
	void SetAutoAspectRatio(bool autoAspect) { autoAspectRatio_ = autoAspect; }

	/// <summary>
	/// 自動アスペクト比計算が有効かどうかを取得します。
	/// </summary>
	bool GetAutoAspectRatio() const { return autoAspectRatio_; }
	
	/// <summary>
	/// GPU転送用のカメラ定数バッファリソースを取得します。
	/// </summary>
	ID3D12Resource* GetCameraResource() const { return cameraResource_.Get(); }

	/// <summary>
	/// 累積動作時間を取得します。
	/// </summary>
	float GetTotalTime() const { return totalTime_; }

	/// <summary>
	/// 視錐台（カリング用Frustum）を取得します。
	/// </summary>
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

	// =================================================================
	// 画面・ワールド座標変換 (Screen to World)
	// =================================================================
	/// <summary>
	/// モニター上の画面座標（ピクセル座標）から、カメラ画面の手前（近クリップ面）のワールド座標を取得します。
	/// </summary>
	/// <param name="screenPos">モニター上のピクセル座標 (Vector2)</param>
	/// <returns>画面手前（近クリップ面）のワールド座標 (Vector3)</returns>
	Vector3 GetScreenNearPosition(const Vector2& screenPos) const;

	/// <summary>
	/// モニター上の画面座標（ピクセル座標）とカメラ前方への距離を指定して、ワールド座標を取得します。
	/// </summary>
	/// <param name="screenPos">モニター上のピクセル座標 (Vector2)</param>
	/// <param name="distance">カメラからの距離（ワールド単位）</param>
	/// <returns>指定距離にあるワールド座標 (Vector3)</returns>
	Vector3 GetScreenWorldPosition(const Vector2& screenPos, float distance) const;

private:
	void UpdateShake(float dt);
	void UpdateFov(float dt);
	void UpdateDollyZoom(float dt);

	Frustum frustum_{};
};


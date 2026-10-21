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

  /// <summary>
  /// カメラの初期化
  /// </summary>
  void Initialize();

  /// <summary>
  /// カメラトランスフォームおよび演出パラメータのリセット
  /// </summary>
  /// <param name="defaultTransform">初期カメラ姿勢</param>
  void Reset(const Transform &defaultTransform);

  /// <summary>
  /// カメラのリセット（デフォルト位置）
  /// </summary>
  void Reset() { Reset(kDefaultCameraTransform); }

  /// <summary>
  /// カメラ行列の毎フレーム更新
  /// </summary>
  void Update();

  /// <summary>
  /// ゲームプレイ時のプレイヤー追従カメラ更新（滑らかな追従・バンク回転）
  /// </summary>
  /// <param name="player">プレイヤーオブジェクト</param>
  /// <param name="dt">デルタタイム</param>
  void UpdateGameplayCamera(Player *player, float dt);

  /// <summary>
  /// カメラ視点トランジションの開始（三人称・側面・ボス視点等）
  /// </summary>
  /// <param name="targetTransform">目標カメラ姿勢</param>
  /// <param name="laneCount">目標レーン数</param>
  /// <param name="stageSettings">ステージ設定ポインタ</param>
  void StartTransition(const Transform &targetTransform, int laneCount,
                       StageSettings *stageSettings);

  /// <summary>
  /// カメラ視点トランジションの更新処理
  /// </summary>
  /// <param name="stageSettings">ステージ設定ポインタ</param>
  /// <param name="onFinished">完了時コールバック</param>
  void UpdateTransition(StageSettings *stageSettings,
                        std::function<void()> onFinished = nullptr);

  /// <summary>
  /// 一人称視点（FPV）時のカメラ位置同期
  /// </summary>
  /// <param name="player">プレイヤーオブジェクト</param>
  void UpdateFirstPersonCamera(Player *player);

  /// <summary>
  /// 一人称視点と三人称視点の切り替えトグル
  /// </summary>
  /// <param name="player">プレイヤーオブジェクト</param>
  void ToggleFirstPerson(Player *player);

  /// <summary>
  /// カメラトランスフォームの直接設定
  /// </summary>
  /// <param name="transform">設定する姿勢</param>
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
  /// <summary>
  /// 画面全方向のカメラシェイク（振動）を発動
  /// </summary>
  /// <param name="intensity">振動の強さ</param>
  /// <param name="duration">継続時間（秒）</param>
  /// <param name="frequency">振動周波数（Hz）</param>
  void Shake(float intensity, float duration, float frequency = 25.0f);

  /// <summary>
  /// 方向指定カメラシェイクを発動
  /// </summary>
  /// <param name="direction">振動方向ベクトル</param>
  /// <param name="intensity">振動の強さ</param>
  /// <param name="duration">継続時間（秒）</param>
  /// <param name="frequency">振動周波数（Hz）</param>
  void ShakeDirectional(const Vector3 &direction, float intensity, float duration, float frequency = 25.0f);

  /// <summary>
  /// 現在実行中のカメラシェイクを停止
  /// </summary>
  void StopShake();

  /// <summary>
  /// 激突・被弾時の衝撃シェイク演出
  /// </summary>
  void ShakeImpact();

  /// <summary>
  /// 敵撃破時のシェイク演出
  /// </summary>
  void ShakeDefeat();

  /// <summary>
  /// ボス撃破時の大シェイク演出
  /// </summary>
  void ShakeBossDefeat();

  /// <summary>
  /// 着地時の軽いシェイク演出
  /// </summary>
  void ShakeLand();

  /// <summary>
  /// カウンター反射時のシェイク演出
  /// </summary>
  void ShakeReflect();

  /// <summary>
  /// 視野角（FOV）のアニメーション変更
  /// </summary>
  /// <param name="targetFov">目標FOV（ラジアン）</param>
  /// <param name="duration">遷移時間（秒）</param>
  void AnimateFov(float targetFov, float duration);

  /// <summary>
  /// 視野角にインパルス衝撃を付与
  /// </summary>
  /// <param name="deltaFov">変化量（ラジアン）</param>
  /// <param name="duration">復帰時間（秒）</param>
  void AddFovImpulse(float deltaFov, float duration);

  /// <summary>
  /// 視野角をデフォルト値へリセット
  /// </summary>
  /// <param name="duration">復帰時間（秒）</param>
  void ResetFov(float duration = 0.2f);

  void SetPositionOffset(const Vector3 &offset);
  void SetRotationOffset(const Vector3 &rotOffset);
  void ClearOffsets();

  /// <summary>
  /// ドリーズーム（めまい効果）演出を開始
  /// </summary>
  void StartDollyZoom(const Vector3 &focusPoint, float targetDistance, float duration);

  /// <summary>
  /// ドリーズーム演出を停止
  /// </summary>
  void StopDollyZoom();

  /// <summary>
  /// ImGuiによるカメラパラメータ設定UI
  /// </summary>
  void ImGui(Player *player, std::function<void(int)> onPresetSelected);
};

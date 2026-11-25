#pragma once

#include <Engine.h>
#include <functional>
#include <memory>

#include "GameObject.h"
#include "State/IPlayerState.h"
#include <CharacterAnimator.h>

/// <summary>
/// プレイヤークラス（主人公ペンギン）
/// 3レーン（可変レーン）ランゲームにおけるプレイヤーの制御を担当します。
/// アクション状態（Run / Jump / Roll / Hit / ForcedCenter）は
/// IPlayerState ベースの State パターンへ委譲して管理します。
/// </summary>
class Player : public GameObject {
public:
  // ロール（スライディング）時の高さオフセット（ステートと描画補正で共有）
  static constexpr float kDefaultRollingHeightOffset = 0.5f;

private:
  enum MoveDirection { Left, Right, None };

  std::unique_ptr<CharacterAnimator> model_ =
      std::make_unique<CharacterAnimator>();
  bool lightingEnabled_ = true;

  // レーン設定定数
  static constexpr int kDefaultMinLane = -1;
  static constexpr int kDefaultMaxLane = 1;
  static constexpr float kDefaultLaneWidth = 4.0f;
  static constexpr float kDefaultLaneChangeSpeed = 0.2f;

  // アクション物理パラメータ定数
  static constexpr float kDefaultGravity = 0.014f;
  static constexpr float kDefaultJumpPower = 0.21f;
  static constexpr float kDefaultBaseHeight = 3.0f;

  // ロール（スライディング）パラメータ定数
  static constexpr float kDefaultRollDuration = 50.0f;
  static constexpr float kDefaultRollTransitionDuration = 0.06f;
  static constexpr float kDefaultWalkTransitionDuration = 0.08f;

  // コライダーサイズ定数
  static constexpr float kDefaultColliderWidth = 0.8f;
  static constexpr float kDefaultColliderHeightStand = 1.5f;
  static constexpr float kDefaultColliderHeightRoll = 0.5f;
  static constexpr float kDefaultColliderDepth = 0.8f;

  // モデルのスケール定数
  static constexpr Vector3 kDefaultModelScale = {140.0f, 140.0f, 140.0f};
  static constexpr Vector3 kDefaultModelTranslate = {0.0f, -1.0f, 0.0f};
  static constexpr float kAnimationSpeedFactor = 0.01f;

  // 色定数
  static constexpr Vector4 kBarrierColor = {0.0f, 1.0f, 1.0f, 1.0f};
  static constexpr Vector4 kDefaultPlayerColor = {1.0f, 1.0f, 1.0f, 1.0f};
  static constexpr float kDefaultSpeedMultiplier = 1.0f;

  // 1フレーム内の最大連続ステート遷移回数（無限ループ防止）
  static constexpr int kMaxStateTransitionsPerFrame = 4;

private:
  // ペンギンモデルのスケール・オフセット（GLTFモデルの0.01スケールを等身大に補正）
  Transform modelOffset_{
      kDefaultModelScale, {0.0f, 0.0f, 0.0f}, kDefaultModelTranslate};

  // レーン移動のための変数
  int laneIndex_ = 0;       // 現在のレーン位置
  int targetLaneIndex_ = 0; // 目標のレーン位置

  float laneChangeSpeed_ = kDefaultLaneChangeSpeed;    // レーン移動の速度
  float lerpTime_ = 0.0f;                              // 補間用タイマー
  float startX_ = 0.0f;                                // 移動開始時のX座標
  MoveDirection moveDirection_ = MoveDirection::None;  // 移動方向

  // レーン設定
  int minLane_ = kDefaultMinLane;
  int maxLane_ = kDefaultMaxLane;
  float laneWidth_ = kDefaultLaneWidth;

  // アクションステート管理
  std::unique_ptr<IPlayerState> currentState_;
  float currentSpeedMultiplier_ = 1.0f;

  // アクション設定パラメータ（ImGui調整対象）
  float gravity_ = kDefaultGravity;
  float jumpPower_ = kDefaultJumpPower;
  float baseHeight_ = kDefaultBaseHeight; // 地面の高さ（Y座標）

  float rollDuration_ = kDefaultRollDuration; // 転がりの継続フレーム数
  bool keepRolling_ = false;                  // 強制的にしゃがみを維持するフラグ
  float rollTransitionDuration_ = kDefaultRollTransitionDuration;
  float walkTransitionDuration_ = kDefaultWalkTransitionDuration;

  // 各アクションの硬直（クールタイム）用変数
  float laneChangeRecovery_ = 0.0f;   // レーン移動終了後の硬直フレーム数
  float jumpRecovery_ = 0.0f;         // ジャンプ着地後の硬直フレーム数
  float rollRecovery_ = 0.0f;         // 転がり終了後の硬直フレーム数
  float currentRecoveryTimer_ = 0.0f; // 現在の硬直タイマー

  // カメラ激突コールバック（シェイク等）
  std::function<void()> onCameraHitCallback_ = nullptr;

  // 腹滑り雪玉化 描画用モデル・状態
  std::unique_ptr<Model> snowballModel_ = std::make_unique<Model>();
  Transform snowballTransform_{};
  bool isSnowballActive_ = false;
  bool isSnowballHidingPenguin_ = false;
  float snowballScale_ = 0.0f;

  // バリアアイテム取得時の状態
  bool hasBarrier_ = false;

  // 操作反転（ボス戦等でカメラが180度反転した際に見た目通りに動かすフラグ）
  bool isInvertedControls_ = false;

  // オートパイロット（タイトル画面での自動運転モード）
  bool isAutoPilot_ = false;

  // プレイヤー入力有効フラグ（カウントダウン中などの操作ロック用）
  bool isInputEnabled_ = true;

  // 表示・非表示（一人称視点時の自キャラメッシュ非表示用など）
  bool isVisible_ = true;

  // 先行入力バッファパラメータ（操作感向上: 約0.16秒）
  static constexpr float kDefaultInputBufferFrames = 10.0f;
  float bufferedJumpTimer_ = 0.0f;
  float bufferedRollTimer_ = 0.0f;
  int bufferedLaneDirection_ = 0; // -1: 左, +1: 右, 0: なし
  float bufferedLaneTimer_ = 0.0f;

  // 着地コールバック（演出・カメラシェイク用）
  std::function<void()> onLandCallback_ = nullptr;

  void UpdateDrawTransform(float speedMultiplier);
  void UpdateDrawTransform() { UpdateDrawTransform(kDefaultSpeedMultiplier); }
  void StepActionState(float speedMultiplier);

public:
  Player();
  ~Player();

  void Initialize(ModelData modelData);
  void Reset();

  void Update(Matrix4x4 view, float speedMultiplier) override;
  void Update(Matrix4x4 view) { Update(view, kDefaultSpeedMultiplier); }

  void ProcessLaneInput();
  void UpdateLaneMovement(float speedMultiplier);

  void SetLaneLimits(int minLane, int maxLane, float laneWidth) {
    minLane_ = minLane;
    maxLane_ = maxLane;
    laneWidth_ = laneWidth;
  }

  void Draw(class Draw &draw) override;
  void ImGuiInnerComponents() override;

  bool HasMaterial() const override {
    return (model_ && model_->GetComponent<MaterialComponent>() != nullptr) ||
           GameObject::HasMaterial();
  }

  // ステート関連
  PlayerActionType GetActionType() const {
    return currentState_ ? currentState_->GetType() : PlayerActionType::Run;
  }
  IPlayerState *GetCurrentActionState() const { return currentState_.get(); }
  float GetCurrentSpeedMultiplier() const { return currentSpeedMultiplier_; }
  void ChangeActionState(std::unique_ptr<IPlayerState> newState);
  void ChangeActionState(PlayerActionType nextType);

  bool GetIsRolling() const { return GetActionType() == PlayerActionType::Roll; }
  void SetKeepRolling(bool keep) { keepRolling_ = keep; }
  bool GetIsJumping() const { return GetActionType() == PlayerActionType::Jump; }
  bool GetIsHit() const { return GetActionType() == PlayerActionType::Hit; }
  bool GetIsForcedCentering() const {
    return GetActionType() == PlayerActionType::ForcedCenter;
  }

  float GetJumpPower() const { return jumpPower_; }
  void SetJumpPower(float power) { jumpPower_ = power; }
  float GetGravity() const { return gravity_; }
  void SetGravity(float gravity) { gravity_ = gravity; }
  float GetLaneChangeSpeed() const { return laneChangeSpeed_; }
  void SetLaneChangeSpeed(float speed) { laneChangeSpeed_ = speed; }
  float GetRollDuration() const { return rollDuration_; }
  void SetRollDuration(float duration) { rollDuration_ = duration; }
  float GetBaseHeight() const { return baseHeight_; }
  void SetBaseHeight(float height) { baseHeight_ = height; }

  bool GetHasBarrier() const { return hasBarrier_; }
  void SetHasBarrier(bool hasBarrier);

  void SetInvertedControls(bool inv) { isInvertedControls_ = inv; }
  bool GetInvertedControls() const { return isInvertedControls_; }

  void SetAutoPilot(bool autoPilot) { isAutoPilot_ = autoPilot; }
  bool GetAutoPilot() const { return isAutoPilot_; }

  void SetInputEnabled(bool enable) { isInputEnabled_ = enable; }
  bool GetInputEnabled() const { return isInputEnabled_; }

  void SetVisible(bool visible) { isVisible_ = visible; }
  bool GetVisible() const { return isVisible_; }

  bool TriggerJump();
  bool TriggerRoll();
  bool TriggerMoveLeft();
  bool TriggerMoveRight();

  int GetLaneIndex() const { return laneIndex_; }
  int GetTargetLaneIndex() const { return targetLaneIndex_; }
  bool IsChangingLane() const { return laneIndex_ != targetLaneIndex_; }
  bool CanAct() const {
    return isInputEnabled_ && (currentRecoveryTimer_ <= 0.0f) &&
           (!currentState_ || currentState_->CanAct());
  }

  void SetCurrentRecoveryTimer(float timer) { currentRecoveryTimer_ = timer; }

  // 先行入力バッファ
  void UpdateInputBuffer(float speedMultiplier);
  bool HasBufferedJump() const { return bufferedJumpTimer_ > 0.0f; }
  bool HasBufferedRoll() const { return bufferedRollTimer_ > 0.0f; }
  bool HasBufferedLane() const {
    return bufferedLaneTimer_ > 0.0f && bufferedLaneDirection_ != 0;
  }
  int GetBufferedLaneDirection() const { return bufferedLaneDirection_; }

  bool ConsumeBufferedJump() {
    if (bufferedJumpTimer_ > 0.0f) {
      bufferedJumpTimer_ = 0.0f;
      return true;
    }
    return false;
  }

  bool ConsumeBufferedRoll() {
    if (bufferedRollTimer_ > 0.0f) {
      bufferedRollTimer_ = 0.0f;
      return true;
    }
    return false;
  }

  int ConsumeBufferedLane() {
    if (bufferedLaneTimer_ > 0.0f && bufferedLaneDirection_ != 0) {
      int dir = bufferedLaneDirection_;
      bufferedLaneTimer_ = 0.0f;
      bufferedLaneDirection_ = 0;
      return dir;
    }
    return 0;
  }

  void ClearInputBuffer() {
    bufferedJumpTimer_ = 0.0f;
    bufferedRollTimer_ = 0.0f;
    bufferedLaneTimer_ = 0.0f;
    bufferedLaneDirection_ = 0;
  }

  // 演出コールバック
  void SetOnLandCallback(std::function<void()> callback) {
    onLandCallback_ = callback;
  }
  const std::function<void()> &GetOnLandCallback() const {
    return onLandCallback_;
  }

  void SetOnCameraHitCallback(std::function<void()> callback) {
    onCameraHitCallback_ = callback;
  }
  void ExecuteCameraHitCallback() {
    if (onCameraHitCallback_) {
      onCameraHitCallback_();
    }
  }

  // 被弾・強制中央移動
  void OnHit(bool isTrip = false, bool isLargeObstacle = false,
             const Vector3 &cameraPos = {0.0f, 3.0f, -12.5f});
  bool IsHitAnimationFinished() const {
    return currentState_ && currentState_->IsHitAnimationFinished();
  }
  bool GetIsTrip() const {
    return currentState_ && currentState_->IsTripHit();
  }

  AABB GetColliderAABB() const;
  AABB GetStandingColliderAABB() const;

  void StartForceToCenter(float duration);

  CharacterAnimator *GetModel() { return model_.get(); }
  bool GetKeepRolling() const { return keepRolling_; }
  float GetRollTransitionDuration() const { return rollTransitionDuration_; }
  float GetWalkTransitionDuration() const { return walkTransitionDuration_; }
  float GetJumpRecovery() const { return jumpRecovery_; }
  float GetRollRecovery() const { return rollRecovery_; }
  float GetLaneChangeRecovery() const { return laneChangeRecovery_; }

  // 腹滑り雪玉化描画アクセサ
  bool GetIsSnowballActive() const { return isSnowballActive_; }
  void SetSnowballActive(bool active) { isSnowballActive_ = active; }
  bool GetIsSnowballHidingPenguin() const { return isSnowballHidingPenguin_; }
  void SetSnowballHidingPenguin(bool hide) { isSnowballHidingPenguin_ = hide; }
  float GetSnowballScale() const { return snowballScale_; }
  void SetSnowballScale(float scale) { snowballScale_ = scale; }
  const Transform &GetSnowballTransform() const { return snowballTransform_; }
  void SetSnowballTransform(const Transform &trans) {
    snowballTransform_ = trans;
  }
  std::shared_ptr<SphereColliderComponent> GetSnowballCollider() const {
    return snowballModel_
               ? snowballModel_->GetComponent<SphereColliderComponent>()
               : nullptr;
  }
  const Transform &GetModelOffset() const { return modelOffset_; }

  void SetLaneIndex(int lane) { laneIndex_ = lane; }
  void SetTargetLaneIndex(int lane) { targetLaneIndex_ = lane; }
  int GetMinLane() const { return minLane_; }
  int GetMaxLane() const { return maxLane_; }
  float GetLaneWidth() const { return laneWidth_; }

  void SetLighting(bool enabled);
  bool GetLighting() const { return lightingEnabled_; }
};

#pragma once

#include <Engine.h>
#include <memory>
#include <functional>

#include "GameObject.h"
#include <CharacterAnimator.h>

enum class PlayerActionType {
  Run,
  Jump,
  Roll,
  Hit,
  ForcedCenter
};

class Player;

class IPlayerState {
public:
  virtual ~IPlayerState() = default;
  virtual PlayerActionType GetType() const = 0;
  virtual void Enter(Player &/*player*/) {}
  virtual void Update(Player &player, float speedMultiplier) = 0;
  virtual void Exit(Player &/*player*/) {}
  virtual bool CanAct() const { return true; }
};

class PlayerRunState : public IPlayerState {
public:
  PlayerActionType GetType() const override { return PlayerActionType::Run; }
  void Enter(Player &player) override;
  void Update(Player &player, float speedMultiplier) override;
};

class PlayerJumpState : public IPlayerState {
public:
  PlayerActionType GetType() const override { return PlayerActionType::Jump; }
  void Enter(Player &player) override;
  void Update(Player &player, float speedMultiplier) override;
};

class PlayerRollState : public IPlayerState {
public:
  PlayerActionType GetType() const override { return PlayerActionType::Roll; }
  void Enter(Player &player) override;
  void Update(Player &player, float speedMultiplier) override;
  void Exit(Player &player) override;
};

class PlayerHitState : public IPlayerState {
public:
  PlayerActionType GetType() const override { return PlayerActionType::Hit; }
  void Enter(Player &player) override;
  void Update(Player &player, float speedMultiplier) override;
  void Exit(Player &player) override;
  bool CanAct() const override { return false; }
};

class PlayerForcedCenterState : public IPlayerState {
public:
  PlayerActionType GetType() const override { return PlayerActionType::ForcedCenter; }
  void Enter(Player &player) override;
  void Update(Player &player, float speedMultiplier) override;
  bool CanAct() const override { return false; }
};

class Player : public GameObject {
  friend class PlayerRunState;
  friend class PlayerJumpState;
  friend class PlayerRollState;
  friend class PlayerHitState;
  friend class PlayerForcedCenterState;

private:
  enum MoveDirection { Left, Right, None };

  std::unique_ptr<CharacterAnimator> model_ =
      std::make_unique<CharacterAnimator>();

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
  static constexpr float kDefaultRollDuration = 30.0f;
  static constexpr float kDefaultRollTransitionDuration = 0.06f;
  static constexpr float kDefaultWalkTransitionDuration = 0.08f;
  static constexpr float kDefaultRollingHeightOffset = 0.5f;

  // コライダーサイズ定数
  static constexpr float kDefaultColliderWidth = 0.8f;
  static constexpr float kDefaultColliderHeightStand = 1.5f;
  static constexpr float kDefaultColliderHeightRoll = 0.5f;
  static constexpr float kDefaultColliderDepth = 0.8f;

  // ヒット・中央復帰パラメータ定数
  static constexpr float kDefaultHitDuration = 90.0f;
  static constexpr float kDefaultForcedCenterDuration = 30.0f;

  // モデル・アックスのスケール定数
  static constexpr Vector3 kDefaultModelScale = {140.0f, 140.0f, 140.0f};
  static constexpr Vector3 kDefaultModelTranslate = {0.0f, -1.0f, 0.0f};
  static constexpr Vector3 kDefaultAxeScale = {100.0f, 100.0f, 100.0f};
  static constexpr float kAnimationSpeedFactor = 0.01f;

  // 色定数
  static constexpr Vector4 kBarrierColor = {0.0f, 1.0f, 1.0f, 1.0f};
  static constexpr Vector4 kDefaultPlayerColor = {1.0f, 1.0f, 1.0f, 1.0f};

  // ヒット演出物理定数
  static constexpr float kKnockbackRandXScale = 0.1f;
  static constexpr float kKnockbackRandCenterOffset = 0.5f;
  static constexpr Vector3 kTripKnockbackVelocity = {0.0f, 0.3f, 0.8f};
  static constexpr Vector3 kHitKnockbackVelocity = {0.0f, 0.4f, -0.6f};
  static constexpr float kHitGravityMultiplier = 2.0f;
  static constexpr float kTripRotationSpeed = 0.2f;
  static constexpr float kHitRotationSpeed = 0.1f;
  static constexpr float kHitLandingFriction = 0.8f;
  static constexpr float kDefaultSpeedMultiplier = 1.0f;

private:
  std::unique_ptr<Model> axe_ = std::make_unique<Model>();
  Transform axeOffset_{kDefaultAxeScale,
                       {
                           0.0f,
                           0.0f,
                           0.0f,
                       },
                       {0.0f, 0.0f, 0.0f}};
  bool isDrawAxe_ = false;

  // ペンギンモデルのスケール・オフセット（GLTFモデルの0.01スケールを等身大に補正）
  Transform modelOffset_{
      kDefaultModelScale, {0.0f, 0.0f, 0.0f}, kDefaultModelTranslate};

  // レーン移動のための変数
  int laneIndex_ = 0;       // 現在のレーン位置
  int targetLaneIndex_ = 0; // 目標のレーン位置

  float laneChangeSpeed_ = kDefaultLaneChangeSpeed;    // レーン移動の速度
  float lerpTime_ = 0.0f;                             // 補間用タイマー
  float startX_ = 0.0f;                               // 移動開始時のX座標
  MoveDirection moveDirection_ = MoveDirection::None; // 移動方向

  // レーン設定
  int minLane_ = kDefaultMinLane;
  int maxLane_ = kDefaultMaxLane;
  float laneWidth_ = kDefaultLaneWidth;

  // アクションステート管理
  std::unique_ptr<IPlayerState> currentState_;
  float currentSpeedMultiplier_ = 1.0f;

  // アクション用の物理・タイマー変数
  float velocityY_ = 0.0f;
  float gravity_ = kDefaultGravity;
  float jumpPower_ = kDefaultJumpPower;
  float baseHeight_ = kDefaultBaseHeight; // 地面の高さ（Y座標）

  float rollTimer_ = 0.0f;
  float rollDuration_ = kDefaultRollDuration; // 転がりの継続フレーム数（約0.5秒）
  bool keepRolling_ = false;   // 強制的にしゃがみを維持するフラグ
  float rollTransitionDuration_ =
      kDefaultRollTransitionDuration; // しゃがみ（sneakWalk）への遷移ブレンド秒数（約3〜4フレーム）
  float walkTransitionDuration_ =
      kDefaultWalkTransitionDuration; // 立ち上がり（walk）への遷移ブレンド秒数（約5フレーム）

  // 各アクションの硬直（クールタイム）用変数
  float laneChangeRecovery_ = 0.0f;   // レーン移動終了後の硬直フレーム数
  float jumpRecovery_ = 0.0f;         // ジャンプ着地後の硬直フレーム数
  float rollRecovery_ = 0.0f;         // 転がり終了後の硬直フレーム数
  float currentRecoveryTimer_ = 0.0f; // 現在の硬直タイマー

  // ヒット時の演出用変数
  bool isTrip_ = false;
  float hitTimer_ = 0.0f;
  float hitDuration_ = kDefaultHitDuration; // ノックバックにかかるフレーム数
  Vector3 knockbackVelocity_{0.0f, 0.0f, 0.0f}; // ノックバック速度

  // 強制中央移動用の変数
  float forcedCenterTimer_ = 0.0f;
  float forcedCenterDuration_ = kDefaultForcedCenterDuration; // 中央に到達するまでのフレーム数
  float forcedCenterStartX_ = 0.0f;

  // バリアアイテム取得時の状態
  bool hasBarrier_ = false;

  // 操作反転（ボス戦等でカメラが180度反転した際に画面の見た目通りに動くようにするフラグ）
  bool isInvertedControls_ = false;

  // オートパイロット（タイトル画面での自動運転モード）
  bool isAutoPilot_ = false;

  // 表示・非表示（一人称視点時の自キャラメッシュ非表示用など）
  bool isVisible_ = true;

  // 先行入力バッファパラメータ (操作感向上: 約0.16秒)
  static constexpr float kDefaultInputBufferFrames = 10.0f;
  float bufferedJumpTimer_ = 0.0f;
  float bufferedRollTimer_ = 0.0f;
  int bufferedLaneDirection_ = 0; // -1: 左, +1: 右, 0: なし
  float bufferedLaneTimer_ = 0.0f;

  // 着地コールバック (演出・カメラシェイク用)
  std::function<void()> onLandCallback_ = nullptr;

  void UpdateDrawTransform(float speedMultiplier);
  void UpdateDrawTransform() { UpdateDrawTransform(kDefaultSpeedMultiplier); }

public:
  Player();
  ~Player();

  void Initialize(ModelData modelData);
  void Reset();

  void Update(Matrix4x4 view, float speedMultiplier) override;
  void Update(Matrix4x4 view) { Update(view, kDefaultSpeedMultiplier); }

  // レーン移動ロジック
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
    return model_ && model_->GetComponent<MaterialComponent>() != nullptr ||
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
  bool GetIsForcedCentering() const { return GetActionType() == PlayerActionType::ForcedCenter; }

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

  // オートパイロット設定
  void SetAutoPilot(bool autoPilot) { isAutoPilot_ = autoPilot; }
  bool GetAutoPilot() const { return isAutoPilot_; }

  // 表示設定
  void SetVisible(bool visible) { isVisible_ = visible; }
  bool GetVisible() const { return isVisible_; }

  // 外部からのアクション発動（オートパイロット用）
  bool TriggerJump();
  bool TriggerRoll();
  bool TriggerMoveLeft();
  bool TriggerMoveRight();

  int GetLaneIndex() const { return laneIndex_; }
  int GetTargetLaneIndex() const { return targetLaneIndex_; }
  bool IsChangingLane() const { return laneIndex_ != targetLaneIndex_; }
  bool CanAct() const {
    return (currentRecoveryTimer_ <= 0.0f) && (!currentState_ || currentState_->CanAct());
  }

  // 先行入力バッファ (操作感向上用)
  void UpdateInputBuffer(float speedMultiplier);
  bool HasBufferedJump() const { return bufferedJumpTimer_ > 0.0f; }
  bool HasBufferedRoll() const { return bufferedRollTimer_ > 0.0f; }
  bool HasBufferedLane() const { return bufferedLaneTimer_ > 0.0f && bufferedLaneDirection_ != 0; }
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

  // 着地演出コールバック
  void SetOnLandCallback(std::function<void()> callback) { onLandCallback_ = callback; }
  const std::function<void()>& GetOnLandCallback() const { return onLandCallback_; }

  // ヒット演出用
  void OnHit(bool isTrip = false);
  bool IsHitAnimationFinished() const;

  // コライダー取得
  AABB GetColliderAABB() const;
  AABB GetStandingColliderAABB() const;

  // 狭まる区間で強制的に中央へ寄せる
  void StartForceToCenter(float duration);

  // ステートクラス向けのアクセサ
  Transform &GetTransformRef() { return transform_; }
  CharacterAnimator *GetModel() { return model_.get(); }
  float &GetVelocityYRef() { return velocityY_; }
  float &GetRollTimerRef() { return rollTimer_; }
  bool GetKeepRolling() const { return keepRolling_; }
  float GetRollTransitionDuration() const { return rollTransitionDuration_; }
  float GetWalkTransitionDuration() const { return walkTransitionDuration_; }
  float &GetCurrentRecoveryTimerRef() { return currentRecoveryTimer_; }
  float GetJumpRecovery() const { return jumpRecovery_; }
  float GetRollRecovery() const { return rollRecovery_; }
  float GetLaneChangeRecovery() const { return laneChangeRecovery_; }
  float &GetHitTimerRef() { return hitTimer_; }
  float GetHitDuration() const { return hitDuration_; }
  Vector3 &GetKnockbackVelocityRef() { return knockbackVelocity_; }
  bool GetIsTrip() const { return isTrip_; }
  void SetIsTrip(bool trip) { isTrip_ = trip; }
  float &GetForcedCenterTimerRef() { return forcedCenterTimer_; }
  float GetForcedCenterDuration() const { return forcedCenterDuration_; }
  void SetForcedCenterDuration(float d) { forcedCenterDuration_ = d; }
  float &GetForcedCenterStartXRef() { return forcedCenterStartX_; }
  void SetLaneIndex(int lane) { laneIndex_ = lane; }
  void SetTargetLaneIndex(int lane) { targetLaneIndex_ = lane; }
  float &GetStartXRef() { return startX_; }
  float &GetLerpTimeRef() { return lerpTime_; }
  int GetMinLane() const { return minLane_; }
  int GetMaxLane() const { return maxLane_; }
  float GetLaneWidth() const { return laneWidth_; }
};

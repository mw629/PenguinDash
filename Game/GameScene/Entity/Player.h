#pragma once

#include <Engine.h>
#include <memory>

#include "GameObject.h"
#include <CharacterAnimator.h>

class Player : public GameObject {
private:
  enum MoveDirection { Left, Right, None };

  std::unique_ptr<CharacterAnimator> model_ =
      std::make_unique<CharacterAnimator>();
public:
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
  // transform_ is inherited from GameObject

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

  // アクション用の変数
  bool isJumping_ = false;
  float velocityY_ = 0.0f;
  float gravity_ = kDefaultGravity;
  float jumpPower_ = kDefaultJumpPower;
  float baseHeight_ = kDefaultBaseHeight; // 地面の高さ（Y座標）

  bool isRolling_ = false;
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
  bool isHit_ = false;
  bool isTrip_ = false;
  float hitTimer_ = 0.0f;
  float hitDuration_ = 90.0f; // ノックバックにかかるフレーム数
  Vector3 knockbackVelocity_{0.0f, 0.0f, 0.0f}; // ノックバック速度

  // 強制中央移動用の変数
  bool isForcedCentering_ = false;
  float forcedCenterTimer_ = 0.0f;
  float forcedCenterDuration_ = 30.0f; // 中央に到達するまでのフレーム数
  float forcedCenterStartX_ = 0.0f;

  // バリアアイテム取得時の状態
  bool hasBarrier_ = false;

  // 操作反転（ボス戦等でカメラが180度反転した際に画面の見た目通りに動くようにするフラグ）
  bool isInvertedControls_ = false;

  // オートパイロット（タイトル画面での自動運転モード）
  bool isAutoPilot_ = false;

  // 表示・非表示（一人称視点時の自キャラメッシュ非表示用など）
  bool isVisible_ = true;

  void UpdateDrawTransform(float speedMultiplier = 1.0f);

public:
  Player();
  ~Player();

  void Initialize(ModelData modelData);
  void Reset();

  void Update(Matrix4x4 view, float speedMultiplier = 1.0f) override;

  void PlayerMove(float speedMultiplier);
  void HitUpdate(float speedMultiplier);

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

  // GetTransform() is inherited from GameObject
  bool GetIsRolling() const { return isRolling_; }
  void SetKeepRolling(bool keep) { keepRolling_ = keep; }
  bool GetIsJumping() const { return isJumping_; }

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
  bool CanAct() const { return currentRecoveryTimer_ <= 0.0f; }

  // ヒット演出用
  void OnHit(bool isTrip = false);
  bool IsHitAnimationFinished() const;

  // 狭まる区間で強制的に中央へ寄せる
  void StartForceToCenter(float duration);
};

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
  std::unique_ptr<Model> axe_ = std::make_unique<Model>();
  Transform axeOffset_{{100.0f, 100.0f, 100.0f},
                       {
                           0.0f,
                           0.0f,
                           0.0f,
                       },
                       {0.0f, 0.0f, 0.0f}};
  bool isDrawAxe_ = false;

  // ペンギンモデルのスケール・オフセット（GLTFモデルの0.01スケールを等身大に補正）
  Transform modelOffset_{
      {140.0f, 140.0f, 140.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}};
  // transform_ is inherited from GameObject

  // レーン移動のための変数
  int laneIndex_ = 0;       // 現在のレーン位置
  int targetLaneIndex_ = 0; // 目標のレーン位置

  float laneChangeSpeed_ = 0.2f;                      // レーン移動の速度
  float lerpTime_ = 0.0f;                             // 補間用タイマー
  float startX_ = 0.0f;                               // 移動開始時のX座標
  MoveDirection moveDirection_ = MoveDirection::None; // 移動方向

  // レーン設定
  int minLane_ = -1;
  int maxLane_ = 1;
  float laneWidth_ = 4.0f;

  // アクション用の変数
  bool isJumping_ = false;
  float velocityY_ = 0.0f;
  float gravity_ = 0.014f;
  float jumpPower_ = 0.21f;
  float baseHeight_ = 3.0f; // 地面の高さ（Y座標）

  bool isRolling_ = false;
  float rollTimer_ = 0.0f;
  float rollDuration_ = 30.0f; // 転がりの継続フレーム数（約0.5秒）
  bool keepRolling_ = false;   // 強制的にしゃがみを維持するフラグ
  float rollTransitionDuration_ =
      0.06f; // しゃがみ（sneakWalk）への遷移ブレンド秒数（約3〜4フレーム）
  float walkTransitionDuration_ =
      0.08f; // 立ち上がり（walk）への遷移ブレンド秒数（約5フレーム）

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

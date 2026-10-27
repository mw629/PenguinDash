#pragma once

#include <Engine.h>
#include <memory>
#include <functional>

#include "GameObject.h"
#include <CharacterAnimator.h>

/// <summary>
/// プレイヤーのアクション（状態）タイプを表す列挙型
/// </summary>
enum class PlayerActionType {
  Run,          //!< 通常走行
  Jump,         //!< ジャンプ中
  Roll,         //!< スライディング（しゃがみ・ロール）中
  Hit,          //!< 障害物被弾・ノックバック・転倒中
  ForcedCenter  //!< 1レーン区間等のステージギミックによる強制中央復帰中
};

class Player;

/// <summary>
/// プレイヤーのアクションステート基底インターフェース（Stateパターン）
/// </summary>
class IPlayerState {
public:
  virtual ~IPlayerState() = default;

  /// <summary>
  /// ステート種別の取得
  /// </summary>
  virtual PlayerActionType GetType() const = 0;

  /// <summary>
  /// ステート開始時の処理
  /// </summary>
  /// <param name="player">プレイヤー参照</param>
  virtual void Enter(Player &/*player*/) {}

  /// <summary>
  /// ステート更新処理
  /// </summary>
  /// <param name="player">プレイヤー参照</param>
  /// <param name="speedMultiplier">ゲーム速度乗数</param>
  virtual void Update(Player &player, float speedMultiplier) = 0;

  /// <summary>
  /// ステート終了時の処理
  /// </summary>
  /// <param name="player">プレイヤー参照</param>
  virtual void Exit(Player &/*player*/) {}

  /// <summary>
  /// このステート中にプレイヤーが通常アクション（レーン移動、ジャンプ等）を入力可能かどうか
  /// </summary>
  virtual bool CanAct() const { return true; }
};

/// <summary>
/// 通常走行ステート
/// 地上を走行し、レーン移動やジャンプ・スライディング等の各種アクション入力を受け付ける
/// </summary>
class PlayerRunState : public IPlayerState {
public:
  PlayerActionType GetType() const override { return PlayerActionType::Run; }
  void Enter(Player &player) override;
  void Update(Player &player, float speedMultiplier) override;
};

/// <summary>
/// ジャンプステート
/// 重力と初速による放物線運動を行い、着地時に走行ステートへ遷移する
/// </summary>
class PlayerJumpState : public IPlayerState {
public:
  PlayerActionType GetType() const override { return PlayerActionType::Jump; }
  void Enter(Player &player) override;
  void Update(Player &player, float speedMultiplier) override;
};

/// <summary>
/// スライディング（ロール）ステート
/// 姿勢を低くして低い障害物を回避する。頭上に障害物がある場合は自動でスライディングが継続される
/// </summary>
class PlayerRollState : public IPlayerState {
public:
  PlayerActionType GetType() const override { return PlayerActionType::Roll; }
  void Enter(Player &player) override;
  void Update(Player &player, float speedMultiplier) override;
  void Exit(Player &player) override;
};

/// <summary>
/// 被弾・ノックバックステート
/// 障害物に衝突した際のノックバック演出を行い、一定時間操作不能となる
/// </summary>
class PlayerHitState : public IPlayerState {
public:
  PlayerActionType GetType() const override { return PlayerActionType::Hit; }
  void Enter(Player &player) override;
  void Update(Player &player, float speedMultiplier) override;
  void Exit(Player &player) override;
  bool CanAct() const override { return false; }
};

/// <summary>
/// 強制中央復帰ステート
/// 1レーン区間等のステージギミック突入時に、プレイヤーを自動的に中央レーンへ補間移動させる
/// </summary>
class PlayerForcedCenterState : public IPlayerState {
public:
  PlayerActionType GetType() const override { return PlayerActionType::ForcedCenter; }
  void Enter(Player &player) override;
  void Update(Player &player, float speedMultiplier) override;
  bool CanAct() const override { return false; }
};

/// <summary>
/// ランゲームのプレイヤーキャラクター（ペンギン）を制御するクラス
/// </summary>
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

  // プレイヤー入力有効フラグ（カウントダウン中などの操作ロック用）
  bool isInputEnabled_ = true;

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
  /// <summary>
  /// コンストラクタ
  /// </summary>
  Player();

  /// <summary>
  /// デストラクタ
  /// </summary>
  ~Player();

  /// <summary>
  /// プレイヤーモデルおよびアニメーションの初期化
  /// </summary>
  /// <param name="modelData">読み込み用モデルデータ</param>
  void Initialize(ModelData modelData);

  /// <summary>
  /// プレイヤー状態（位置、レーン、速度、ステート等）のリセット
  /// </summary>
  void Reset();

  /// <summary>
  /// プレイヤーの毎フレーム更新
  /// </summary>
  /// <param name="view">ビュー行列</param>
  /// <param name="speedMultiplier">ゲーム速度乗数</param>
  void Update(Matrix4x4 view, float speedMultiplier) override;

  /// <summary>
  /// プレイヤーの毎フレーム更新（デフォルト速度）
  /// </summary>
  /// <param name="view">ビュー行列</param>
  void Update(Matrix4x4 view) { Update(view, kDefaultSpeedMultiplier); }

  /// <summary>
  /// レーン移動の入力受付（キーボード/ゲームパッド）
  /// </summary>
  void ProcessLaneInput();

  /// <summary>
  /// レーン間の滑らかな補間移動更新
  /// </summary>
  /// <param name="speedMultiplier">速度乗数</param>
  void UpdateLaneMovement(float speedMultiplier);

  /// <summary>
  /// レーン範囲と幅の設定
  /// </summary>
  /// <param name="minLane">最小レーンインデックス</param>
  /// <param name="maxLane">最大レーンインデックス</param>
  /// <param name="laneWidth">レーン1本あたりの幅</param>
  void SetLaneLimits(int minLane, int maxLane, float laneWidth) {
    minLane_ = minLane;
    maxLane_ = maxLane;
    laneWidth_ = laneWidth;
  }

  /// <summary>
  /// プレイヤーの描画処理
  /// </summary>
  /// <param name="draw">描画マネージャ</param>
  void Draw(class Draw &draw) override;

  /// <summary>
  /// ImGuiによるパラメータ編集インスペクター
  /// </summary>
  void ImGuiInnerComponents() override;

  bool HasMaterial() const override {
    return model_ && model_->GetComponent<MaterialComponent>() != nullptr ||
           GameObject::HasMaterial();
  }

  // =========================================================
  // ステート管理・状態確認
  // =========================================================

  /// <summary>
  /// 現在のアクションタイプを取得
  /// </summary>
  PlayerActionType GetActionType() const {
    return currentState_ ? currentState_->GetType() : PlayerActionType::Run;
  }

  /// <summary>
  /// 現在のアクションステートインスタンスを取得
  /// </summary>
  IPlayerState *GetCurrentActionState() const { return currentState_.get(); }

  /// <summary>
  /// 現在のゲーム速度乗数を取得
  /// </summary>
  float GetCurrentSpeedMultiplier() const { return currentSpeedMultiplier_; }

  /// <summary>
  /// アクションステートを新しいステートインスタンスに遷移
  /// </summary>
  /// <param name="newState">遷移先ステート</param>
  void ChangeActionState(std::unique_ptr<IPlayerState> newState);

  /// <summary>
  /// アクションステートを種別指定で遷移
  /// </summary>
  /// <param name="nextType">遷移先のアクション種別</param>
  void ChangeActionState(PlayerActionType nextType);

  /// <summary>
  /// スライディング（ロール）中かどうか
  /// </summary>
  bool GetIsRolling() const { return GetActionType() == PlayerActionType::Roll; }

  /// <summary>
  /// スライディング状態を強制維持するか設定（頭上に障害物がある場合など）
  /// </summary>
  void SetKeepRolling(bool keep) { keepRolling_ = keep; }

  /// <summary>
  /// ジャンプ中かどうか
  /// </summary>
  bool GetIsJumping() const { return GetActionType() == PlayerActionType::Jump; }

  /// <summary>
  /// 被弾中（ノックバック中）かどうか
  /// </summary>
  bool GetIsHit() const { return GetActionType() == PlayerActionType::Hit; }

  /// <summary>
  /// 中央強制移動中かどうか
  /// </summary>
  bool GetIsForcedCentering() const { return GetActionType() == PlayerActionType::ForcedCenter; }

  // =========================================================
  // パラメータ取得・設定
  // =========================================================

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

  /// <summary>
  /// バリア所持状態の取得
  /// </summary>
  bool GetHasBarrier() const { return hasBarrier_; }

  /// <summary>
  /// バリア所持状態の設定（マテリアルカラーの切り替えも含む）
  /// </summary>
  void SetHasBarrier(bool hasBarrier);

  /// <summary>
  /// 操作反転フラグの設定（ボス戦やカメラ反転演出用）
  /// </summary>
  void SetInvertedControls(bool inv) { isInvertedControls_ = inv; }
  bool GetInvertedControls() const { return isInvertedControls_; }

  /// <summary>
  /// オートパイロット（自動運転）モードの設定
  /// </summary>
  void SetAutoPilot(bool autoPilot) { isAutoPilot_ = autoPilot; }
  bool GetAutoPilot() const { return isAutoPilot_; }

  /// <summary>
  /// プレイヤー入力有効フラグ設定（カウントダウン等の操作ロック用）
  /// </summary>
  void SetInputEnabled(bool enable) { isInputEnabled_ = enable; }
  bool GetInputEnabled() const { return isInputEnabled_; }

  /// <summary>
  /// プレイヤー描画表示・非表示の設定
  /// </summary>
  void SetVisible(bool visible) { isVisible_ = visible; }
  bool GetVisible() const { return isVisible_; }

  // =========================================================
  // アクション発動トリガー（入力・オートパイロット共用）
  // =========================================================

  /// <summary>
  /// 外部からのジャンプ発動（オートパイロット等用）
  /// </summary>
  /// <returns>発動に成功した場合true</returns>
  bool TriggerJump();

  /// <summary>
  /// 外部からのスライディング（ロール）発動
  /// </summary>
  /// <returns>発動に成功した場合true</returns>
  bool TriggerRoll();

  /// <summary>
  /// 外部からの左レーン移動発動
  /// </summary>
  /// <returns>発動に成功した場合true</returns>
  bool TriggerMoveLeft();

  /// <summary>
  /// 外部からの右レーン移動発動
  /// </summary>
  /// <returns>発動に成功した場合true</returns>
  bool TriggerMoveRight();

  // =========================================================
  // レーン・アクション可否状態
  // =========================================================

  int GetLaneIndex() const { return laneIndex_; }
  int GetTargetLaneIndex() const { return targetLaneIndex_; }
  bool IsChangingLane() const { return laneIndex_ != targetLaneIndex_; }

  /// <summary>
  /// 現在プレイヤーが入力やアクションを行える状態か判定
  /// </summary>
  bool CanAct() const {
    return isInputEnabled_ && (currentRecoveryTimer_ <= 0.0f) && (!currentState_ || currentState_->CanAct());
  }

  // =========================================================
  // 先行入力バッファ
  // =========================================================

  /// <summary>
  /// 先行入力バッファのタイマー更新
  /// </summary>
  void UpdateInputBuffer(float speedMultiplier);
  bool HasBufferedJump() const { return bufferedJumpTimer_ > 0.0f; }
  bool HasBufferedRoll() const { return bufferedRollTimer_ > 0.0f; }
  bool HasBufferedLane() const { return bufferedLaneTimer_ > 0.0f && bufferedLaneDirection_ != 0; }
  int GetBufferedLaneDirection() const { return bufferedLaneDirection_; }

  /// <summary>
  /// バッファされたジャンプ入力を消費
  /// </summary>
  /// <returns>消費できた場合true</returns>
  bool ConsumeBufferedJump() {
    if (bufferedJumpTimer_ > 0.0f) {
      bufferedJumpTimer_ = 0.0f;
      return true;
    }
    return false;
  }

  /// <summary>
  /// バッファされたスライディング入力を消費
  /// </summary>
  /// <returns>消費できた場合true</returns>
  bool ConsumeBufferedRoll() {
    if (bufferedRollTimer_ > 0.0f) {
      bufferedRollTimer_ = 0.0f;
      return true;
    }
    return false;
  }

  /// <summary>
  /// バッファされたレーン移動入力を消費
  /// </summary>
  /// <returns>移動方向（-1: 左, +1: 右, 0: なし）</returns>
  int ConsumeBufferedLane() {
    if (bufferedLaneTimer_ > 0.0f && bufferedLaneDirection_ != 0) {
      int dir = bufferedLaneDirection_;
      bufferedLaneTimer_ = 0.0f;
      bufferedLaneDirection_ = 0;
      return dir;
    }
    return 0;
  }

  /// <summary>
  /// 先行入力バッファをクリア
  /// </summary>
  void ClearInputBuffer() {
    bufferedJumpTimer_ = 0.0f;
    bufferedRollTimer_ = 0.0f;
    bufferedLaneTimer_ = 0.0f;
    bufferedLaneDirection_ = 0;
  }

  // =========================================================
  // コールバック・衝突判定・ギミック
  // =========================================================

  /// <summary>
  /// ジャンプ着地時の演出コールバックを設定（カメラシェイク・土煙等用）
  /// </summary>
  void SetOnLandCallback(std::function<void()> callback) { onLandCallback_ = callback; }
  const std::function<void()>& GetOnLandCallback() const { return onLandCallback_; }

  /// <summary>
  /// 障害物被弾・衝突時の処理開始
  /// </summary>
  /// <param name="isTrip">つまずき転倒かどうか</param>
  void OnHit(bool isTrip = false);

  /// <summary>
  /// 被弾・ノックバックアニメーションが完了したか判定
  /// </summary>
  bool IsHitAnimationFinished() const;

  /// <summary>
  /// 現在の姿勢に応じたコライダーAABBを取得
  /// </summary>
  AABB GetColliderAABB() const;

  /// <summary>
  /// 通常立ちポーズ時のコライダーAABBを取得（スライディング継続判定用）
  /// </summary>
  AABB GetStandingColliderAABB() const;

  /// <summary>
  /// 1レーン区間等への移行時に中央レーンへ強制移動開始
  /// </summary>
  /// <param name="duration">移動完了までのフレーム数</param>
  void StartForceToCenter(float duration);

  // =========================================================
  // ステートクラス向けのアクセサ
  // =========================================================
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

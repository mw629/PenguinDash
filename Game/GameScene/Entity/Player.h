#pragma once

#include <Engine.h>
#include <memory>
#include <functional>

#include "GameObject.h"
#include <CharacterAnimator.h>

/// <summary>
/// プレイヤーのアクション・操作ステート種別
/// </summary>
enum class PlayerActionType {
  Run,          // 走行（通常移動）
  Jump,         // ジャンプ（跳躍・空中）
  Roll,         // ロール（スライディング・低姿勢）
  Hit,          // 被弾・ノックバック
  ForcedCenter  // 中央レーンへの強制移動
};

class Player;

/// <summary>
/// プレイヤーステート（操作・行動状態）基底インターフェースクラス
/// Stateパターンにより、走行・ジャンプ・ロール等の状態ごとの挙動や入力処理を分離・管理します。
/// </summary>
class IPlayerState {
public:
  virtual ~IPlayerState() = default;

  /// <summary>
  /// ステート種別の取得
  /// </summary>
  /// <returns>対応するPlayerActionType</returns>
  virtual PlayerActionType GetType() const = 0;

  /// <summary>
  /// ステート開始時の初期化処理
  /// </summary>
  /// <param name="player">プレイヤー参照</param>
  virtual void Enter(Player &/*player*/) {}

  /// <summary>
  /// 毎フレームの更新処理（入力受付、移動物理、アニメーション制御等）
  /// </summary>
  /// <param name="player">プレイヤー参照</param>
  /// <param name="speedMultiplier">ゲーム全体の速度倍率（タイムスケール・スクロール比率）</param>
  virtual void Update(Player &player, float speedMultiplier) = 0;

  /// <summary>
  /// ステート終了時の後処理・リセット
  /// </summary>
  /// <param name="player">プレイヤー参照</param>
  virtual void Exit(Player &/*player*/) {}

  /// <summary>
  /// プレイヤーが任意操作（ジャンプ・ロール・レーン移動等）を行えるかどうか
  /// </summary>
  /// <returns>操作可能な場合 true、被弾中や強制移動中など操作不能な場合 false</returns>
  virtual bool CanAct() const { return true; }
};

/// <summary>
/// プレイヤーの走行（通常移動）ステートクラス
/// 地上を走行中のアニメーション再生、左右レーン移動入力の処理、
/// およびジャンプ／ロールへのアクション遷移（先行入力含む）を管理します。
/// </summary>
class PlayerRunState : public IPlayerState {
public:
  PlayerActionType GetType() const override { return PlayerActionType::Run; }
  void Enter(Player &player) override;
  void Update(Player &player, float speedMultiplier) override;
};

/// <summary>
/// プレイヤーのジャンプ（跳躍・滞空）ステートクラス
/// 上方向への跳躍初速の適用、重力による落下物理計算、空中でのレーン移動制御、
/// 着地判定・着地SE・画面揺れ、および着地時の先行入力バッファ判定を管理します。
/// </summary>
class PlayerJumpState : public IPlayerState {
public:
  PlayerActionType GetType() const override { return PlayerActionType::Jump; }
  void Enter(Player &player) override;
  void Update(Player &player, float speedMultiplier) override;
};

/// <summary>
/// プレイヤーのロール（スライディング・低姿勢回避）ステートクラス
/// 高空障害物をくぐるための低姿勢化（モデル高さ調整・アニメーション再生）、
/// タイマーによる持続制御（長押し継続対応）、レーン移動やジャンプによるキャンセルを管理します。
/// </summary>
class PlayerRollState : public IPlayerState {
public:
  PlayerActionType GetType() const override { return PlayerActionType::Roll; }
  void Enter(Player &player) override;
  void Update(Player &player, float speedMultiplier) override;
  void Exit(Player &player) override;
};

/// <summary>
/// プレイヤーの被弾（ノックバック・被ダメージ）ステートクラス
/// 障害物や敵攻撃衝突時の吹き飛び物理挙動・回転演出を制御し、
/// 被弾演出中はプレイヤーの任意操作を無効化（CanAct = false）します。
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
/// プレイヤーの中央レーン強制移動ステートクラス
/// ボス戦開始演出時などに、現在位置から滑らかなイージング補間によって
/// プレイヤーを中央レーン（レーン0）へ強制的に移動させます（移動中は操作無効）。
/// </summary>
class PlayerForcedCenterState : public IPlayerState {
public:
  PlayerActionType GetType() const override { return PlayerActionType::ForcedCenter; }
  void Enter(Player &player) override;
  void Update(Player &player, float speedMultiplier) override;
  bool CanAct() const override { return false; }
};

/// <summary>
/// プレイヤークラス（主人公ペンギン）
/// 3レーン（可変レーン）ランゲームにおけるプレイヤーの制御を担当します。
/// 
/// 【主な責務】
/// 1. 移動・レーン制御:
///    - 左右入力に応じたレーン間の滑らかなLerp補間移動
///    - レーン境界（minLane〜maxLane）の制限
///    - ギミックによる中央レーンへの強制復帰
/// 2. アクション・ステートマシン（Stateパターン）:
///    - 通常走行（Run）、ジャンプ（Jump）、スライディング（Roll）、被弾・転倒（Hit）、中央復帰（ForcedCenter）
/// 3. 先行入力バッファ:
///    - 着地直前や硬直中のジャンプ・ロール・レーン移動入力を保持し、操作感を向上
/// 4. 演出・アニメーション:
///    - CharacterAnimatorによるGLTFラン／スライドモデルのブレンドアニメーション
///    - 着地時のシェイク演出コールバック、大型障害物被弾時の画面激突演出（Camera Slam）
/// 5. アイテム・特殊状態:
///    - バリア機能、操作反転（ボス戦演出等）、タイトル用オートパイロット
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

  // モデルのスケール定数
  static constexpr Vector3 kDefaultModelScale = {140.0f, 140.0f, 140.0f};
  static constexpr Vector3 kDefaultModelTranslate = {0.0f, -1.0f, 0.0f};
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

  // カメラ激突（Camera Slam）演出定数
  static constexpr float kCameraSlamDurationFrames = 20.0f; // カメラへ飛んでいくフレーム数
  static constexpr float kCameraStickSlideSpeedY = -0.005f;  // 画面張り付き後の滑り落ち速度
  static constexpr float kCameraSlamSpinSpeedX = 0.25f;    // 吹き飛び回転速度X
  static constexpr float kCameraSlamSpinSpeedZ = 0.15f;    // 吹き飛び回転速度Z

  // 腹滑り雪玉化（Snowball Hit）演出定数
  static constexpr float kTripSnowballDuration = 110.0f;  // 演出全体のフレーム数（約1.8秒）
  static constexpr float kSnowballFormDelay = 12.0f;      // 腹ばい滑走から雪玉が形成され始めるフレーム
  static constexpr float kSnowballFullFormFrames = 42.0f; // 雪玉が最大サイズに到達するフレーム
  static constexpr float kSnowballMaxScale = 2.0f;        // 雪玉の最大スケール
  static constexpr float kSnowballInitialSpeedZ = 0.40f;  // 初期滑走速度
  static constexpr float kSnowballMaxSpeedZ = 0.85f;      // 雪玉加速後の最高速度
  static constexpr float kSnowballRollSpeed = 0.30f;      // 雪玉の回転速度

private:
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

  // カメラ激突（Camera Slam）演出用変数
  bool isCameraSlam_ = false;       // 大型障害物に衝突してカメラに向かって吹き飛ぶフラグ
  bool hasHitCamera_ = false;       // カメラ画面に到達・激突したフラグ
  Vector3 cameraTargetPos_{0.0f, 0.0f, -12.5f}; // カメラ激突目標座標
  std::function<void()> onCameraHitCallback_ = nullptr; // カメラ激突時のコールバック（シェイク等）

  // 腹滑り雪玉化演出用変数
  std::unique_ptr<Model> snowballModel_ = std::make_unique<Model>();
  Transform snowballTransform_{};
  bool isSnowballActive_ = false;
  bool isSnowballHidingPenguin_ = false;
  float snowballScale_ = 0.0f;
  float snowballRotationX_ = 0.0f;

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

  // プレイヤー入力有効フラグ設定
  void SetInputEnabled(bool enable) { isInputEnabled_ = enable; }
  bool GetInputEnabled() const { return isInputEnabled_; }

  // 表示設定
  void SetVisible(bool visible) { isVisible_ = visible; }
  bool GetVisible() const { return isVisible_; }

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

  int GetLaneIndex() const { return laneIndex_; }
  int GetTargetLaneIndex() const { return targetLaneIndex_; }
  bool IsChangingLane() const { return laneIndex_ != targetLaneIndex_; }
  bool CanAct() const {
    return isInputEnabled_ && (currentRecoveryTimer_ <= 0.0f) && (!currentState_ || currentState_->CanAct());
  }

  /// <summary>
  /// 先行入力バッファのタイマー更新
  /// </summary>
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

  // カメラ激突（Camera Slam）アクセス
  bool GetIsCameraSlam() const { return isCameraSlam_; }
  void SetIsCameraSlam(bool slam) { isCameraSlam_ = slam; }
  bool GetHasHitCamera() const { return hasHitCamera_; }
  void SetHasHitCamera(bool hit) { hasHitCamera_ = hit; }
  const Vector3& GetCameraTargetPos() const { return cameraTargetPos_; }
  void SetCameraTargetPos(const Vector3& pos) { cameraTargetPos_ = pos; }
  void SetOnCameraHitCallback(std::function<void()> callback) { onCameraHitCallback_ = callback; }
  void ExecuteCameraHitCallback() { if (onCameraHitCallback_) onCameraHitCallback_(); }

  /// <summary>
  /// 障害物被弾・衝突時の処理開始
  /// </summary>
  /// <param name="isTrip">つまずき転倒かどうか</param>
  /// <param name="isLargeObstacle">大型障害物（カメラ激突対象）かどうか</param>
  /// <param name="cameraPos">カメラ（画面）の激突目標座標</param>
  void OnHit(bool isTrip = false, bool isLargeObstacle = false, const Vector3& cameraPos = {0.0f, 3.0f, -12.5f});

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

  // 腹滑り雪玉化用アクセサ
  bool GetIsSnowballActive() const { return isSnowballActive_; }
  void SetSnowballActive(bool active) { isSnowballActive_ = active; }
  bool GetIsSnowballHidingPenguin() const { return isSnowballHidingPenguin_; }
  void SetSnowballHidingPenguin(bool hide) { isSnowballHidingPenguin_ = hide; }
  float GetSnowballScale() const { return snowballScale_; }
  void SetSnowballScale(float scale) { snowballScale_ = scale; }
  float GetSnowballRotationX() const { return snowballRotationX_; }
  void SetSnowballRotationX(float rot) { snowballRotationX_ = rot; }
  float &GetSnowballRotationXRef() { return snowballRotationX_; }
  const Transform &GetSnowballTransform() const { return snowballTransform_; }
  void SetSnowballTransform(const Transform &trans) { snowballTransform_ = trans; }
  const Transform &GetModelOffset() const { return modelOffset_; }

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

#pragma once
#include <memory>

class Boss;

/// <summary>
/// ボスの状態種別
/// </summary>
enum class BossState {
  Appearance, // 出現
  Battle,     // 戦闘
  Defeat,     // 撃破
  Victory     // 勝利（プレイヤー敗北）
};

/// <summary>
/// ボスステート基底インターフェース
/// 各状態固有のタイマー・挙動・被ダメージ応答をカプセル化します。
/// </summary>
class IBossState {
protected:
  float stateTimer_ = 0.0f;

public:
  virtual ~IBossState() = default;

  /// <summary>
  /// ステート種別を取得
  /// </summary>
  virtual BossState GetType() const = 0;

  /// <summary>
  /// ステート滞在時間を取得
  /// </summary>
  float GetStateTimer() const { return stateTimer_; }

  /// <summary>
  /// ステート開始時の初期化処理
  /// </summary>
  virtual void Enter(Boss & /*boss*/) { stateTimer_ = 0.0f; }

  /// <summary>
  /// 毎フレーム更新処理。遷移が必要な場合は次のステートインスタンスを返す。
  /// </summary>
  virtual std::unique_ptr<IBossState> Update(Boss &boss, float dt,
                                             float speedMultiplier) = 0;

  /// <summary>
  /// 被ダメージ時の応答処理。遷移が必要な場合は次のステートインスタンスを返す。
  /// </summary>
  virtual std::unique_ptr<IBossState> OnDamage(Boss &boss);

  /// <summary>
  /// ステート終了時の後処理
  /// </summary>
  virtual void Exit(Boss & /*boss*/) {}
};


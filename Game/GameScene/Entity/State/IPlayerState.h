#pragma once
#include <memory>

class Player;

/// <summary>
/// プレイヤーのアクション・操作ステート種別
/// </summary>
enum class PlayerActionType {
  Run,         // 走行（通常移動）
  Jump,        // ジャンプ（跳躍・空中）
  Roll,        // ロール（スライディング・低姿勢）
  Hit,         // 被弾・ノックバック
  ForcedCenter // 中央レーンへの強制移動
};

class IPlayerState;

/// <summary>
/// ステート更新時の遷移リクエスト
/// 遷移元ステート自身が Update 途中で破棄されないよう、遷移先を戻り値として返します。
/// </summary>
struct PlayerStateTransition {
  std::unique_ptr<IPlayerState> nextState;
  bool updateImmediately = false;

  static PlayerStateTransition None() { return {nullptr, false}; }
  static PlayerStateTransition To(std::unique_ptr<IPlayerState> state,
                                  bool immediate = false) {
    return {std::move(state), immediate};
  }
};

/// <summary>
/// プレイヤーステート（操作・行動状態）基底インターフェース
/// Stateパターンにより、走行・ジャンプ・ロール・被弾・強制中央移動の
/// 状態固有データと挙動をカプセル化します。
/// </summary>
class IPlayerState {
public:
  virtual ~IPlayerState() = default;

  /// <summary>
  /// ステート種別の取得
  /// </summary>
  virtual PlayerActionType GetType() const = 0;

  /// <summary>
  /// ステート開始時の初期化処理
  /// </summary>
  virtual void Enter(Player & /*player*/) {}

  /// <summary>
  /// 毎フレームの更新処理。遷移が必要な場合は遷移リクエストを返す。
  /// </summary>
  virtual PlayerStateTransition Update(Player &player,
                                       float speedMultiplier) = 0;

  /// <summary>
  /// ステート終了時の後処理・リセット
  /// </summary>
  virtual void Exit(Player & /*player*/) {}

  /// <summary>
  /// プレイヤーが任意操作（ジャンプ・ロール・レーン移動等）を行えるかどうか
  /// </summary>
  virtual bool CanAct() const { return true; }

  /// <summary>
  /// 固定タイムスケール（1.0f）で更新すべきか（被弾演出など）
  /// </summary>
  virtual bool UsesFixedTimeScale() const { return false; }

  /// <summary>
  /// 描画オフセットとしてローリング用の低姿勢高さ補正が必要かどうか
  /// </summary>
  virtual bool NeedsRollingHeightOffset() const { return false; }

  /// <summary>
  /// つまずき転倒（雪玉化）被弾中かどうか
  /// </summary>
  virtual bool IsTripHit() const { return false; }

  /// <summary>
  /// 被弾アニメーションが完了したかどうか
  /// </summary>
  virtual bool IsHitAnimationFinished() const { return false; }
};


#pragma once
#include <Math/Calculation.h>
#include <memory>

class GameScene;

/// <summary>
/// プレイモード種別（3レーン / 1レーン / ボス戦）
/// </summary>
enum class PlayingState { ThreeLane = 0, OneLane = 1, Boss = 2 };

/// <summary>
/// プレイモードステート基底インターフェース
/// 3レーン走行・1レーン（右サイドビュー）・ボス戦の各モード固有のデータと振る舞いをカプセル化します。
/// </summary>
class IPlayingState {
public:
  virtual ~IPlayingState() = default;

  /// <summary>
  /// プレイモード種別を取得
  /// </summary>
  virtual PlayingState GetType() const = 0;

  /// <summary>
  /// モード開始時の初期化およびカメラ目標値・レーン数の設定
  /// </summary>
  virtual void Enter(GameScene &scene, PlayingState prevState,
                     Transform &outTarget, int &outLaneCount) = 0;

  /// <summary>
  /// モード終了時の後処理
  /// </summary>
  virtual void Exit(GameScene & /*scene*/, PlayingState /*nextState*/) {}

  /// <summary>
  /// 毎フレーム更新処理。遷移が必要な場合は遷移先ステートを返す。
  /// </summary>
  virtual std::unique_ptr<IPlayingState> Update(GameScene & /*scene*/,
                                                float /*timeScale*/,
                                                float /*scoreScrollSpeed*/) {
    return nullptr;
  }

  /// <summary>
  /// カメラトランジション完了時のコールバック処理
  /// </summary>
  virtual void OnCameraTransitionComplete(GameScene &scene);

  /// <summary>
  /// 左右入力を反転すべきかどうか（ボス戦のカメラ反転時など）
  /// </summary>
  virtual bool ShouldInvertControls(const GameScene & /*scene*/) const {
    return false;
  }

  /// <summary>
  /// 1レーン（右サイドビュー）モード中かどうか
  /// </summary>
  virtual bool IsOneLaneMode() const { return false; }

  /// <summary>
  /// 1レーンモード中の走行距離を取得
  /// </summary>
  virtual float GetRightSideDistance() const { return 0.0f; }
};


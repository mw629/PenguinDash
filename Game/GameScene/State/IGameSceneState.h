#pragma once
#include <memory>

class GameScene;

/// <summary>
/// ゲームシーン全体の状態種別
/// </summary>
enum class GameState {
  Title = 0,
  Playing = 1,
  Paused = 2,
  PlayerHit = 3,
  FirstPlaceCutIn = 4,
  GameOver = 5,
  Editor = 6
};

/// <summary>
/// ゲームシーン全体のステート基底インターフェース
/// 各状態（タイトル・プレイ中・ポーズ・被弾・1位カットイン・ゲームオーバー・エディター）の
/// 固有データ・遷移処理・更新・カメラ/ライト/UI制御ポリシーをカプセル化します。
/// </summary>
class IGameSceneState {
public:
  virtual ~IGameSceneState() = default;

  /// <summary>
  /// ステート種別を取得
  /// </summary>
  virtual GameState GetType() const = 0;

  /// <summary>
  /// ステート開始時の初期化処理
  /// </summary>
  virtual void Enter(GameScene & /*scene*/) {}

  /// <summary>
  /// 毎フレーム更新処理。遷移が必要な場合は次のステートインスタンスを返す。
  /// </summary>
  virtual std::unique_ptr<IGameSceneState> Update(GameScene &scene) = 0;

  /// <summary>
  /// ステート終了時の後処理
  /// </summary>
  virtual void Exit(GameScene & /*scene*/, GameState /*nextState*/) {}

  /// <summary>
  /// 太陽光サイクル更新に使うデルタタイムを返す
  /// </summary>
  virtual float GetSunLightDeltaTime(float timeScale) const;

  /// <summary>
  /// 状態に応じた非FPVカメラ更新処理
  /// </summary>
  virtual void UpdateCamera(GameScene & /*scene*/, float /*dt*/) {}

  /// <summary>
  /// 1位カットイン演出を描画すべきかどうか
  /// </summary>
  virtual bool ShouldRenderRankingCutIn(const GameScene &scene) const;

  /// <summary>
  /// 実績解除通知ポップアップを描画してよい状態かどうか
  /// </summary>
  virtual bool ShouldDrawAchievementPopup() const { return true; }

  /// <summary>
  /// ポーズ中の設定モーダルが開いているか（PausedSceneState用）
  /// </summary>
  virtual bool IsPauseSettingsOpen() const { return false; }

  /// <summary>
  /// ポーズ中の設定モーダルを閉じる（PausedSceneState用）
  /// </summary>
  virtual void ClosePauseSettings() {}

  /// <summary>
  /// リザルト経過タイマー（GameOverSceneState用）
  /// </summary>
  virtual float GetResultTimer() const { return 0.0f; }

  /// <summary>
  /// リザルト獲得実績リストのスクロール位置（GameOverSceneState用）
  /// </summary>
  virtual int GetResultAchievementScrollOffset() const { return 0; }
};


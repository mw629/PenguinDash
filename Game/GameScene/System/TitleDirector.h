#pragma once
#include <Engine.h>
#include <GameObjects/Object/2d/Sprite.h>
#include <Math/Calculation.h>
#include <memory>
#include <array>

class Player;
class StageSettings;
class ScoreManager;
class GameCameraController;
class Texture;
class Draw;

class TitleDirector {
public:
  enum class CountdownMode {
    None,
    TitleExit,
    Restart
  };

private:
  // カウントダウン演出パラメータ
  static constexpr float kTitlePrepDuration = 1.0f;      // タイトル時のカメラ復帰ディレイ (障害物クリア待ち)
  static constexpr float kRestartPrepDuration = 0.6f;    // リスタート時の呼吸・準備ディレイ (間を持たせる)
  static constexpr float kCountdownStepDuration = 0.85f; // 各カウントの間隔 (3, 2, 1) をしっかり認識できるよう調整
  static constexpr float kGoDuration = 1.0f;             // GO!の表示時間
  static constexpr float kTitleExitDuration = 4.55f;     // タイトル演出全体: 1.0s + 2.55s + 1.0s
  static constexpr float kRestartDuration = 4.15f;       // リスタート演出全体: 0.6s + 2.55s + 1.0s
  static constexpr float kExitTimeoutOffset = 2.0f;
  static constexpr float kDefaultOrbitSpeed = 0.35f;
  static constexpr float kDefaultOrbitRadius = 14.0f;
  static constexpr float kDefaultOrbitHeight = 4.0f;
  static constexpr float kDefaultTargetOffsetY = 1.2f;

  static constexpr Vector2 kCenterPivot = {0.5f, 0.5f};
  static constexpr Vector4 kDefaultSpriteColor = {1.0f, 1.0f, 1.0f, 1.0f};

  static constexpr int kLaneCount = 3;

  // 自動操縦 AI パラメータ
  static constexpr float kAiDefaultLaneWidth = 2.0f;
  static constexpr float kAiMaxDetectionDist = 999.0f;
  static constexpr float kAiBehindIgnoreDist = -0.8f;
  static constexpr float kAiCloseIgnoreDist = -0.5f;
  static constexpr float kAiJumpTriggerDistMax = 3.6f;
  static constexpr float kAiJumpTriggerDistMin = 0.8f;
  static constexpr float kAiRollTriggerDistMax = 3.8f;
  static constexpr float kAiRollTriggerDistMin = 0.8f;
  static constexpr float kAiWallAvoidDist = 15.0f;
  static constexpr float kAiSafeSideDist = 5.0f;
  static constexpr float kAiCenterLaneClearDist = 4.0f;
  static constexpr float kAiReturnCenterTriggerDist = 20.0f;
  static constexpr float kAiReturnCenterMinDist = 22.0f;
  static constexpr float kAiReturnCenterWallDist = 30.0f;

  // カウントダウン表示位置・サイズパラメータ
  static constexpr Vector2 kPenguinCenterPos = {640.0f, 380.0f};
  static constexpr Vector2 kCountSpriteSize = {330.0f, 330.0f};
  static constexpr Vector2 kGoSpriteSize = {370.0f, 370.0f};

  CountdownMode countdownMode_ = CountdownMode::None;
  float countdownTimer_ = 0.0f;
  float prepDuration_ = 0.0f;
  float totalDuration_ = 0.0f;
  int lastCountStep_ = -1;
  Transform titleExitStartCamTransform_;

  // カウントダウン用ペンギンスプライト (0: 3, 1: 2, 2: 1, 3: GO!)
  std::array<std::unique_ptr<Sprite>, 4> countSprites_;
  std::array<int, 4> countTextureHandles_{-1, -1, -1, -1};

  // オービットカメラ
  bool isTitleOrbitCamera_ = true;
  float titleCameraAngle_ = 0.0f;
  float titleCameraSpeed_ = kDefaultOrbitSpeed;
  float titleCameraRadius_ = kDefaultOrbitRadius;
  float titleCameraHeight_ = kDefaultOrbitHeight;
  float titleTargetOffsetY_ = kDefaultTargetOffsetY;

public:
  TitleDirector() = default;
  ~TitleDirector() = default;

  /// <summary>
  /// カウントダウンスプライト等のリソース初期化
  /// </summary>
  /// <param name="texture">テクスチャマネージャ</param>
  void Initialize(Texture *texture);

  /// <summary>
  /// タイトル演出状態のリセット
  /// </summary>
  void Reset();

  /// <summary>
  /// タイトル画面からゲーム開始演出（カウントダウン）を開始
  /// </summary>
  /// <param name="currentCamTransform">現在のカメラトランスフォーム</param>
  void StartExit(const Transform &currentCamTransform);

  /// <summary>
  /// リスタート時のカウントダウン演出を開始
  /// </summary>
  void StartRestartCountdown();

  /// <summary>
  /// タイトル脱出演出中かどうかを取得
  /// </summary>
  bool IsExiting() const { return countdownMode_ == CountdownMode::TitleExit; }

  /// <summary>
  /// カウントダウン演出が稼働中かどうかを取得
  /// </summary>
  bool IsCountdownActive() const {
    return countdownMode_ != CountdownMode::None && countdownTimer_ < totalDuration_;
  }

  /// <summary>
  /// GO!演出に突入したか（操作解禁タイミング）を取得
  /// </summary>
  bool IsGoStarted() const {
    if (countdownMode_ == CountdownMode::None)
      return true;
    return countdownTimer_ >= (prepDuration_ + 3.0f * kCountdownStepDuration);
  }

  /// <summary>
  /// プレイヤー入力が許可されているかを取得
  /// </summary>
  bool IsInputAllowed() const {
    return IsGoStarted();
  }

  CountdownMode GetCountdownMode() const { return countdownMode_; }
  float GetExitTimer() const { return countdownTimer_; }
  float GetExitDuration() const { return kTitleExitDuration; }

  /// <summary>
  /// カウントダウンタイマーおよびスプライトアニメーションの更新
  /// </summary>
  /// <param name="dt">デルタタイム</param>
  void UpdateCountdown(float dt);

  /// <summary>
  /// タイトル脱出演出の更新
  /// </summary>
  /// <param name="dt">デルタタイム</param>
  void UpdateExit(float dt) { UpdateCountdown(dt); }

  /// <summary>
  /// タイトル演出が完全に終了したかを判定
  /// </summary>
  /// <param name="stageSettings">ステージ設定ポインタ</param>
  /// <returns>演出終了かつ開始準備完了時true</returns>
  bool IsExitComplete(StageSettings *stageSettings) const;

  /// <summary>
  /// タイトル画面でのペンギン自動操縦（オートパイロットAI）の更新
  /// </summary>
  /// <param name="player">プレイヤーポインタ</param>
  /// <param name="stageSettings">ステージ設定ポインタ</param>
  void UpdateAutoPilot(Player *player, StageSettings *stageSettings);

  /// <summary>
  /// タイトル画面での衝突判定（ボーナス等）
  /// </summary>
  void CheckCollisions(Player *player, StageSettings *stageSettings,
                       ScoreManager *scoreManager);

  /// <summary>
  /// タイトル画面のカメラ更新（オービット回転またはゲーム開始トランジション）
  /// </summary>
  /// <param name="player">プレイヤーポインタ</param>
  /// <param name="cameraController">カメラコントローラポインタ</param>
  /// <param name="dt">デルタタイム</param>
  void UpdateCamera(Player *player, GameCameraController *cameraController,
                    float dt);

  /// <summary>
  /// ペンギンカウントダウンスプライトの描画
  /// </summary>
  /// <param name="draw">描画マネージャ</param>
  void DrawPenguins(Draw &draw) { DrawCountdown(draw); }

  /// <summary>
  /// カウントダウン演出（3, 2, 1, GO!）の描画
  /// </summary>
  /// <param name="draw">描画マネージャ</param>
  void DrawCountdown(Draw &draw);

  /// <summary>
  /// ImGuiデバッグ設定の描画
  /// </summary>
  void ImGui();

  // オービットカメラ設定
  bool GetTitleOrbitCamera() const { return isTitleOrbitCamera_; }
  void SetTitleOrbitCamera(bool enable) { isTitleOrbitCamera_ = enable; }
  float GetTitleCameraSpeed() const { return titleCameraSpeed_; }
  void SetTitleCameraSpeed(float speed) { titleCameraSpeed_ = speed; }
  float GetTitleCameraRadius() const { return titleCameraRadius_; }
  void SetTitleCameraRadius(float radius) { titleCameraRadius_ = radius; }
  float GetTitleCameraHeight() const { return titleCameraHeight_; }
  void SetTitleCameraHeight(float height) { titleCameraHeight_ = height; }
  float GetTitleTargetOffsetY() const { return titleTargetOffsetY_; }
  void SetTitleTargetOffsetY(float offset) { titleTargetOffsetY_ = offset; }
};

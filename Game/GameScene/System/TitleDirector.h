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

  void Initialize(Texture *texture);
  void Reset();

  void StartExit(const Transform &currentCamTransform);
  void StartRestartCountdown();

  bool IsExiting() const { return countdownMode_ == CountdownMode::TitleExit; }
  bool IsCountdownActive() const {
    return countdownMode_ != CountdownMode::None && countdownTimer_ < totalDuration_;
  }
  bool IsGoStarted() const {
    if (countdownMode_ == CountdownMode::None)
      return true;
    return countdownTimer_ >= (prepDuration_ + 3.0f * kCountdownStepDuration);
  }
  bool IsInputAllowed() const {
    return IsGoStarted();
  }

  CountdownMode GetCountdownMode() const { return countdownMode_; }
  float GetExitTimer() const { return countdownTimer_; }
  float GetExitDuration() const { return kTitleExitDuration; }

  void UpdateCountdown(float dt);
  void UpdateExit(float dt) { UpdateCountdown(dt); }
  bool IsExitComplete(StageSettings *stageSettings) const;

  void UpdateAutoPilot(Player *player, StageSettings *stageSettings);
  void CheckCollisions(Player *player, StageSettings *stageSettings,
                       ScoreManager *scoreManager);
  void UpdateCamera(Player *player, GameCameraController *cameraController,
                    float dt);

  void DrawPenguins(Draw &draw) { DrawCountdown(draw); }
  void DrawCountdown(Draw &draw);
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

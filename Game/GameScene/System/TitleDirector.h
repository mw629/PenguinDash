#pragma once
#include <Engine.h>
#include <GameObjects/Object/2d/Sprite.h>
#include <Math/Calculation.h>
#include <memory>

class Player;
class StageSettings;
class ScoreManager;
class GameCameraController;
class Texture;
class Draw;

class TitleDirector {
private:
  static constexpr int kTitlePenguinCount = 15;
  static constexpr float kTitleExitDuration = 3.0f;
  static constexpr float kExitTimeoutOffset = 3.0f;
  static constexpr float kDefaultOrbitSpeed = 0.35f;
  static constexpr float kDefaultOrbitRadius = 14.0f;
  static constexpr float kDefaultOrbitHeight = 4.0f;
  static constexpr float kDefaultTargetOffsetY = 1.2f;

  static constexpr Vector3 kPenguinInitialPos = {-300.0f, 360.0f, 0.0f};
  static constexpr Vector2 kPenguinSpriteSize = {130.0f, 130.0f};
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

  // 群れペンギン疾走演出パラメータ
  static constexpr float kStampedeStartX = -100.0f;
  static constexpr float kStampedeEndX = 1620.0f;
  static constexpr float kStampedeBaseY = 360.0f;
  static constexpr float kStampedeStepY = 41.5f;
  static constexpr float kStampedeSlopeX = 26.0f;
  static constexpr float kStampedeMaxOffsetY = 290.5f;
  static constexpr float kStampedeBaseDepth = 0.92f;
  static constexpr float kStampedeDepthScale = 0.10f;
  static constexpr float kStampedeLeaderScale = 1.10f;
  static constexpr float kStampedeAnimTimeStep = 0.09f;
  static constexpr float kStampedeAnimFps = 10.0f;
  static constexpr float kStampedeBobbingHeight = 4.0f;
  static constexpr float kStampedeTiltAngle = -0.05f;
  static constexpr float kStampedeMaxDrawX = 1360.0f;
  static constexpr float kStampedeTrailAlphaScale = 1.5f;
  static constexpr float kStampedeTrailMaxAlpha = 0.65f;
  static constexpr Vector2 kStampedeTrailOffset = {-65.0f, 5.0f};
  static constexpr Vector2 kStampedeTrailSize = {55.0f, 2.5f};
  static constexpr float kStampedeTrailAlphaMultiplier = 0.7f;

  bool isTitleExiting_ = false;
  float titleExitTimer_ = 0.0f;
  Transform titleExitStartCamTransform_;

  // 15体ペンギン疾走演出
  std::unique_ptr<Sprite> titlePenguinSprites_[kTitlePenguinCount];
  int titlePenguinTextureHandle_ = -1;

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
  bool IsExiting() const { return isTitleExiting_; }
  float GetExitTimer() const { return titleExitTimer_; }
  float GetExitDuration() const { return kTitleExitDuration; }

  void UpdateExit(float dt);
  bool IsExitComplete(StageSettings *stageSettings) const;

  void UpdateAutoPilot(Player *player, StageSettings *stageSettings);
  void CheckCollisions(Player *player, StageSettings *stageSettings,
                       ScoreManager *scoreManager);
  void UpdateCamera(Player *player, GameCameraController *cameraController,
                    float dt);

  void DrawPenguins(Draw &draw);
  void ImGui();
};
}
bool IsInputAllowed() const {
  if (countdownMode_ == CountdownMode::None)
    return true;
  return countdownTimer_ >= (prepDuration_ + 3.0f * kCountdownStepDuration);
}

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

void DrawPenguins(Draw &draw);
void DrawCountdown(Draw &draw);
void ImGui();
}
;

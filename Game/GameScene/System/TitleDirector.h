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
public:
  static constexpr int kTitlePenguinCount = 15;
  static constexpr float kTitleExitDuration = 3.0f;

private:
  bool isTitleExiting_ = false;
  float titleExitTimer_ = 0.0f;
  Transform titleExitStartCamTransform_;

  // 15体ペンギン疾走演出
  std::unique_ptr<Sprite> titlePenguinSprites_[kTitlePenguinCount];
  int titlePenguinTextureHandle_ = -1;

  // オービットカメラ
  bool isTitleOrbitCamera_ = true;
  float titleCameraAngle_ = 0.0f;
  float titleCameraSpeed_ = 0.35f;
  float titleCameraRadius_ = 14.0f;
  float titleCameraHeight_ = 4.0f;
  float titleTargetOffsetY_ = 1.2f;

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

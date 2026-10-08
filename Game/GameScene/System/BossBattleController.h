#pragma once
#include <Math/Calculation.h>
#include <functional>

class Boss;
class StageSettings;
class GameSceneEffect;
class GameCameraController;

class BossBattleController {
private:
  float bossAttackTimer_ = 0.0f;
  float bossAttackSpawnZ_ = -35.0f;
  float bossAttackDropHeight_ = 18.0f;
  float bossAttackFallDuration_ = 28.0f;
  float bossAttackReflectMinZ_ = -15.0f;
  float bossReflectArcHeight_ = 7.0f;
  float bossReflectDuration_ = 36.0f;

  Vector3 bossCameraTranslate_{0.0f, 7.0f, 14.0f};
  Vector3 bossCameraRotate_{0.24f, 3.25f, 0.0f};

  int currentReflectLane_ = -1;

public:
  BossBattleController() = default;
  ~BossBattleController() = default;

  void Reset();
  void Update(float timeScale, Boss *boss, StageSettings *stageSettings,
              GameSceneEffect *effectManager);

  int GetReflectLane() const { return currentReflectLane_; }
  const Vector3 &GetBossCameraTranslate() const { return bossCameraTranslate_; }
  const Vector3 &GetBossCameraRotate() const { return bossCameraRotate_; }
  float GetReflectMinZ() const { return bossAttackReflectMinZ_; }

  void ImGui(Boss *boss, GameCameraController *cameraController,
             bool isPlayingBoss, bool isCameraTransitioning);
};

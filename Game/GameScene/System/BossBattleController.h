#pragma once
#include <Math/Calculation.h>
#include <functional>

class Boss;
class StageSettings;
class GameSceneEffect;
class GameCameraController;

class BossBattleController {
private:
  static constexpr float kDefaultAttackSpawnZ = -35.0f;
  static constexpr float kDefaultAttackDropHeight = 18.0f;
  static constexpr float kDefaultAttackFallDuration = 28.0f;
  static constexpr float kDefaultAttackReflectMinZ = -15.0f;
  static constexpr float kDefaultReflectArcHeight = 7.0f;
  static constexpr float kDefaultReflectDuration = 36.0f;
  static constexpr Vector3 kDefaultBossCameraTranslate{0.0f, 7.0f, 14.0f};
  static constexpr Vector3 kDefaultBossCameraRotate{0.24f, 3.25f, 0.0f};
  static constexpr int kNoReflectLane = -1;
  static constexpr float kAttackIntervalFrames = 120.0f;
  static constexpr int kLaneCount = 3;
  static constexpr float kBaseSpawnY = 2.0f;
  static constexpr float kHalfFactor = 0.5f;
  static constexpr float kBossAABBSize = 5.0f;
  static constexpr float kAttackReflectMaxZ = 15.0f;
  static constexpr float kReflectRumbleDuration = 0.18f;
  static constexpr unsigned short kReflectRumbleSpeed = 32000;
  static constexpr float kReflectHitMinProgress = 0.7f;
  static constexpr float kDefeatRumbleDuration = 0.5f;
  static constexpr unsigned short kMaxRumbleSpeed = 65535;

  float bossAttackTimer_ = 0.0f;
  float bossAttackSpawnZ_ = kDefaultAttackSpawnZ;
  float bossAttackDropHeight_ = kDefaultAttackDropHeight;
  float bossAttackFallDuration_ = kDefaultAttackFallDuration;
  float bossAttackReflectMinZ_ = kDefaultAttackReflectMinZ;
  float bossReflectArcHeight_ = kDefaultReflectArcHeight;
  float bossReflectDuration_ = kDefaultReflectDuration;

  Vector3 bossCameraTranslate_ = kDefaultBossCameraTranslate;
  Vector3 bossCameraRotate_ = kDefaultBossCameraRotate;

  int currentReflectLane_ = kNoReflectLane;

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

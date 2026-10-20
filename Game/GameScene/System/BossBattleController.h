#pragma once
#include <Entity/Obstacle.h>
#include <Math/Calculation.h>
#include <functional>
#include <vector>

class Boss;
class StageSettings;
class GameSceneEffect;
class GameCameraController;

class BossBattleController {
public:
  enum class PatternType {
    Standard,       // 基本：3レーン中 1安全, 1通常, 1反射
    JumpAction,     // アクション強制：低空ジャンプ回避 ＋ 追撃反射
    SlideAction,    // アクション強制：高空スライディング回避 ＋ 追撃反射
    SequentialWave, // 時間差ウェーブ：左→中→右 または 右→中→左 の連続落下
    DoubleWave,     // 2連波状攻撃：時間差で2波が落ちてくる
    Count
  };

  struct QueuedAttack {
    float delayFrames = 0.0f;
    int lane = 0; // 0:左, 1:中央, 2:右
    Obstacle::Type type = Obstacle::Type::BossAttack;
  };

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

  std::vector<QueuedAttack> attackQueue_;
  int lastPatternIndex_ = -1;
  PatternType currentPattern_ = PatternType::Standard;

private:
  void EnqueuePattern(PatternType pattern);
  void SpawnAttack(StageSettings *stageSettings, int lane, Obstacle::Type type);
  PatternType SelectNextPattern(int bossHP);

public:
  BossBattleController() = default;
  ~BossBattleController() = default;

  void Reset();
  void Update(float timeScale, Boss *boss, StageSettings *stageSettings,
              GameSceneEffect *effectManager,
              GameCameraController *cameraController = nullptr);

  void TriggerPattern(PatternType pattern) {
    attackQueue_.clear();
    EnqueuePattern(pattern);
  }

  int GetReflectLane() const { return currentReflectLane_; }
  const Vector3 &GetBossCameraTranslate() const { return bossCameraTranslate_; }
  void SetBossCameraTranslate(const Vector3 &tr) { bossCameraTranslate_ = tr; }
  const Vector3 &GetBossCameraRotate() const { return bossCameraRotate_; }
  void SetBossCameraRotate(const Vector3 &rot) { bossCameraRotate_ = rot; }
  float GetReflectMinZ() const { return bossAttackReflectMinZ_; }
  PatternType GetCurrentPattern() const { return currentPattern_; }
  size_t GetQueuedAttackCount() const { return attackQueue_.size(); }

  float GetAttackSpawnZ() const { return bossAttackSpawnZ_; }
  void SetAttackSpawnZ(float z) { bossAttackSpawnZ_ = z; }
  float GetAttackDropHeight() const { return bossAttackDropHeight_; }
  void SetAttackDropHeight(float h) { bossAttackDropHeight_ = h; }
  float GetAttackFallDuration() const { return bossAttackFallDuration_; }
  void SetAttackFallDuration(float d) { bossAttackFallDuration_ = d; }
  float GetAttackReflectMinZ() const { return bossAttackReflectMinZ_; }
  void SetAttackReflectMinZ(float z) { bossAttackReflectMinZ_ = z; }
  float GetReflectArcHeight() const { return bossReflectArcHeight_; }
  void SetReflectArcHeight(float h) { bossReflectArcHeight_ = h; }
  float GetReflectDuration() const { return bossReflectDuration_; }
  void SetReflectDuration(float d) { bossReflectDuration_ = d; }

  void ImGui(Boss *boss, GameCameraController *cameraController,
             bool isPlayingBoss, bool isCameraTransitioning);
};

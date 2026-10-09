#pragma once
#include "GameObject.h"
#include <Engine.h>
#include <memory>

enum class BossState {
  Appearance, // 出現
  Battle,     // 戦闘
  Defeat,     // 撃破
  Victory     // 勝利（プレイヤー敗北）
};

class Boss;

class IBossState {
public:
  virtual ~IBossState() = default;
  virtual BossState GetType() const = 0;
  virtual void Enter(Boss &/*boss*/) {}
  virtual void Update(Boss &boss, float speedMultiplier) = 0;
  virtual void Exit(Boss &/*boss*/) {}
};

class BossAppearanceState : public IBossState {
public:
  BossState GetType() const override { return BossState::Appearance; }
  void Enter(Boss &boss) override;
  void Update(Boss &boss, float speedMultiplier) override;
};

class BossBattleState : public IBossState {
public:
  BossState GetType() const override { return BossState::Battle; }
  void Enter(Boss &boss) override;
  void Update(Boss &boss, float speedMultiplier) override;
};

class BossDefeatState : public IBossState {
public:
  BossState GetType() const override { return BossState::Defeat; }
  void Enter(Boss &boss) override;
  void Update(Boss &boss, float speedMultiplier) override;
};

class BossVictoryState : public IBossState {
public:
  BossState GetType() const override { return BossState::Victory; }
  void Enter(Boss &boss) override;
  void Update(Boss &boss, float speedMultiplier) override;
};

class Boss : public GameObject {
private:
  friend class BossAppearanceState;
  friend class BossBattleState;
  friend class BossDefeatState;
  friend class BossVictoryState;

  static constexpr int kMaxHP = 5;
  static constexpr float kAppearanceDuration = 2.0f;
  static constexpr float kDefeatDuration = 3.0f;
  static constexpr float kDamageFlashDuration = 0.5f;
  static constexpr Vector3 kDefaultStartPos = {-18.0f, 20.0f, -40.0f};
  static constexpr Vector3 kDefaultTargetPos = {-6.0f, 3.0f, -2.0f};
  static constexpr Vector3 kDefaultBossScale = {5.0f, 5.0f, 5.0f};
  static constexpr Vector4 kDamageColor = {2.0f, 0.4f, 0.4f, 1.0f};
  static constexpr Vector4 kNormalColor = {1.0f, 1.0f, 1.0f, 1.0f};
  static constexpr float kMaxShakeIntensity = 2.0f;
  static constexpr float kShakeFreqX = 50.0f;
  static constexpr float kShakeFreqY = 65.0f;
  static constexpr float kShakeFreqZ = 40.0f;
  static constexpr float kBattleAnimSpeed = 0.05f;
  static constexpr float kBattleBobbingAmplitude = 1.0f;
  static constexpr float kSinkSpeed = 0.1f;
  static constexpr float kScaleOffsetFactor = 0.5f;
  static constexpr float kDefaultSpeedMultiplier = 1.0f;

private:
  std::unique_ptr<Model> model_ = std::make_unique<Model>();
  int hp_ = kMaxHP;
  bool isHit_ = false;
  float hitTimer_ = 0.0f;
  std::unique_ptr<IBossState> currentState_;
  float stateTimer_ = 0.0f;
  float battleAnimTimer_ = 0.0f;

  Vector3 startPos_;
  Vector3 targetPos_;

public:
  Boss();
  ~Boss();

  void Initialize(ModelData modelData);
  void Reset();
  void Spawn();
  void Spawn(float x, float y, float z);

  void Update(Matrix4x4 view, float speedMultiplier) override;
  void Update(Matrix4x4 view) { Update(view, kDefaultSpeedMultiplier); }
  void Draw(class Draw &draw) override;
  void ImGuiInnerComponents() override;

  bool HasMaterial() const override {
    return (model_ && model_->GetComponent<MaterialComponent>() != nullptr) ||
           GameObject::HasMaterial();
  }

  void OnDamage();
  int GetHP() const { return hp_; }
  int GetMaxHP() const { return kMaxHP; }

  BossState GetState() const;
  float GetStateTimer() const { return stateTimer_; }
  void ChangeState(BossState nextState);
  void ChangeState(std::unique_ptr<IBossState> nextState);

  Transform &GetTransformRef() { return transform_; }

  Vector3 GetTargetPos() const { return targetPos_; }
  void SetTargetPos(const Vector3 &targetPos) { targetPos_ = targetPos; }
  Vector3 &GetTargetPosRef() { return targetPos_; }

  const Vector3 &GetStartPos() const { return startPos_; }
  void SetStartPos(const Vector3 &startPos) { startPos_ = startPos; }

  float &GetBattleAnimTimerRef() { return battleAnimTimer_; }
};

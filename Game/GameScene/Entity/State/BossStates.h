#pragma once
#include "IBossState.h"

/// <summary>
/// ボス出現演出ステート
/// </summary>
class BossAppearanceState final : public IBossState {
private:
  static constexpr float kAppearanceDuration = 2.0f;
  static constexpr float kMaxShakeIntensity = 2.0f;
  static constexpr float kShakeFreqX = 50.0f;
  static constexpr float kShakeFreqY = 65.0f;
  static constexpr float kShakeFreqZ = 40.0f;

public:
  BossState GetType() const override { return BossState::Appearance; }
  void Enter(Boss &boss) override;
  std::unique_ptr<IBossState> Update(Boss &boss, float dt,
                                     float speedMultiplier) override;
};

/// <summary>
/// ボス戦闘中ステート
/// </summary>
class BossBattleState final : public IBossState {
private:
  static constexpr float kBattleAnimSpeed = 0.05f;
  static constexpr float kBattleBobbingAmplitude = 1.0f;
  float battleAnimTimer_ = 0.0f;

public:
  BossState GetType() const override { return BossState::Battle; }
  void Enter(Boss &boss) override;
  std::unique_ptr<IBossState> Update(Boss &boss, float dt,
                                     float speedMultiplier) override;
  std::unique_ptr<IBossState> OnDamage(Boss &boss) override;
};

/// <summary>
/// ボス撃破演出ステート
/// </summary>
class BossDefeatState final : public IBossState {
private:
  static constexpr float kDefeatDuration = 3.0f;
  static constexpr float kSinkSpeed = 0.1f;

public:
  BossState GetType() const override { return BossState::Defeat; }
  std::unique_ptr<IBossState> Update(Boss &boss, float dt,
                                     float speedMultiplier) override;
  std::unique_ptr<IBossState> OnDamage(Boss &boss) override;
};

/// <summary>
/// ボス勝利演出ステート（プレイヤー敗北時）
/// </summary>
class BossVictoryState final : public IBossState {
private:
  static constexpr float kRiseSpeed = 0.1f;

public:
  BossState GetType() const override { return BossState::Victory; }
  std::unique_ptr<IBossState> Update(Boss &boss, float dt,
                                     float speedMultiplier) override;
};

/// <summary>
/// BossState列挙値から対応するステートインスタンスを生成するファクトリ関数
/// </summary>
std::unique_ptr<IBossState> CreateBossState(BossState state);


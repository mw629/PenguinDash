#include "BossStates.h"
#include "../Boss.h"
#include "Math/Calculation.h"
#include <cmath>

// ==========================================
// IBossState デフォルト実装
// ==========================================

std::unique_ptr<IBossState> IBossState::OnDamage(Boss &boss) {
  boss.ApplyDamage();
  return nullptr;
}

// ==========================================
// BossAppearanceState
// ==========================================

void BossAppearanceState::Enter(Boss &boss) {
  IBossState::Enter(boss);
  Transform t = boss.GetTransform();
  t.rotate = {0.0f, 0.0f, 0.0f};
  boss.SetTransform(t);
}

std::unique_ptr<IBossState> BossAppearanceState::Update(Boss &boss, float dt,
                                                        float speedMultiplier) {
  stateTimer_ += dt * speedMultiplier;
  float t = stateTimer_ / kAppearanceDuration;
  if (t > 1.0f) {
    t = 1.0f;
  }

  // EaseOutQuad（だんだんゆっくり着地）
  float easeT = 1.0f - (1.0f - t) * (1.0f - t);
  const Vector3 &startPos = boss.GetStartPos();
  const Vector3 targetPos = boss.GetTargetPos();
  Transform transform = boss.GetTransform();

  transform.translate.x = startPos.x + (targetPos.x - startPos.x) * easeT;
  transform.translate.y = startPos.y + (targetPos.y - startPos.y) * easeT;
  transform.translate.z = startPos.z + (targetPos.z - startPos.z) * easeT;

  // 登場時は回転しながら降りてくる
  transform.rotate.y = (1.0f - easeT) * k2Pi;

  // 出現演出でシェイクさせる（だんだん揺れが収まる）
  float shakeIntensity = (1.0f - easeT) * kMaxShakeIntensity;
  transform.translate.x += std::sin(stateTimer_ * kShakeFreqX) * shakeIntensity;
  transform.translate.y += std::cos(stateTimer_ * kShakeFreqY) * shakeIntensity;
  transform.translate.z += std::sin(stateTimer_ * kShakeFreqZ) * shakeIntensity;

  if (stateTimer_ > kAppearanceDuration) {
    transform.rotate.y = 0.0f;
    boss.SetTransform(transform);
    return std::make_unique<BossBattleState>();
  }

  boss.SetTransform(transform);
  return nullptr;
}

// ==========================================
// BossBattleState
// ==========================================

void BossBattleState::Enter(Boss &boss) {
  IBossState::Enter(boss);
  battleAnimTimer_ = 0.0f;
  Transform t = boss.GetTransform();
  t.rotate.y = 0.0f;
  boss.SetTransform(t);
}

std::unique_ptr<IBossState> BossBattleState::Update(Boss &boss, float dt,
                                                    float speedMultiplier) {
  stateTimer_ += dt * speedMultiplier;
  battleAnimTimer_ += kBattleAnimSpeed * speedMultiplier;

  const Vector3 targetPos = boss.GetTargetPos();
  Transform transform = boss.GetTransform();
  transform.translate.x = targetPos.x;
  transform.translate.z = targetPos.z;
  transform.translate.y =
      targetPos.y + std::sin(battleAnimTimer_) * kBattleBobbingAmplitude;
  boss.SetTransform(transform);
  return nullptr;
}

std::unique_ptr<IBossState> BossBattleState::OnDamage(Boss &boss) {
  boss.ApplyDamage();
  if (boss.GetHP() <= 0) {
    return std::make_unique<BossDefeatState>();
  }
  return nullptr;
}

// ==========================================
// BossDefeatState
// ==========================================

std::unique_ptr<IBossState> BossDefeatState::Update(Boss &boss, float dt,
                                                    float speedMultiplier) {
  stateTimer_ += dt * speedMultiplier;
  Transform transform = boss.GetTransform();
  transform.translate.y -= kSinkSpeed * speedMultiplier;
  boss.SetTransform(transform);

  if (stateTimer_ > kDefeatDuration) {
    boss.SetIsActive(false);
  }
  return nullptr;
}

std::unique_ptr<IBossState> BossDefeatState::OnDamage(Boss & /*boss*/) {
  // 撃破後はダメージを受けない
  return nullptr;
}

// ==========================================
// BossVictoryState
// ==========================================

std::unique_ptr<IBossState> BossVictoryState::Update(Boss &boss, float dt,
                                                     float speedMultiplier) {
  stateTimer_ += dt * speedMultiplier;
  Transform transform = boss.GetTransform();
  transform.translate.y += kRiseSpeed * speedMultiplier;
  boss.SetTransform(transform);
  return nullptr;
}

// ==========================================
// Factory
// ==========================================

std::unique_ptr<IBossState> CreateBossState(BossState state) {
  switch (state) {
  case BossState::Appearance:
    return std::make_unique<BossAppearanceState>();
  case BossState::Battle:
    return std::make_unique<BossBattleState>();
  case BossState::Defeat:
    return std::make_unique<BossDefeatState>();
  case BossState::Victory:
    return std::make_unique<BossVictoryState>();
  default:
    return std::make_unique<BossAppearanceState>();
  }
}


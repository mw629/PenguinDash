#include "Boss.h"
#include "Graphics/Render/Draw.h"
#include "Math/Calculation.h"
#include <cmath>
#include <imgui.h>

Boss::Boss() {
  name_ = "Boss";
  isActive_ = false;
}

Boss::~Boss() {}

void Boss::Initialize(ModelData modelData) {
  model_->Initialize(modelData);
  model_->SetShader("ObjectShader");
  Reset();
}

// ==========================================
// Boss States Implementation
// ==========================================

void BossAppearanceState::Enter(Boss &boss) {
  boss.GetTransformRef().rotate = {0.0f, 0.0f, 0.0f};
}

void BossAppearanceState::Update(Boss &boss, float speedMultiplier) {
  (void)speedMultiplier;
  float stateTimer = boss.GetStateTimer();
  float t = stateTimer / Boss::kAppearanceDuration;
  if (t > 1.0f)
    t = 1.0f;

  // EaseOutQuad (だんだんゆっくりに)
  float easeT = 1.0f - (1.0f - t) * (1.0f - t);
  const Vector3 &startPos = boss.GetStartPos();
  const Vector3 &targetPos = boss.GetTargetPos();
  Transform &transform = boss.GetTransformRef();

  transform.translate.x = startPos.x + (targetPos.x - startPos.x) * easeT;
  transform.translate.y = startPos.y + (targetPos.y - startPos.y) * easeT;
  transform.translate.z = startPos.z + (targetPos.z - startPos.z) * easeT;

  // 登場時は回転しながら降りてくる
  transform.rotate.y = (1.0f - easeT) * k2Pi;

  // 出現演出でシェイクさせる (だんだん揺れが収まる)
  float shakeIntensity = (1.0f - easeT) * 2.0f;
  transform.translate.x += std::sin(stateTimer * 50.0f) * shakeIntensity;
  transform.translate.y += std::cos(stateTimer * 65.0f) * shakeIntensity;
  transform.translate.z += std::sin(stateTimer * 40.0f) * shakeIntensity;

  if (stateTimer > Boss::kAppearanceDuration) {
    transform.rotate.y = 0.0f;
    boss.ChangeState(BossState::Battle);
  }
}

void BossBattleState::Enter(Boss &boss) {
  boss.GetTransformRef().rotate.y = 0.0f;
}

void BossBattleState::Update(Boss &boss, float speedMultiplier) {
  boss.GetBattleAnimTimerRef() += 0.05f * speedMultiplier;
  const Vector3 &targetPos = boss.GetTargetPos();
  Transform &transform = boss.GetTransformRef();
  transform.translate.x = targetPos.x;
  transform.translate.z = targetPos.z;
  transform.translate.y = targetPos.y + std::sin(boss.GetBattleAnimTimerRef()) * 1.0f;
}

void BossDefeatState::Enter(Boss &boss) {
  (void)boss;
}

void BossDefeatState::Update(Boss &boss, float speedMultiplier) {
  boss.GetTransformRef().translate.y -= 0.1f * speedMultiplier;
  if (boss.GetStateTimer() > Boss::kDefeatDuration) {
    boss.SetIsActive(false);
  }
}

void BossVictoryState::Enter(Boss &boss) {
  (void)boss;
}

void BossVictoryState::Update(Boss &boss, float speedMultiplier) {
  boss.GetTransformRef().translate.y += 0.1f * speedMultiplier;
}

// ==========================================
// Boss Implementation
// ==========================================

void Boss::Reset() {
  isActive_ = false;
  hp_ = kMaxHP;
  isHit_ = false;
  hitTimer_ = 0.0f;
  stateTimer_ = 0.0f;
  battleAnimTimer_ = 0.0f;

  startPos_ = kDefaultStartPos;
  targetPos_ = kDefaultTargetPos;
  transform_.translate = startPos_;
  transform_.rotate = {0.0f, 0.0f, 0.0f};
  transform_.scale = kDefaultBossScale;

  ChangeState(BossState::Appearance);

  if (model_) {
    model_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
    if (model_->GetMaterial()) {
      model_->GetMaterial()->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
    }
    Transform drawTransform = transform_;
    drawTransform.translate.y -= transform_.scale.y * 0.5f;
    model_->SetTransform(drawTransform);
  }
}

void Boss::Spawn(float x, float y, float z) {
  Reset();
  targetPos_ = {x, y, z};
  isActive_ = true;
  ChangeState(BossState::Appearance);
}

BossState Boss::GetState() const {
  return currentState_ ? currentState_->GetType() : BossState::Appearance;
}

void Boss::ChangeState(BossState nextState) {
  switch (nextState) {
  case BossState::Appearance:
    ChangeState(std::make_unique<BossAppearanceState>());
    break;
  case BossState::Battle:
    ChangeState(std::make_unique<BossBattleState>());
    break;
  case BossState::Defeat:
    ChangeState(std::make_unique<BossDefeatState>());
    break;
  case BossState::Victory:
    ChangeState(std::make_unique<BossVictoryState>());
    break;
  }
}

void Boss::ChangeState(std::unique_ptr<IBossState> nextState) {
  if (currentState_) {
    currentState_->Exit(*this);
  }
  currentState_ = std::move(nextState);
  stateTimer_ = 0.0f;
  if (currentState_) {
    currentState_->Enter(*this);
  }
}

void Boss::Update(Matrix4x4 view, float speedMultiplier) {
  if (!isActive_)
    return;

  stateTimer_ += Engine::kDefaultDeltaTime * speedMultiplier; // おおよその時間を加算

  if (currentState_) {
    currentState_->Update(*this, speedMultiplier);
  }

  if (isHit_) {
    hitTimer_ -= Engine::kDefaultDeltaTime; // 約1フレーム分
    if (hitTimer_ <= 0.0f) {
      isHit_ = false;
      model_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
      model_->GetMaterial()->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
    }
  }

  Transform drawTransform = transform_;
  drawTransform.translate.y -= transform_.scale.y * 0.5f; // 底面原点(Y=0)を当たり判定中心に合わせるオフセット
  model_->SetTransform(drawTransform);
  model_->SettingWvp(view);
  GameObject::Update(view, speedMultiplier);
}

void Boss::Draw(class Draw &draw) {
  if (!isActive_)
    return;
  draw.DrawObj(model_.get());
  GameObject::Draw(draw);
}

void Boss::ImGuiInnerComponents() {
#ifdef _USE_IMGUI
  if (model_) {
    model_->ImGui(false);
  }

  const char *stateNames[] = {"Appearance", "Battle", "Defeat", "Victory"};
  ImGui::Text("Boss State: %s", stateNames[static_cast<int>(GetState())]);
  ImGui::Text("State Timer: %.2f", stateTimer_);

  if (ImGui::Button("Spawn Boss"))
    Spawn(kDefaultTargetPos.x, kDefaultTargetPos.y, kDefaultTargetPos.z);
  ImGui::SameLine();
  if (ImGui::Button("Reset Boss"))
    Reset();

  if (ImGui::Button("Test Defeat State"))
    ChangeState(BossState::Defeat);
  ImGui::SameLine();
  if (ImGui::Button("Test Victory State"))
    ChangeState(BossState::Victory);
#endif
}

void Boss::OnDamage() {
  if (GetState() == BossState::Defeat)
    return; // 撃破後はダメージを受けない

  hp_--;
  isHit_ = true;
  hitTimer_ = kDamageFlashDuration; // ダメージ演出時間
  model_->SetColor(kDamageColor); // 赤く光る
  model_->GetMaterial()->SetColor(kDamageColor);

  if (hp_ <= 0 && GetState() == BossState::Battle) {
    ChangeState(BossState::Defeat);
  }
}

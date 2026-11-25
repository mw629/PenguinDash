#include "Boss.h"
#include "Graphics/Render/Draw.h"
#include "State/BossStates.h"
#include <imgui.h>

Boss::Boss() {
  name_ = "Boss";
  isActive_ = false;
}

Boss::~Boss() {}

void Boss::Initialize(ModelData modelData) {
  model_->Initialize(modelData);
  model_->SetShader("ObjectShader");
  model_->SetLighting(lightingEnabled_);
  Reset();
}

void Boss::SetLighting(bool enabled) {
  lightingEnabled_ = enabled;
  if (model_) {
    model_->SetLighting(enabled);
  }
}

void Boss::Reset() {
  isActive_ = false;
  hp_ = kMaxHP;
  isHit_ = false;
  hitTimer_ = 0.0f;

  startPos_ = kDefaultStartPos;
  targetPos_ = kDefaultTargetPos;
  transform_.translate = startPos_;
  transform_.rotate = {0.0f, 0.0f, 0.0f};
  transform_.scale = kDefaultBossScale;

  ChangeState(BossState::Appearance);

  if (model_) {
    model_->SetColor(kNormalColor);
    if (model_->GetMaterial()) {
      model_->GetMaterial()->SetColor(kNormalColor);
    }
    Transform drawTransform = transform_;
    drawTransform.translate.y -= transform_.scale.y * kScaleOffsetFactor;
    model_->SetTransform(drawTransform);
  }
}

void Boss::Spawn() {
  Spawn(kDefaultTargetPos.x, kDefaultTargetPos.y, kDefaultTargetPos.z);
}

void Boss::Spawn(float x, float y, float z) {
  Reset();
  targetPos_ = {x, y, z};
  isActive_ = true;
}

BossState Boss::GetState() const {
  return currentState_ ? currentState_->GetType() : BossState::Appearance;
}

void Boss::ChangeState(BossState nextState) {
  ChangeState(CreateBossState(nextState));
}

void Boss::ChangeState(std::unique_ptr<IBossState> nextState) {
  if (currentState_) {
    currentState_->Exit(*this);
  }
  currentState_ = std::move(nextState);
  if (currentState_) {
    currentState_->Enter(*this);
  }
}

void Boss::Update(Matrix4x4 view, float speedMultiplier) {
  if (!isActive_) {
    return;
  }

  if (currentState_) {
    auto nextState =
        currentState_->Update(*this, Engine::kDefaultDeltaTime, speedMultiplier);
    if (nextState) {
      ChangeState(std::move(nextState));
    }
  }

  if (isHit_) {
    hitTimer_ -= Engine::kDefaultDeltaTime; // 約1フレーム分
    if (hitTimer_ <= 0.0f) {
      isHit_ = false;
      model_->SetColor(kNormalColor);
      if (model_->GetMaterial()) {
        model_->GetMaterial()->SetColor(kNormalColor);
      }
    }
  }

  Transform drawTransform = transform_;
  drawTransform.translate.y -=
      transform_.scale.y *
      kScaleOffsetFactor; // 底面原点(Y=0)を当たり判定中心に合わせるオフセット
  model_->SetTransform(drawTransform);
  model_->SettingWvp(view);
  GameObject::Update(view, speedMultiplier);
}

void Boss::Draw(class Draw &draw) {
  if (!isActive_) {
    return;
  }
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
  ImGui::Text("State Timer: %.2f", GetStateTimer());

  if (ImGui::Button("Spawn Boss")) {
    Spawn();
  }
  ImGui::SameLine();
  if (ImGui::Button("Reset Boss")) {
    Reset();
  }

  if (ImGui::Button("Test Defeat State")) {
    ChangeState(BossState::Defeat);
  }
  ImGui::SameLine();
  if (ImGui::Button("Test Victory State")) {
    ChangeState(BossState::Victory);
  }
#endif
}

void Boss::ApplyDamage() {
  hp_--;
  isHit_ = true;
  hitTimer_ = kDamageFlashDuration; // ダメージ演出時間
  model_->SetColor(kDamageColor);   // 赤く光る
  if (model_->GetMaterial()) {
    model_->GetMaterial()->SetColor(kDamageColor);
  }
}

void Boss::OnDamage() {
  if (!currentState_) {
    return;
  }
  if (auto nextState = currentState_->OnDamage(*this)) {
    ChangeState(std::move(nextState));
  }
}

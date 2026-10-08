#include "Boss.h"
#include "Graphics/Render/Draw.h"
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

void Boss::Reset() {
  isActive_ = false;
  hp_ = kMaxHP;
  isHit_ = false;
  hitTimer_ = 0.0f;
  state_ = BossState::Appearance;
  stateTimer_ = 0.0f;
  battleAnimTimer_ = 0.0f;

  startPos_ = {-18.0f, 20.0f, -40.0f};
  targetPos_ = {-6.0f, 3.0f, -2.0f};
  transform_.translate = startPos_;
  transform_.rotate = {0.0f, 0.0f, 0.0f};
  transform_.scale = {5.0f, 5.0f, 5.0f};

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

void Boss::ChangeState(BossState nextState) {
  state_ = nextState;
  stateTimer_ = 0.0f;
}

void Boss::Update(Matrix4x4 view, float speedMultiplier) {
  if (!isActive_)
    return;

  stateTimer_ += 0.016f * speedMultiplier; // おおよその時間を加算

  switch (state_) {
  case BossState::Appearance: {
    // 出現時の演出（左奥から所定の位置へ飛んでくる）
    float t = stateTimer_ / 2.0f; // 2秒かけて登場
    if (t > 1.0f)
      t = 1.0f;

    // EaseOutQuad (だんだんゆっくりに)
    float easeT = 1.0f - (1.0f - t) * (1.0f - t);

    transform_.translate.x = startPos_.x + (targetPos_.x - startPos_.x) * easeT;
    transform_.translate.y = startPos_.y + (targetPos_.y - startPos_.y) * easeT;
    transform_.translate.z = startPos_.z + (targetPos_.z - startPos_.z) * easeT;

    // 登場時は回転しながら降りてくる
    transform_.rotate.y = (1.0f - easeT) * 3.14159f * 2.0f;

    // 出現演出でシェイクさせる (だんだん揺れが収まる)
    float shakeIntensity = (1.0f - easeT) * 2.0f;
    transform_.translate.x += std::sin(stateTimer_ * 50.0f) * shakeIntensity;
    transform_.translate.y += std::cos(stateTimer_ * 65.0f) * shakeIntensity;
    transform_.translate.z += std::sin(stateTimer_ * 40.0f) * shakeIntensity;

    if (stateTimer_ > 2.0f) {
      transform_.rotate.y = 0.0f;
      ChangeState(BossState::Battle);
    }
    break;
  }
  case BossState::Battle: {
    // ふわふわ浮かぶアニメーション
    battleAnimTimer_ += 0.05f * speedMultiplier;
    transform_.translate.x = targetPos_.x;
    transform_.translate.z = targetPos_.z;
    transform_.translate.y = targetPos_.y + std::sin(battleAnimTimer_) * 1.0f;
    break;
  }
  case BossState::Defeat:
    // 撃破時の演出（落下して消えるなど）
    transform_.translate.y -= 0.1f * speedMultiplier;
    if (stateTimer_ > 3.0f) {
      isActive_ = false; // 演出が終わったら非アクティブに
    }
    break;

  case BossState::Victory:
    // ボス勝利時（プレイヤー敗北時）の演出（高笑い、飛び去るなど）
    transform_.translate.y += 0.1f * speedMultiplier;
    break;
  }

  if (isHit_) {
    hitTimer_ -= 0.016f; // 約1フレーム分
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
  ImGui::Text("Boss State: %s", stateNames[static_cast<int>(state_)]);
  ImGui::Text("State Timer: %.2f", stateTimer_);

  if (ImGui::Button("Spawn Boss"))
    Spawn(-6.0f, 3.0f, -2.0f);
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
  if (state_ == BossState::Defeat)
    return; // 撃破後はダメージを受けない

  hp_--;
  isHit_ = true;
  hitTimer_ = 0.5f; // 0.5秒間ダメージ演出
  model_->SetColor({2.0f, 0.4f, 0.4f, 1.0f}); // 赤く光る
  model_->GetMaterial()->SetColor({2.0f, 0.4f, 0.4f, 1.0f});

  if (hp_ <= 0 && state_ == BossState::Battle) {
    ChangeState(BossState::Defeat);
  }
}

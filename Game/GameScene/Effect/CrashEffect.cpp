#include "CrashEffect.h"
#include "Graphics/Render/Draw.h"
#include <algorithm>
#include <cmath>
#include <imgui.h>

void CrashEffect::Initialize(Texture *texture) {
  if (!texture) return;
  crashTextureHandle_ = texture->CreateTexture("Resources/Texture/Crash.png");
  crashSpriteData_.transform.scale = {0.0f, 0.0f, 1.0f};
  crashSpriteData_.transform.translate = {crashPosition_.x, crashPosition_.y, 0.0f};
  crashSpriteData_.transform.rotate = {0.0f, 0.0f, 0.0f};
  crashSpriteData_.size = crashBaseSize_;
  crashSpriteData_.pivot = kCenterPivot;
  crashSpriteData_.textureArea[0] = {0.0f, 0.0f};
  crashSpriteData_.textureArea[1] = {1.0f, 1.0f};
  crashSpriteData_.scaleMode = SpriteScaleMode::Fit;
  crashSpriteData_.anchor = SpriteAnchor::None;

  crashSprite_ = std::make_unique<Sprite>();
  crashSprite_->Initialize(crashSpriteData_, crashTextureHandle_);
  crashSprite_->GetMaterial()->SetColor(kDefaultSpriteColor);
  crashSprite_->SettingWvp();
}

void CrashEffect::Trigger() {
  crashTimer_ = 0.0f;
  if (crashSprite_) {
    Transform ct = crashSpriteData_.transform;
    ct.scale = {0.0f, 0.0f, 1.0f};
    ct.translate = {crashPosition_.x, crashPosition_.y, 0.0f};
    crashSprite_->SetTransform(ct);
    crashSprite_->SettingWvp();
  }
}

void CrashEffect::Reset() {
  crashTimer_ = 0.0f;
  if (crashSprite_) {
    Transform ct = crashSpriteData_.transform;
    ct.scale = {0.0f, 0.0f, 1.0f};
    ct.translate = {crashPosition_.x, crashPosition_.y, 0.0f};
    crashSprite_->SetTransform(ct);
    crashSprite_->SettingWvp();
  }
}

void CrashEffect::Update(float dt) {
  crashTimer_ += dt;
  if (crashSprite_) {
    float t = std::clamp(crashTimer_ / crashScaleDuration_, 0.0f, 1.0f);
    // EaseOutBack
    const float c1 = kEaseOutBackOvershoot;
    const float c3 = c1 + 1.0f;
    float s = (t >= 1.0f) ? 1.0f : (1.0f + c3 * std::pow(t - 1.0f, 3.0f) + c1 * std::pow(t - 1.0f, 2.0f));
    if (s < 0.0f) s = 0.0f;

    Transform ct = crashSpriteData_.transform;
    ct.scale = {s, s, 1.0f};
    ct.translate = {crashPosition_.x, crashPosition_.y, 0.0f};
    crashSprite_->SetTransform(ct);
    crashSprite_->SettingWvp();
  }
}

void CrashEffect::Render(Draw &draw) {
  if (crashSprite_) {
    draw.DrawSprite(crashSprite_.get());
  }
}

void CrashEffect::ImGui() {
#ifdef _USE_IMGUI
  if (ImGui::TreeNode("クラッシュ演出設定")) {
    ImGui::DragFloat2("画面位置", &crashPosition_.x, 1.0f, 0.0f, 1280.0f);
    ImGui::DragFloat("拡大演出持続時間", &crashScaleDuration_, 0.02f, 0.05f, 2.0f);
    if (ImGui::DragFloat2("基本サイズ", &crashBaseSize_.x, 1.0f, 50.0f, 1280.0f)) {
      if (crashSprite_) {
        crashSprite_->SetSize(crashBaseSize_);
        crashSprite_->UpdateVertexBuffer();
      }
    }
    ImGui::TreePop();
  }
#endif
}


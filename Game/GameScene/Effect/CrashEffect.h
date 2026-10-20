#pragma once
#include <Engine.h>
#include <GameObjects/Object/2d/Sprite.h>
#include <Math/Calculation.h>
#include <memory>

class CrashEffect {
private:
  static constexpr float kDefaultScaleDuration = 0.35f;
  static constexpr Vector2 kDefaultPosition = {640.0f, 320.0f};
  static constexpr Vector2 kDefaultBaseSize = {580.0f, 580.0f * (569.0f / 1024.0f)};
  static constexpr Vector4 kDefaultSpriteColor = {1.0f, 1.0f, 1.0f, 1.0f};
  static constexpr Vector2 kCenterPivot = {0.5f, 0.5f};
  static constexpr float kEaseOutBackOvershoot = 1.70158f;

  std::unique_ptr<Sprite> crashSprite_;
  SpriteData crashSpriteData_;
  int crashTextureHandle_ = -1;
  float crashTimer_ = 0.0f;
  float crashScaleDuration_ = kDefaultScaleDuration;
  Vector2 crashBaseSize_ = kDefaultBaseSize;
  Vector2 crashPosition_ = kDefaultPosition;

public:
  CrashEffect() = default;
  ~CrashEffect() = default;

  void Initialize(class Texture *texture);
  void Trigger();
  void Reset();
  void Update(float dt);
  void Update() { Update(Engine::kDefaultDeltaTime); }
  void Render(class Draw &draw);
  void ImGui();

  Sprite *GetSprite() const { return crashSprite_.get(); }
  const Vector2 &GetCrashPosition() const { return crashPosition_; }
  void SetCrashPosition(const Vector2 &pos) { crashPosition_ = pos; }
  float GetCrashScaleDuration() const { return crashScaleDuration_; }
  void SetCrashScaleDuration(float dur) { crashScaleDuration_ = dur; }
  const Vector2 &GetCrashBaseSize() const { return crashBaseSize_; }
  void SetCrashBaseSize(const Vector2 &size) {
    crashBaseSize_ = size;
    if (crashSprite_) {
      crashSprite_->SetSize(crashBaseSize_);
      crashSprite_->UpdateVertexBuffer();
    }
  }
};

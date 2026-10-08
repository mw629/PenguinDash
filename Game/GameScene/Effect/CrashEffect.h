#pragma once
#include <Engine.h>
#include <GameObjects/Object/2d/Sprite.h>
#include <Math/Calculation.h>
#include <memory>

class CrashEffect {
private:
  std::unique_ptr<Sprite> crashSprite_;
  SpriteData crashSpriteData_;
  int crashTextureHandle_ = -1;
  float crashTimer_ = 0.0f;
  float crashScaleDuration_ = 0.35f;
  Vector2 crashBaseSize_ = {580.0f, 580.0f * (569.0f / 1024.0f)};
  Vector2 crashPosition_ = {640.0f, 320.0f};

public:
  CrashEffect() = default;
  ~CrashEffect() = default;

  void Initialize(class Texture *texture);
  void Trigger();
  void Reset();
  void Update(float dt = 1.0f / 60.0f);
  void Render(class Draw &draw);
  void ImGui();

  Sprite *GetSprite() const { return crashSprite_.get(); }
};

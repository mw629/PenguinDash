#pragma once
#include "HexBarrier.h"
#include <Emitter.h>
#include <Engine.h>
#include <memory>

enum class BarrierEffectState { Inactive, Deploying, Active, Breaking };

class GameSceneEffect {
public:
  GameSceneEffect();
  ~GameSceneEffect() = default;

  void Initialize();

  void PlayingUpdate(const Matrix4x4 &view, const Vector3 &playerPos);
  void PlayerHitUpdate(const Matrix4x4 &view);
  void EditorUpdate(const Matrix4x4 &view);

  void EmitDust(const Vector3 &playerPos);

  Emitter* GetSnowEffect() const { return snowEffect_.get(); }

  // バリア演出
  void EmitBarrier(const Vector3 &playerPos);
  void BreakBarrier(const Vector3 &playerPos);
  void ClearBarrier();
  bool IsBarrierActive() const;

  void Draw(class Draw &draw);
  void ImGui();

  // バリアパラメータ設定・取得
  const Vector3 &GetBarrierOffset() const { return barrierOffset_; }
  void SetBarrierOffset(const Vector3 &offset) { barrierOffset_ = offset; }
  const Vector3 &GetBarrierBaseScale() const { return barrierBaseScale_; }
  void SetBarrierBaseScale(const Vector3 &scale) { barrierBaseScale_ = scale; }
  const Vector4 &GetBarrierBaseColor() const { return barrierBaseColor_; }
  void SetBarrierBaseColor(const Vector4 &color) { barrierBaseColor_ = color; }
  const Vector4 &GetBarrierFlashColor() const { return barrierFlashColor_; }
  void SetBarrierFlashColor(const Vector4 &color) { barrierFlashColor_ = color; }
  float GetBarrierRadius() const { return barrier_ ? barrier_->GetRadius() : 0.5f; }
  void SetBarrierRadius(float r) { if (barrier_) barrier_->SetRadius(r); }
  bool GetBarrierPointyTopped() const { return barrier_ ? barrier_->GetPointyTopped() : false; }
  void SetBarrierPointyTopped(bool p) { if (barrier_) barrier_->SetPointyTopped(p); }
  int GetBarrierShape() const { return barrier_ ? static_cast<int>(barrier_->GetShape()) : 1; }
  void SetBarrierShape(int s) { if (barrier_) barrier_->SetShape(static_cast<HexBarrierShape>(s)); }

private:
  static constexpr float kDefaultBarrierDeployDuration = 0.25f;
  static constexpr float kDefaultBarrierBreakDuration = 0.35f;
  static constexpr Vector3 kDefaultBarrierOffset = {0.0f, 0.0f, 1.2f};
  static constexpr Vector3 kDefaultBarrierBaseScale = {0.6f, 0.6f, 0.6f};
  static constexpr Vector4 kDefaultBarrierBaseColor = {0.25f, 0.8f, 1.0f, 0.85f};
  static constexpr Vector4 kDefaultBarrierFlashColor = {1.0f, 1.0f, 1.0f, 0.95f};
  static constexpr float kDefaultHexRadius = 0.5f;
  static constexpr float kDustYOffset = 0.8f;
  static constexpr float kSnowPosScaleX = 0.2f;
  static constexpr float kSnowPosY = 9.0f;
  static constexpr float kSnowPosZOffset = 5.0f;
  static constexpr float kDeployOvershootStrength = 0.25f;
  static constexpr float kPulseScaleAmplitude = 0.035f;
  static constexpr float kPulseFrequency = 4.0f;
  static constexpr float kPulseBrightnessBase = 0.85f;
  static constexpr float kPulseBrightnessAmplitude = 0.15f;
  static constexpr float kBreakExpandStrength = 0.9f;
  static constexpr float kBreakFlashThreshold = 0.15f;

  void UpdateBarrier(const Matrix4x4 &view, const Vector3 &playerPos, float deltaTime);
  void UpdateBarrier(const Matrix4x4 &view, const Vector3 &playerPos) { UpdateBarrier(view, playerPos, Engine::kDefaultDeltaTime); }

  std::unique_ptr<Emitter> dustEffect_;
  std::unique_ptr<Emitter> snowEffect_;

  // バリア
  std::unique_ptr<HexBarrier> barrier_;
  BarrierEffectState barrierState_ = BarrierEffectState::Inactive;
  float barrierDeployTimer_ = 0.0f;
  float barrierDeployDuration_ = kDefaultBarrierDeployDuration;
  float barrierBreakTimer_ = 0.0f;
  float barrierBreakDuration_ = kDefaultBarrierBreakDuration;
  float barrierPulseTimer_ = 0.0f;
  Vector3 barrierOffset_ = kDefaultBarrierOffset;
  Vector3 barrierBaseScale_ = kDefaultBarrierBaseScale;
  Vector4 barrierBaseColor_ = kDefaultBarrierBaseColor;
  Vector4 barrierFlashColor_ = kDefaultBarrierFlashColor;
  Vector3 lastPlayerPos_ = {0.0f, 0.0f, 0.0f};
};

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

private:
  void UpdateBarrier(const Matrix4x4 &view, const Vector3 &playerPos,
                     float deltaTime = 1.0f / 60.0f);

  std::unique_ptr<Emitter> dustEffect_;
  std::unique_ptr<Emitter> snowEffect_;

  // バリア
  std::unique_ptr<HexBarrier> barrier_;
  BarrierEffectState barrierState_ = BarrierEffectState::Inactive;
  float barrierDeployTimer_ = 0.0f;
  float barrierDeployDuration_ = 0.25f;
  float barrierBreakTimer_ = 0.0f;
  float barrierBreakDuration_ = 0.35f;
  float barrierPulseTimer_ = 0.0f;
  Vector3 barrierOffset_ = {0.0f, 0.0f, 1.2f};
  Vector3 barrierBaseScale_ = {0.6f, 0.6f, 0.6f};
  Vector4 barrierBaseColor_ = {0.25f, 0.8f, 1.0f, 0.85f};
  Vector4 barrierFlashColor_ = {1.0f, 1.0f, 1.0f, 0.95f};
  Vector3 lastPlayerPos_ = {0.0f, 0.0f, 0.0f};
};

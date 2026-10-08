#include "GameSceneEffect.h"
#include "Graphics/Render/Draw.h"
#include <cmath>
#include <cstdlib>
#include <imgui.h>

GameSceneEffect::GameSceneEffect() {
  dustEffect_ = std::make_unique<Emitter>();
  snowEffect_ = std::make_unique<Emitter>();
  barrier_ = std::make_unique<HexBarrier>();
}

void GameSceneEffect::Initialize() {
  // 六角形バリアの初期化
  if (barrier_) {
    barrier_->Initialize("Resources/Texture/white64x64.png");
    barrier_->SetRadius(0.5f);
    barrier_->SetShape(HexBarrierShape::Honeycomb);
  }

  dustEffect_->Initialize();
  dustEffect_->LoadFromJson("Dustparticle");
  dustEffect_->name_ = "Dust Effect";
  dustEffect_->generatorBehavior = nullptr; // JSONの設定に完全に従う
  dustEffect_->SetStop(
      true); // 自動発生を停止（スクリプトから手動でのみEmitする）
  strncpy_s(dustEffect_->saveFileName_, sizeof(dustEffect_->saveFileName_),
            "Dustparticle", _TRUNCATE);

  // 雪パーティクルの初期化
  snowEffect_->Initialize();
  snowEffect_->LoadFromJson("Snowparticle");
  snowEffect_->name_ = "Snow Effect";
  snowEffect_->generatorBehavior = nullptr; // JSONの設定に従う
  snowEffect_->SetStop(false);              // 常時自動発生
  snowEffect_->SetLoop(true);
  strncpy_s(snowEffect_->saveFileName_, sizeof(snowEffect_->saveFileName_),
            "Snowparticle", _TRUNCATE);
}

void GameSceneEffect::PlayingUpdate(const Matrix4x4 &view,
                                    const Vector3 &playerPos) {
  UpdateBarrier(view, playerPos);
  if (dustEffect_) {
    dustEffect_->Update(view);
  }
  if (snowEffect_) {
    Vector3 snowPos = {playerPos.x * 0.2f, 9.0f, playerPos.z + 5.0f};
    snowEffect_->SetPosition(snowPos);
    snowEffect_->Update(view);
  }
}

void GameSceneEffect::PlayerHitUpdate(const Matrix4x4 &view) {
  UpdateBarrier(view, lastPlayerPos_);
  if (dustEffect_) {
    dustEffect_->Update(view);
  }
  if (snowEffect_) {
    snowEffect_->Update(view);
  }
}

void GameSceneEffect::EditorUpdate(const Matrix4x4 &view) {
  UpdateBarrier(view, lastPlayerPos_);
  if (dustEffect_) {
    dustEffect_->SettingWvp(view);
  }
  if (snowEffect_) {
    snowEffect_->SettingWvp(view);
  }
}

void GameSceneEffect::EmitDust(const Vector3 &playerPos) {
  EmitterData ed = dustEffect_->GetEmitterData();
  ed.transform.translate = playerPos;
  ed.transform.translate.y -= 0.8f; // ペンギンの足元（地面Y=2.0付近）に発生させる
  dustEffect_->SetEmitterData(ed);
  dustEffect_->Emit();
}



void GameSceneEffect::EmitBarrier(const Vector3 &playerPos) {
  barrierState_ = BarrierEffectState::Deploying;
  barrierDeployTimer_ = 0.0f;
  barrierPulseTimer_ = 0.0f;
  lastPlayerPos_ = playerPos;
}

void GameSceneEffect::BreakBarrier(const Vector3 &playerPos) {
  barrierState_ = BarrierEffectState::Breaking;
  barrierBreakTimer_ = 0.0f;
  lastPlayerPos_ = playerPos;
}

void GameSceneEffect::ClearBarrier() {
  barrierState_ = BarrierEffectState::Inactive;
}

bool GameSceneEffect::IsBarrierActive() const {
  return barrierState_ != BarrierEffectState::Inactive;
}

void GameSceneEffect::UpdateBarrier(const Matrix4x4 &view,
                                    const Vector3 &playerPos, float deltaTime) {
  if (!barrier_ || barrierState_ == BarrierEffectState::Inactive) {
    return;
  }

  lastPlayerPos_ = playerPos;

  Vector3 currentScale = barrierBaseScale_;
  Vector4 currentColor = barrierBaseColor_;

  if (barrierState_ == BarrierEffectState::Deploying) {
    barrierDeployTimer_ += deltaTime;
    float t =
        std::clamp(barrierDeployTimer_ / barrierDeployDuration_, 0.0f, 1.0f);
    // EaseOutBack風の弾む展開アニメーション
    float s = std::sin(t * 1.5707963f);
    float overshoot = 1.0f + 0.25f * std::sin(t * 3.14159265f);
    float scaleFactor = s * overshoot;
    currentScale.x *= scaleFactor;
    currentScale.y *= scaleFactor;
    currentScale.z *= scaleFactor;

    // 展開時は白フラッシュからベースカラーへ移行
    currentColor.x =
        barrierFlashColor_.x * (1.0f - t) + barrierBaseColor_.x * t;
    currentColor.y =
        barrierFlashColor_.y * (1.0f - t) + barrierBaseColor_.y * t;
    currentColor.z =
        barrierFlashColor_.z * (1.0f - t) + barrierBaseColor_.z * t;
    currentColor.w = barrierBaseColor_.w;

    if (barrierDeployTimer_ >= barrierDeployDuration_) {
      barrierState_ = BarrierEffectState::Active;
    }
  } else if (barrierState_ == BarrierEffectState::Active) {
    barrierPulseTimer_ += deltaTime;
    // 呼吸のようなパルス（拡縮と発光明滅）
    float pulse = 1.0f + 0.035f * std::sin(barrierPulseTimer_ * 4.0f);
    float bright = 0.85f + 0.15f * std::sin(barrierPulseTimer_ * 4.0f);

    currentScale.x *= pulse;
    currentScale.y *= pulse;
    currentScale.z *= pulse;

    currentColor.x = barrierBaseColor_.x * bright;
    currentColor.y = barrierBaseColor_.y * bright;
    currentColor.z = barrierBaseColor_.z * bright;
    currentColor.w = barrierBaseColor_.w;
  } else if (barrierState_ == BarrierEffectState::Breaking) {
    barrierBreakTimer_ += deltaTime;
    float t =
        std::clamp(barrierBreakTimer_ / barrierBreakDuration_, 0.0f, 1.0f);

    // 急速に拡大しながらフェードアウト
    float expand = 1.0f + 0.9f * t;
    currentScale.x *= expand;
    currentScale.y *= expand;
    currentScale.z *= expand;

    // 最初の一瞬はフラッシュ、その後減衰
    float fade = (1.0f - t > 0.0f) ? (1.0f - t) : 0.0f;
    if (t < 0.15f) {
      currentColor = barrierFlashColor_;
    } else {
      currentColor.x = barrierBaseColor_.x * fade;
      currentColor.y = barrierBaseColor_.y * fade;
      currentColor.z = barrierBaseColor_.z * fade;
      currentColor.w = barrierBaseColor_.w * fade;
    }

    if (barrierBreakTimer_ >= barrierBreakDuration_) {
      barrierState_ = BarrierEffectState::Inactive;
    }
  }

  // マテリアルカラーを更新
  if (auto mat = barrier_->GetComponent<MaterialComponent>()) {
    if (auto factory = mat->GetMaterialFactory()) {
      factory->SetColor(currentColor);
    }
  }

  // プレイヤーの前方に配置
  Transform barrierTransform;
  barrierTransform.scale = currentScale;
  barrierTransform.rotate = {0.0f, 0.0f, 0.0f};
  barrierTransform.translate = {playerPos.x + barrierOffset_.x,
                                playerPos.y + barrierOffset_.y,
                                playerPos.z + barrierOffset_.z};

  barrier_->SetTransform(barrierTransform);
  barrier_->SettingWvp(view);
}

void GameSceneEffect::Draw(class Draw &draw) {
  if (dustEffect_) {
    dustEffect_->Draw(draw);
  }

  // 雪パーティクルの描画
  if (snowEffect_) {
    snowEffect_->Draw(draw);
  }

  // バリア描画
  if (barrier_ && barrierState_ != BarrierEffectState::Inactive) {
    draw.DrawObj(barrier_.get());
  }
}

void GameSceneEffect::ImGui() {
#ifdef _USE_IMGUI
  if (ImGui::CollapsingHeader("Barrier Effect")) {
    const char *stateNames[] = {"Inactive", "Deploying", "Active", "Breaking"};
    ImGui::Text("Current State: %s",
                stateNames[static_cast<int>(barrierState_)]);

    if (ImGui::Button("Emit Barrier")) {
      EmitBarrier(lastPlayerPos_);
    }
    ImGui::SameLine();
    if (ImGui::Button("Break Barrier")) {
      BreakBarrier(lastPlayerPos_);
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear Barrier")) {
      ClearBarrier();
    }

    ImGui::Separator();
    ImGui::DragFloat3("Barrier Offset", &barrierOffset_.x, 0.05f);
    ImGui::DragFloat3("Barrier Scale", &barrierBaseScale_.x, 0.05f, 0.1f,
                      10.0f);
    ImGui::ColorEdit4("Barrier Color", &barrierBaseColor_.x);
    ImGui::ColorEdit4("Flash Color", &barrierFlashColor_.x);

    if (barrier_) {
      float r = barrier_->GetRadius();
      if (ImGui::DragFloat("Hex Radius", &r, 0.02f, 0.1f, 5.0f)) {
        barrier_->SetRadius(r);
      }

      bool pointy = barrier_->GetPointyTopped();
      if (ImGui::Checkbox("Pointy Topped", &pointy)) {
        barrier_->SetPointyTopped(pointy);
      }

      int shapeType = static_cast<int>(barrier_->GetShape());
      const char *shapes[] = {"Single", "Honeycomb (7-cell)"};
      if (ImGui::Combo("Shape", &shapeType, shapes, IM_ARRAYSIZE(shapes))) {
        barrier_->SetShape(static_cast<HexBarrierShape>(shapeType));
      }

      if (auto mat = barrier_->GetComponent<MaterialComponent>()) {
        int blendMode = static_cast<int>(mat->GetBlend());
        const char *blendNames[] = {"None",     "Normal",   "Add",
                                    "Subtract", "Multiply", "Screen"};
        if (ImGui::Combo("Blend Mode", &blendMode, blendNames,
                         IM_ARRAYSIZE(blendNames))) {
          mat->SetBlend(static_cast<BlendMode>(blendMode));
        }
      }
    }
  }

  if (ImGui::CollapsingHeader("Particles")) {
    if (dustEffect_)
      dustEffect_->ImGui();
    if (snowEffect_)
      snowEffect_->ImGui();
  }
#endif
}

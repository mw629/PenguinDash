#include "GameSceneEffect.h"
#include "Graphics/Render/Draw.h"
#include <cmath>
#include <cstdlib>
#include <imgui.h>

GameSceneEffect::GameSceneEffect() {
  dustEffect_ = std::make_unique<Emitter>();
  snowEffect_ = std::make_unique<Emitter>();
  diamondDustEffect_ = std::make_unique<Emitter>();
  jumpEffect_ = std::make_unique<Emitter>();
  landEffect_ = std::make_unique<Emitter>();
  slideEffect_ = std::make_unique<Emitter>();
  speedTrailEffect_ = std::make_unique<Emitter>();
  itemGetEffect_ = std::make_unique<Emitter>();
  obstacleBreakEffect_ = std::make_unique<Emitter>();
  playerHitEffect_ = std::make_unique<Emitter>();
  barrierBreakEffect_ = std::make_unique<Emitter>();
  bossImpactEffect_ = std::make_unique<Emitter>();
  barrier_ = std::make_unique<HexBarrier>();
}

void GameSceneEffect::Initialize() {
  // 六角形バリアの初期化
  if (barrier_) {
    barrier_->Initialize("Resources/Texture/white64x64.png");
    barrier_->SetRadius(kDefaultHexRadius);
    barrier_->SetShape(HexBarrierShape::Honeycomb);
  }

  // 走行ダスト
  dustEffect_->Initialize();
  dustEffect_->LoadFromJson("Dustparticle");
  dustEffect_->SetName("Dust Effect");
  dustEffect_->generatorBehavior = nullptr;
  dustEffect_->SetStop(true);
  dustEffect_->SetSaveFileName("Dustparticle");

  // 雪パーティクルの初期化
  snowEffect_->Initialize();
  snowEffect_->LoadFromJson("Snowparticle");
  snowEffect_->SetName("Snow Effect");
  snowEffect_->generatorBehavior = nullptr;
  snowEffect_->SetStop(false);
  snowEffect_->SetLoop(true);
  snowEffect_->SetSaveFileName("Snowparticle");

  // ダイヤモンドダスト（きらめく微粒子）
  diamondDustEffect_->Initialize();
  diamondDustEffect_->LoadFromJson("DiamondDustparticle");
  diamondDustEffect_->SetName("Diamond Dust Effect");
  diamondDustEffect_->generatorBehavior = nullptr;
  diamondDustEffect_->SetStop(false);
  diamondDustEffect_->SetLoop(true);
  diamondDustEffect_->SetSaveFileName("DiamondDustparticle");

  // ジャンプ踏み込みリング
  jumpEffect_->Initialize();
  jumpEffect_->LoadFromJson("JumpRingparticle");
  jumpEffect_->SetName("Jump Ring Effect");
  jumpEffect_->generatorBehavior = nullptr;
  jumpEffect_->SetStop(true);
  jumpEffect_->SetLoop(false);
  jumpEffect_->SetSaveFileName("JumpRingparticle");

  // 着地スノーバースト
  landEffect_->Initialize();
  landEffect_->LoadFromJson("LandBlastparticle");
  landEffect_->SetName("Landing Snow Blast");
  landEffect_->generatorBehavior = nullptr;
  landEffect_->SetStop(true);
  landEffect_->SetLoop(false);
  landEffect_->SetSaveFileName("LandBlastparticle");

  // スライディング（お腹滑り）雪しぶき
  slideEffect_->Initialize();
  slideEffect_->LoadFromJson("SlideSprayparticle");
  slideEffect_->SetName("Sliding Snow Spray");
  slideEffect_->generatorBehavior = nullptr;
  slideEffect_->SetStop(true);
  slideEffect_->SetLoop(false);
  slideEffect_->SetSaveFileName("SlideSprayparticle");

  // スピードライン（疾走風粒子）
  speedTrailEffect_->Initialize();
  speedTrailEffect_->LoadFromJson("SpeedTrailparticle");
  speedTrailEffect_->SetName("Speed Trail Effect");
  speedTrailEffect_->generatorBehavior = nullptr;
  speedTrailEffect_->SetStop(true);
  speedTrailEffect_->SetLoop(true);
  speedTrailEffect_->SetSaveFileName("SpeedTrailparticle");

  // アイテム取得スパークル
  itemGetEffect_->Initialize();
  itemGetEffect_->LoadFromJson("ItemGetparticle");
  itemGetEffect_->SetName("Item Pickup Sparkle");
  itemGetEffect_->generatorBehavior = nullptr;
  itemGetEffect_->SetStop(true);
  itemGetEffect_->SetLoop(false);
  itemGetEffect_->SetSaveFileName("ItemGetparticle");

  // 障害物粉砕バースト
  obstacleBreakEffect_->Initialize();
  obstacleBreakEffect_->LoadFromJson("ObstacleBreakparticle");
  obstacleBreakEffect_->SetName("Obstacle Shatter Blast");
  obstacleBreakEffect_->generatorBehavior = nullptr;
  obstacleBreakEffect_->SetStop(true);
  obstacleBreakEffect_->SetLoop(false);
  obstacleBreakEffect_->SetSaveFileName("ObstacleBreakparticle");

  // プレイヤー被弾クラッシュ
  playerHitEffect_->Initialize();
  playerHitEffect_->LoadFromJson("PlayerHitparticle");
  playerHitEffect_->SetName("Player Hit Crash");
  playerHitEffect_->generatorBehavior = nullptr;
  playerHitEffect_->SetStop(true);
  playerHitEffect_->SetLoop(false);
  playerHitEffect_->SetSaveFileName("PlayerHitparticle");

  // バリア破壊氷結晶
  barrierBreakEffect_->Initialize();
  barrierBreakEffect_->LoadFromJson("BarrierBreakparticle");
  barrierBreakEffect_->SetName("Barrier Shatter Effect");
  barrierBreakEffect_->generatorBehavior = nullptr;
  barrierBreakEffect_->SetStop(true);
  barrierBreakEffect_->SetLoop(false);
  barrierBreakEffect_->SetSaveFileName("BarrierBreakparticle");

  // ボス攻撃着地・ヒットエフェクト
  bossImpactEffect_->Initialize();
  bossImpactEffect_->LoadFromJson("BossImpactparticle");
  bossImpactEffect_->SetName("Boss Impact Effect");
  bossImpactEffect_->generatorBehavior = nullptr;
  bossImpactEffect_->SetStop(true);
  bossImpactEffect_->SetLoop(false);
  bossImpactEffect_->SetSaveFileName("BossImpactparticle");
}

void GameSceneEffect::PlayingUpdate(const Matrix4x4 &view,
                                    const Vector3 &playerPos) {
  UpdateBarrier(view, playerPos);
  if (dustEffect_) {
    dustEffect_->Update(view);
  }
  if (snowEffect_) {
    Vector3 snowPos = {playerPos.x * kSnowPosScaleX, kSnowPosY, playerPos.z + kSnowPosZOffset};
    snowEffect_->SetPosition(snowPos);
    snowEffect_->Update(view);
  }
  if (diamondDustEffect_) {
    Vector3 diamondPos = {playerPos.x * 0.5f, playerPos.y + 4.0f, playerPos.z + 8.0f};
    diamondDustEffect_->SetPosition(diamondPos);
    diamondDustEffect_->Update(view);
  }
  if (jumpEffect_) {
    jumpEffect_->Update(view);
  }
  if (landEffect_) {
    landEffect_->Update(view);
  }
  if (slideEffect_) {
    slideEffect_->Update(view);
  }
  if (speedTrailEffect_) {
    if (speedTrailActive_) {
      Vector3 trailPos = {playerPos.x, playerPos.y + 1.0f, playerPos.z + 4.0f};
      speedTrailEffect_->SetPosition(trailPos);
      speedTrailEffect_->SetStop(false);
    } else {
      speedTrailEffect_->SetStop(true);
    }
    speedTrailEffect_->Update(view);
  }
  if (itemGetEffect_) {
    itemGetEffect_->Update(view);
  }
  if (obstacleBreakEffect_) {
    obstacleBreakEffect_->Update(view);
  }
  if (playerHitEffect_) {
    playerHitEffect_->Update(view);
  }
  if (barrierBreakEffect_) {
    barrierBreakEffect_->Update(view);
  }
  if (bossImpactEffect_) {
    bossImpactEffect_->Update(view);
  }
}

void GameSceneEffect::PlayerHitUpdate(const Matrix4x4 &view) {
  UpdateBarrier(view, lastPlayerPos_);
  if (dustEffect_) dustEffect_->Update(view);
  if (snowEffect_) snowEffect_->Update(view);
  if (diamondDustEffect_) diamondDustEffect_->Update(view);
  if (jumpEffect_) jumpEffect_->Update(view);
  if (landEffect_) landEffect_->Update(view);
  if (slideEffect_) slideEffect_->Update(view);
  if (speedTrailEffect_) {
    speedTrailEffect_->SetStop(true);
    speedTrailEffect_->Update(view);
  }
  if (itemGetEffect_) itemGetEffect_->Update(view);
  if (obstacleBreakEffect_) obstacleBreakEffect_->Update(view);
  if (playerHitEffect_) playerHitEffect_->Update(view);
  if (barrierBreakEffect_) barrierBreakEffect_->Update(view);
  if (bossImpactEffect_) bossImpactEffect_->Update(view);
}

void GameSceneEffect::EditorUpdate(const Matrix4x4 &view) {
  UpdateBarrier(view, lastPlayerPos_);
  if (dustEffect_) dustEffect_->SettingWvp(view);
  if (snowEffect_) snowEffect_->SettingWvp(view);
  if (diamondDustEffect_) diamondDustEffect_->SettingWvp(view);
  if (jumpEffect_) jumpEffect_->SettingWvp(view);
  if (landEffect_) landEffect_->SettingWvp(view);
  if (slideEffect_) slideEffect_->SettingWvp(view);
  if (speedTrailEffect_) speedTrailEffect_->SettingWvp(view);
  if (itemGetEffect_) itemGetEffect_->SettingWvp(view);
  if (obstacleBreakEffect_) obstacleBreakEffect_->SettingWvp(view);
  if (playerHitEffect_) playerHitEffect_->SettingWvp(view);
  if (barrierBreakEffect_) barrierBreakEffect_->SettingWvp(view);
  if (bossImpactEffect_) bossImpactEffect_->SettingWvp(view);
}

void GameSceneEffect::EmitDust(const Vector3 &playerPos) {
  EmitterData ed = dustEffect_->GetEmitterData();
  ed.transform.translate = playerPos;
  ed.transform.translate.y -= kDustYOffset; // ペンギンの足元（地面Y=2.0付近）に発生させる
  dustEffect_->SetEmitterData(ed);
  dustEffect_->Emit();
}

void GameSceneEffect::EmitJump(const Vector3 &playerPos) {
  if (!jumpEffect_) return;
  EmitterCircle ec = jumpEffect_->GetEmitterCircle();
  ec.translate = playerPos;
  ec.translate.y -= kDustYOffset;
  jumpEffect_->SetEmitterCircle(ec);
  jumpEffect_->TriggerBurst();
}

void GameSceneEffect::EmitLand(const Vector3 &playerPos) {
  if (!landEffect_) return;
  EmitterSphere es = landEffect_->GetEmitterSphere();
  es.translate = playerPos;
  es.translate.y -= 0.3f; // 地面に深くめり込ませず、足元の地面直上から吹き上がらせる
  landEffect_->SetEmitterSphere(es);
  landEffect_->TriggerBurst();
}

void GameSceneEffect::EmitSlide(const Vector3 &playerPos) {
  if (!slideEffect_) return;
  EmitterData ed = slideEffect_->GetEmitterData();
  ed.transform.translate = playerPos;
  ed.transform.translate.y -= (kDustYOffset + 0.1f);
  slideEffect_->SetEmitterData(ed);
  slideEffect_->Emit();
}

void GameSceneEffect::EmitItemGet(const Vector3 &pos, int itemType) {
  if (!itemGetEffect_) return;
  EmitterSphere es = itemGetEffect_->GetEmitterSphere();
  es.translate = pos;
  itemGetEffect_->SetEmitterSphere(es);

  // アイテム種別に応じたカラーグラデーション
  Vector4 startCol = {1.0f, 0.9f, 0.2f, 1.0f}; // 0: スコア/魚 (ゴールド)
  Vector4 endCol = {1.0f, 0.6f, 0.0f, 0.0f};
  if (itemType == 1) { // バリア (シアン/スカイブルー)
    startCol = {0.3f, 0.9f, 1.0f, 1.0f};
    endCol = {0.0f, 0.5f, 1.0f, 0.0f};
  } else if (itemType == 2) { // ボム/クリア (ファイアレッド/オレンジ)
    startCol = {1.0f, 0.4f, 0.1f, 1.0f};
    endCol = {1.0f, 0.1f, 0.0f, 0.0f};
  } else if (itemType == 3) { // ボス/ワンレーン (マゼンタ/パープル)
    startCol = {0.9f, 0.4f, 1.0f, 1.0f};
    endCol = {0.5f, 0.1f, 0.9f, 0.0f};
  }
  itemGetEffect_->SetColorOverLifetime(true, startCol, endCol);
  itemGetEffect_->TriggerBurst();
}

void GameSceneEffect::EmitObstacleBreak(const Vector3 &pos) {
  if (!obstacleBreakEffect_) return;
  EmitterSphere es = obstacleBreakEffect_->GetEmitterSphere();
  es.translate = pos;
  obstacleBreakEffect_->SetEmitterSphere(es);
  obstacleBreakEffect_->TriggerBurst();
}

void GameSceneEffect::EmitPlayerHit(const Vector3 &pos) {
  if (!playerHitEffect_) return;
  EmitterSphere es = playerHitEffect_->GetEmitterSphere();
  es.translate = pos;
  playerHitEffect_->SetEmitterSphere(es);
  playerHitEffect_->TriggerBurst();
}

void GameSceneEffect::EmitBarrierBreak(const Vector3 &pos) {
  if (!barrierBreakEffect_) return;
  EmitterSphere es = barrierBreakEffect_->GetEmitterSphere();
  es.translate = pos;
  barrierBreakEffect_->SetEmitterSphere(es);
  barrierBreakEffect_->TriggerBurst();
}

void GameSceneEffect::EmitBossImpact(const Vector3 &pos) {
  if (!bossImpactEffect_) return;
  EmitterCircle ec = bossImpactEffect_->GetEmitterCircle();
  ec.translate = pos;
  bossImpactEffect_->SetEmitterCircle(ec);
  bossImpactEffect_->TriggerBurst();
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
    float s = std::sin(t * kPiOver2);
    float overshoot = 1.0f + kDeployOvershootStrength * std::sin(t * kPi);
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
    float pulse = 1.0f + kPulseScaleAmplitude * std::sin(barrierPulseTimer_ * kPulseFrequency);
    float bright = kPulseBrightnessBase + kPulseBrightnessAmplitude * std::sin(barrierPulseTimer_ * kPulseFrequency);

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
    float expand = 1.0f + kBreakExpandStrength * t;
    currentScale.x *= expand;
    currentScale.y *= expand;
    currentScale.z *= expand;

    // 最初の一瞬はフラッシュ、その後減衰
    float fade = (1.0f - t > 0.0f) ? (1.0f - t) : 0.0f;
    if (t < kBreakFlashThreshold) {
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
  if (snowEffect_) {
    snowEffect_->Draw(draw);
  }
  if (diamondDustEffect_) {
    diamondDustEffect_->Draw(draw);
  }
  if (jumpEffect_) {
    jumpEffect_->Draw(draw);
  }
  if (landEffect_) {
    landEffect_->Draw(draw);
  }
  if (slideEffect_) {
    slideEffect_->Draw(draw);
  }
  if (speedTrailEffect_) {
    speedTrailEffect_->Draw(draw);
  }
  if (itemGetEffect_) {
    itemGetEffect_->Draw(draw);
  }
  if (obstacleBreakEffect_) {
    obstacleBreakEffect_->Draw(draw);
  }
  if (playerHitEffect_) {
    playerHitEffect_->Draw(draw);
  }
  if (barrierBreakEffect_) {
    barrierBreakEffect_->Draw(draw);
  }
  if (bossImpactEffect_) {
    bossImpactEffect_->Draw(draw);
  }

  // バリア描画
  if (barrier_ && barrierState_ != BarrierEffectState::Inactive) {
    draw.DrawObj(barrier_.get());
  }
}

void GameSceneEffect::ImGui() {
#ifdef _USE_IMGUI
  if (ImGui::CollapsingHeader("バリアエフェクト")) {
    const char *stateNames[] = {"非アクティブ", "展開中", "アクティブ", "破壊中"};
    ImGui::Text("現在の状態: %s",
                stateNames[static_cast<int>(barrierState_)]);

    if (ImGui::Button("バリア展開")) {
      EmitBarrier(lastPlayerPos_);
    }
    ImGui::SameLine();
    if (ImGui::Button("バリア破壊")) {
      BreakBarrier(lastPlayerPos_);
    }
    ImGui::SameLine();
    if (ImGui::Button("バリア消去")) {
      ClearBarrier();
    }

    ImGui::Separator();
    ImGui::DragFloat3("バリア位置オフセット", &barrierOffset_.x, 0.05f);
    ImGui::DragFloat3("バリアスケール", &barrierBaseScale_.x, 0.05f, 0.1f,
                      10.0f);
    ImGui::ColorEdit4("バリアカラー", &barrierBaseColor_.x);
    ImGui::ColorEdit4("フラッシュカラー", &barrierFlashColor_.x);

    if (barrier_) {
      float r = barrier_->GetRadius();
      if (ImGui::DragFloat("ヘキサゴン半径", &r, 0.02f, 0.1f, 5.0f)) {
        barrier_->SetRadius(r);
      }

      bool pointy = barrier_->GetPointyTopped();
      if (ImGui::Checkbox("頂点上向き (Pointy Topped)", &pointy)) {
        barrier_->SetPointyTopped(pointy);
      }

      int shapeType = static_cast<int>(barrier_->GetShape());
      const char *shapes[] = {"単一六角形 (Single)", "ハニカム (7セル)"};
      if (ImGui::Combo("形状", &shapeType, shapes, IM_ARRAYSIZE(shapes))) {
        barrier_->SetShape(static_cast<HexBarrierShape>(shapeType));
      }

      if (auto mat = barrier_->GetComponent<MaterialComponent>()) {
        int blendMode = static_cast<int>(mat->GetBlend());
        const char *blendNames[] = {"なし (None)",     "通常 (Normal)",   "加算 (Add)",
                                    "減算 (Subtract)", "乗算 (Multiply)", "スクリーン (Screen)"};
        if (ImGui::Combo("ブレンドモード", &blendMode, blendNames,
                         IM_ARRAYSIZE(blendNames))) {
          mat->SetBlend(static_cast<BlendMode>(blendMode));
        }
      }
    }
  }

  if (ImGui::CollapsingHeader("パーティクル一覧")) {
    ImGui::Checkbox("スピードライン有効", &speedTrailActive_);

    if (ImGui::Button("テスト: ジャンプエフェクト")) {
      EmitJump(lastPlayerPos_);
    }
    ImGui::SameLine();
    if (ImGui::Button("テスト: 着地エフェクト")) {
      EmitLand(lastPlayerPos_);
    }
    ImGui::SameLine();
    if (ImGui::Button("テスト: アイテム取得")) {
      EmitItemGet(lastPlayerPos_, 0);
    }
    if (ImGui::Button("テスト: 障害物粉砕")) {
      EmitObstacleBreak(lastPlayerPos_);
    }
    ImGui::SameLine();
    if (ImGui::Button("テスト: プレイヤー被弾")) {
      EmitPlayerHit(lastPlayerPos_);
    }
    ImGui::SameLine();
    if (ImGui::Button("テスト: ボス衝撃")) {
      EmitBossImpact(lastPlayerPos_);
    }

    if (dustEffect_) dustEffect_->ImGui();
    if (snowEffect_) snowEffect_->ImGui();
    if (diamondDustEffect_) diamondDustEffect_->ImGui();
    if (jumpEffect_) jumpEffect_->ImGui();
    if (landEffect_) landEffect_->ImGui();
    if (slideEffect_) slideEffect_->ImGui();
    if (speedTrailEffect_) speedTrailEffect_->ImGui();
    if (itemGetEffect_) itemGetEffect_->ImGui();
    if (obstacleBreakEffect_) obstacleBreakEffect_->ImGui();
    if (playerHitEffect_) playerHitEffect_->ImGui();
    if (barrierBreakEffect_) barrierBreakEffect_->ImGui();
    if (bossImpactEffect_) bossImpactEffect_->ImGui();
  }
#endif
}


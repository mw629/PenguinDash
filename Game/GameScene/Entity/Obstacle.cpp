#include "Obstacle.h"
#include "../Camera/GameCameraController.h"
#include "../Effect/GameSceneEffect.h"
#include "../GameScene.h"
#include "../Stage/StageSettings.h"
#include "../System/ScoreManager.h"
#include "../System/SoundManager.h"
#include "Graphics/Render/Draw.h"
#include "Math/Calculation.h"
#include "Player.h"
#include "Resource/AssetManager.h"
#include <cmath>

// =========================================================================
// IObstacleBehavior 基底実装および具象ビヘイビアクラス群
// =========================================================================

void IObstacleBehavior::OnSpawn(Obstacle &obs, float x, float y,
                                float z) const {
  obs.targetY_ = y;
  obs.isFalling_ = false;
  obs.justLanded_ = false;
  obs.transform_.translate = {x, y, z};
}

void IObstacleBehavior::UpdateScroll(Obstacle &obs, float scrollSpeed) const {
  // 通常は手前にスクロール
  obs.transform_.translate.z -= scrollSpeed;
  if (obs.transform_.translate.z < Obstacle::kPlayerObstacleDespawnZ) {
    obs.isActive_ = false;
  }
}

void IObstacleBehavior::ApplyDrawTransform(Obstacle &obs,
                                           Transform &drawTransform) const {
  if (UsesBottomOriginOffset()) {
    drawTransform.translate.y -=
        obs.collisionHeight_ * Obstacle::kScaleOffsetFactor;
  }
}

void IObstacleBehavior::OnHit(Obstacle & /*obs*/) const {}

bool IObstacleBehavior::OnPlayerCollision(Obstacle &obs, GameScene *scene,
                                          Player *player) const {
  if (player->GetHasBarrier()) {
    player->SetHasBarrier(false);
    scene->AddBarrierBlockCount();
    obs.OnBlowAway();
    if (scene->GetCameraController()) {
      scene->GetCameraController()->ShakeDefeat();
    }
    SoundManager::GetInstance()->PlaySE(SoundManager::SE::BarrierBreak);
    if (scene->GetEffectManager()) {
      scene->GetEffectManager()->EmitObstacleBreak(obs.transform_.translate);
      scene->GetEffectManager()->EmitBarrierBreak(
          player->GetTransform().translate);
      scene->GetEffectManager()->BreakBarrier(player->GetTransform().translate);
    }
    return false;
  }

  // 通常被弾
  scene->TriggerPlayerHit(IsTripOnHit(), IsLargeObstacle());
  return true;
}

/// <summary>
/// 通常障害物（Low / High / Wall）の共通基底ビヘイビア
/// </summary>
class RegularObstacleBehavior : public IObstacleBehavior {
public:
  bool IsRegularObstacle() const override { return true; }
  bool UsesBottomOriginOffset() const override { return true; }
};

class LowObstacleBehavior final : public RegularObstacleBehavior {
public:
  const char *GetDebugTypeName() const override { return "Low (木/ジャンプ)"; }
  Vector4 GetDebugTypeColor() const override { return {0.2f, 1.0f, 0.3f, 1.0f}; }
  bool IsTripOnHit() const override { return true; }
  void ApplyAppearance(Obstacle &obs) const override {
    obs.ApplyProfile(obs.lowModel_.get(), Obstacle::kProfileLow, true);
  }
};

class HighObstacleBehavior final : public RegularObstacleBehavior {
public:
  const char *GetDebugTypeName() const override { return "High (アーチ/潜り)"; }
  Vector4 GetDebugTypeColor() const override { return {1.0f, 0.9f, 0.2f, 1.0f}; }
  bool CanRollUnder() const override { return true; }
  bool IsLargeObstacle() const override { return true; }
  void ApplyAppearance(Obstacle &obs) const override {
    obs.ApplyProfile(obs.highModel_.get(), Obstacle::kProfileHigh, true);
  }
};

class WallObstacleBehavior final : public RegularObstacleBehavior {
public:
  const char *GetDebugTypeName() const override { return "Wall (氷壁/レーン)"; }
  Vector4 GetDebugTypeColor() const override { return {1.0f, 0.4f, 0.1f, 1.0f}; }
  bool RequiresLaneChangeToAvoid() const override { return true; }
  bool IsLargeObstacle() const override { return true; }
  void ApplyAppearance(Obstacle &obs) const override {
    obs.ApplyProfile(obs.wallModel_.get(), Obstacle::kProfileWall, true,
                     {1.0f, 1.0f, 1.0f, 0.6f});
  }
};

class BonusObstacleBehavior final : public IObstacleBehavior {
public:
  const char *GetDebugTypeName() const override { return "Bonus (ボーナス)"; }
  Vector4 GetDebugTypeColor() const override { return {0.9f, 0.4f, 1.0f, 1.0f}; }
  bool UsesBottomOriginOffset() const override { return true; }
  void ApplyAppearance(Obstacle &obs) const override {
    obs.ApplyProfile(obs.bonusModel_.get(), Obstacle::kProfileBonus, true);
  }
  bool OnPlayerCollision(Obstacle &obs, GameScene *scene,
                         Player * /*player*/) const override {
    obs.OnBlowAway();
    if (scene->GetScoreManager()) {
      scene->GetScoreManager()->AddBonusEnemyHit();
    }
    if (scene->GetCameraController()) {
      scene->GetCameraController()->ShakeDefeat();
    }
    if (scene->GetEffectManager()) {
      scene->GetEffectManager()->EmitItemGet(obs.transform_.translate, 0);
      scene->GetEffectManager()->EmitObstacleBreak(obs.transform_.translate);
    }
    SoundManager::GetInstance()->PlaySE(SoundManager::SE::BonusHit);
    return false;
  }
};

class GuideFloorBehavior final : public IObstacleBehavior {
public:
  const char *GetDebugTypeName() const override { return "Guide (誘導床)"; }
  Vector4 GetDebugTypeColor() const override { return {0.0f, 0.8f, 1.0f, 1.0f}; }
  void ApplyAppearance(Obstacle &obs) const override {
    obs.ApplyProfile(obs.itemModel_.get(), Obstacle::kProfileGuideFloor, false);
    if (obs.currentModel_) {
      obs.currentModel_->GetMaterial()->SetColor(Obstacle::kGuideFloorColor);
      obs.currentModel_->SetShader("ObjectShader");
    }
  }
  bool OnPlayerCollision(Obstacle & /*obs*/, GameScene * /*scene*/,
                         Player *player) const override {
    player->StartForceToCenter(Obstacle::kBarrierForceToCenterFrames);
    SoundManager::GetInstance()->PlaySE(SoundManager::SE::Slide);
    return false;
  }
};

/// <summary>
/// ビルボードアイテム共通基底ビヘイビア
/// </summary>
class ItemObstacleBehavior : public IObstacleBehavior {
public:
  bool IsItem() const override { return true; }

  void OnSpawn(Obstacle &obs, float x, float y, float z) const override {
    IObstacleBehavior::OnSpawn(obs, x, y, z);
    obs.transform_.translate.y += Obstacle::kItemSpawnHeightOffset;
  }

  void ApplyDrawTransform(Obstacle &obs,
                          Transform &drawTransform) const override {
    // ビルボードアイテムの浮遊ボビング演出（上下に優しく揺れる）
    obs.itemFloatTimer_ += Obstacle::kItemBobbingSpeed;
    drawTransform.translate.y +=
        std::sin(obs.itemFloatTimer_ +
                 obs.transform_.translate.x *
                     Obstacle::kItemBobbingPhaseMultiplier) *
        Obstacle::kItemBobbingAmplitude;
  }

  void OnHit(Obstacle &obs) const override {
    // アイテム取得時は即座に非アクティブ（消去）にする
    obs.isActive_ = false;
  }
};

class CameraItemBehavior final : public ItemObstacleBehavior {
public:
  const char *GetDebugTypeName() const override { return "Item: 1Lane"; }
  Vector4 GetDebugTypeColor() const override { return {0.3f, 1.0f, 0.6f, 1.0f}; }
  void ApplyAppearance(Obstacle &obs) const override {
    obs.ApplyProfile(obs.oneLaneItemModel_.get(),
                     Obstacle::kProfileItemBillboardSmall);
  }
  bool OnPlayerCollision(Obstacle &obs, GameScene *scene,
                         Player * /*player*/) const override {
    obs.OnHit();
    scene->AddItemGetCount();
    if (scene->GetEffectManager()) {
      scene->GetEffectManager()->EmitItemGet(obs.transform_.translate, 3);
    }
    // 遠方の障害物を吹き飛ばし、カメラ遷移までの待機時間を解消する
    if (scene->GetStageSettings()) {
      for (int j = 0; j < scene->GetStageSettings()->GetMaxObstacles(); j++) {
        Obstacle *target = scene->GetStageSettings()->GetObstacle(j);
        if (target && target->GetIsActive() && target->IsRegularObstacle()) {
          if (target->GetTransform().translate.z > 15.0f) {
            target->OnBlowAway();
            if (scene->GetEffectManager()) {
              scene->GetEffectManager()->EmitObstacleBreak(
                  target->GetTransform().translate);
            }
          }
        }
      }
    }
    scene->ChangePlayingState(GameScene::PlayingState::OneLane);
    SoundManager::GetInstance()->PlaySE(SoundManager::SE::ItemGet);
    return false;
  }
};

class BarrierItemBehavior final : public ItemObstacleBehavior {
public:
  const char *GetDebugTypeName() const override { return "Item: Barrier"; }
  Vector4 GetDebugTypeColor() const override { return {0.2f, 0.8f, 1.0f, 1.0f}; }
  void ApplyAppearance(Obstacle &obs) const override {
    obs.ApplyProfile(obs.barrierItemModel_.get(),
                     Obstacle::kProfileItemBillboardSmall);
  }
  bool OnPlayerCollision(Obstacle &obs, GameScene *scene,
                         Player *player) const override {
    obs.OnHit();
    scene->AddItemGetCount();
    player->SetHasBarrier(true);
    SoundManager::GetInstance()->PlaySE(SoundManager::SE::Barrier);
    if (scene->GetEffectManager()) {
      scene->GetEffectManager()->EmitItemGet(obs.transform_.translate, 1);
      scene->GetEffectManager()->EmitBarrier(player->GetTransform().translate);
    }
    return false;
  }
};

class ClearItemBehavior final : public ItemObstacleBehavior {
public:
  const char *GetDebugTypeName() const override { return "Item: Bomb"; }
  Vector4 GetDebugTypeColor() const override { return {1.0f, 0.2f, 0.2f, 1.0f}; }
  void ApplyAppearance(Obstacle &obs) const override {
    obs.ApplyProfile(obs.clearItemModel_.get(),
                     Obstacle::kProfileItemBillboardSmall);
  }
  bool OnPlayerCollision(Obstacle &obs, GameScene *scene,
                         Player * /*player*/) const override {
    obs.OnHit();
    scene->AddItemGetCount();
    scene->AddClearBombCount();
    SoundManager::GetInstance()->PlaySE(SoundManager::SE::ClearBomb);
    if (scene->GetCameraController()) {
      scene->GetCameraController()->ShakeImpact();
    }
    if (scene->GetEffectManager()) {
      scene->GetEffectManager()->EmitItemGet(obs.transform_.translate, 2);
      scene->GetEffectManager()->EmitObstacleBreak(obs.transform_.translate);
    }
    if (scene->GetStageSettings()) {
      for (int j = 0; j < scene->GetStageSettings()->GetMaxObstacles(); j++) {
        Obstacle *target = scene->GetStageSettings()->GetObstacle(j);
        if (target && target->GetIsActive() && target->IsRegularObstacle()) {
          target->OnBlowAway();
          if (scene->GetEffectManager()) {
            scene->GetEffectManager()->EmitObstacleBreak(
                target->GetTransform().translate);
          }
        }
      }
    }
    return false;
  }
};

class BossItemBehavior final : public ItemObstacleBehavior {
public:
  const char *GetDebugTypeName() const override { return "Item: Boss"; }
  Vector4 GetDebugTypeColor() const override { return {1.0f, 0.8f, 0.0f, 1.0f}; }
  void ApplyAppearance(Obstacle &obs) const override {
    obs.ApplyProfile(obs.bossItemModel_.get(), Obstacle::kProfileBossItem);
  }
  bool OnPlayerCollision(Obstacle &obs, GameScene *scene,
                         Player * /*player*/) const override {
    obs.OnHit();
    scene->AddItemGetCount();
    SoundManager::GetInstance()->PlaySE(SoundManager::SE::Barrier);
    if (scene->GetEffectManager()) {
      scene->GetEffectManager()->EmitItemGet(obs.transform_.translate, 3);
    }
    if (scene->GetStageSettings()) {
      for (int j = 0; j < scene->GetStageSettings()->GetMaxObstacles(); j++) {
        Obstacle *target = scene->GetStageSettings()->GetObstacle(j);
        if (target && target->GetIsActive() && target->IsRegularObstacle()) {
          target->OnBlowAway();
          if (scene->GetEffectManager()) {
            scene->GetEffectManager()->EmitObstacleBreak(
                target->GetTransform().translate);
          }
        }
      }
    }
    scene->ChangePlayingState(GameScene::PlayingState::Boss);
    return false;
  }
};

/// <summary>
/// ボス攻撃共通基底ビヘイビア（上空落下・+Z進行・反射/落下中判定スキップ）
/// </summary>
class BossAttackBehavior : public IObstacleBehavior {
public:
  const char *GetDebugTypeName() const override { return "Boss Attack"; }
  Vector4 GetDebugTypeColor() const override { return {1.0f, 0.3f, 0.6f, 1.0f}; }
  bool IsBossAttackType() const override { return true; }
  bool UsesBottomOriginOffset() const override { return true; }

  void OnSpawn(Obstacle &obs, float x, float y, float z) const override {
    obs.targetY_ = y;
    obs.isFalling_ = true;
    obs.fallTimer_ = 0.0f;
    obs.justLanded_ = false;
    obs.transform_.translate = {x, obs.targetY_ + obs.dropHeight_, z};
  }

  void UpdateScroll(Obstacle &obs, float scrollSpeed) const override {
    // 上空からの落下演出
    if (obs.isFalling_) {
      obs.fallTimer_ += 1.0f;
      float t = obs.fallTimer_ / obs.fallDuration_;
      if (t >= 1.0f) {
        t = 1.0f;
        obs.isFalling_ = false;
        obs.justLanded_ = true;
        obs.transform_.translate.y = obs.targetY_;
      } else {
        float easeT = t * t;
        float startY = obs.targetY_ + obs.dropHeight_;
        obs.transform_.translate.y = startY + (obs.targetY_ - startY) * easeT;
      }
    }

    // ボスの攻撃は奥から手前(+Z方向)へ
    obs.transform_.translate.z += scrollSpeed;
    if (obs.transform_.translate.z > Obstacle::kBossAttackDespawnZ) {
      obs.isActive_ = false;
      obs.isFalling_ = false;
    }
  }

  bool OnPlayerCollision(Obstacle &obs, GameScene *scene,
                         Player *player) const override {
    if (obs.isReflected_ || obs.isFalling_) {
      return false;
    }
    return IObstacleBehavior::OnPlayerCollision(obs, scene, player);
  }
};

class BossAttackNormalBehavior final : public BossAttackBehavior {
public:
  bool RequiresLaneChangeToAvoid() const override { return true; }
  bool IsLargeObstacle() const override { return true; }
  void ApplyAppearance(Obstacle &obs) const override {
    obs.ApplyProfile(obs.wallModel_.get(), Obstacle::kProfileWall, true);
  }
};

class BossAttackReflectableBehavior final : public BossAttackBehavior {
public:
  bool RequiresLaneChangeToAvoid() const override { return true; }
  void ApplyAppearance(Obstacle &obs) const override {
    obs.ApplyProfile(obs.reflectingAttackModel_.get(),
                     Obstacle::kProfileBossAttackReflectable, true);
  }
};

class BossAttackLowBehavior final : public BossAttackBehavior {
public:
  bool IsTripOnHit() const override { return true; }
  void ApplyAppearance(Obstacle &obs) const override {
    obs.ApplyProfile(obs.lowModel_.get(), Obstacle::kProfileLow, true);
  }
};

class BossAttackHighBehavior final : public BossAttackBehavior {
public:
  bool CanRollUnder() const override { return true; }
  bool IsLargeObstacle() const override { return true; }
  void ApplyAppearance(Obstacle &obs) const override {
    obs.ApplyProfile(obs.highModel_.get(), Obstacle::kProfileHigh, true);
  }
};

const IObstacleBehavior *Obstacle::ResolveBehavior(Type type) {
  static const LowObstacleBehavior kLow;
  static const HighObstacleBehavior kHigh;
  static const WallObstacleBehavior kWall;
  static const BonusObstacleBehavior kBonus;
  static const GuideFloorBehavior kGuideFloor;
  static const CameraItemBehavior kCameraItem;
  static const BarrierItemBehavior kBarrierItem;
  static const ClearItemBehavior kClearItem;
  static const BossItemBehavior kBossItem;
  static const BossAttackNormalBehavior kBossAttack;
  static const BossAttackReflectableBehavior kBossAttackReflectable;
  static const BossAttackLowBehavior kBossAttackLow;
  static const BossAttackHighBehavior kBossAttackHigh;

  switch (type) {
  case Type::Low:
    return &kLow;
  case Type::High:
    return &kHigh;
  case Type::Wall:
    return &kWall;
  case Type::Bonus:
    return &kBonus;
  case Type::GuideFloor:
    return &kGuideFloor;
  case Type::CameraItem:
    return &kCameraItem;
  case Type::BarrierItem:
    return &kBarrierItem;
  case Type::ClearItem:
    return &kClearItem;
  case Type::BossItem:
    return &kBossItem;
  case Type::BossAttack:
    return &kBossAttack;
  case Type::BossAttackReflectable:
    return &kBossAttackReflectable;
  case Type::BossAttackLow:
    return &kBossAttackLow;
  case Type::BossAttackHigh:
    return &kBossAttackHigh;
  }
  return &kWall;
}

Obstacle::Obstacle() {
  name_ = "Obstacle";
  isActive_ = false;
  behavior_ = ResolveBehavior(type_);
}

Obstacle::~Obstacle() {}

void Obstacle::Initialize(ModelData lowData, ModelData highData,
                          ModelData wallData, ModelData bonusData,
                          ModelData iceBomData, ModelData reflectingAttackData,
                          Type type) {
  lowModelData_ = lowData;
  highModelData_ = highData;
  wallModelData_ = wallData;
  bonusModelData_ = bonusData;
  iceBomModelData_ = iceBomData;
  reflectingAttackModelData_ = reflectingAttackData;

  lowModel_->Initialize(lowData);
  lowModel_->SetShader("ObjectShader");
  lowModel_->SetLighting(lightingEnabled_);
  lowModel_->GetMaterial()->SetColor(kNormalColor);

  highModel_->Initialize(highData);
  highModel_->SetShader("ObjectShader");
  highModel_->SetLighting(lightingEnabled_);
  highModel_->GetMaterial()->SetColor(kNormalColor);

  wallModel_->Initialize(wallData);
  wallModel_->SetShader("ObjectShader");
  wallModel_->SetLighting(lightingEnabled_);
  wallModel_->GetMaterial()->SetColor(kNormalColor);

  bonusModel_->Initialize(bonusData);
  bonusModel_->SetShader("ObjectShader");
  bonusModel_->SetLighting(lightingEnabled_);
  bonusModel_->GetMaterial()->SetColor(kNormalColor);

  itemModel_->Initialize(wallData);
  itemModel_->SetShader("ObjectShader");
  itemModel_->SetLighting(lightingEnabled_);

  iceBomModel_->Initialize(iceBomData);
  iceBomModel_->SetShader("ObjectShader");
  iceBomModel_->SetLighting(lightingEnabled_);
  iceBomModel_->GetMaterial()->SetColor(kNormalColor);

  reflectingAttackModel_->Initialize(reflectingAttackData);
  reflectingAttackModel_->SetShader("ObjectShader");
  reflectingAttackModel_->SetLighting(lightingEnabled_);
  reflectingAttackModel_->GetMaterial()->SetColor(kNormalColor);

  // ビルボードアイテム用平面モデルの初期化
  ModelData planeData =
      AssetManager::LoadModel("Resources/Model/obj", "plane.obj");

  // 1. バリアを張るアイテム (shieldItem)
  ModelData barrierData = planeData;
  barrierData.material.textureFilePath = "Resources/Texture/Item/shieldItem.png";
  barrierData.textureIndex =
      AssetManager::LoadTexture("Resources/Texture/Item/shieldItem.png");
  barrierItemModel_->Initialize(barrierData);
  barrierItemModel_->SetShader("ObjectShader");
  barrierItemModel_->SetCullMode(kCullModeNone);
  barrierItemModel_->SetLighting(false);
  barrierItemModel_->SetBillboard(true);
  barrierItemModel_->GetMaterial()->SetColor(kNormalColor);

  // 2. ボス戦アイテム (BossItems)
  ModelData bossData = planeData;
  bossData.material.textureFilePath = "Resources/Texture/Item/BossItems.png";
  bossData.textureIndex =
      AssetManager::LoadTexture("Resources/Texture/Item/BossItems.png");
  bossItemModel_->Initialize(bossData);
  bossItemModel_->SetShader("ObjectShader");
  bossItemModel_->SetCullMode(kCullModeNone);
  bossItemModel_->SetLighting(false);
  bossItemModel_->SetBillboard(true);
  bossItemModel_->GetMaterial()->SetColor(kNormalColor);

  // 3. レーン消去アイテム (BomItem)
  ModelData clearData = planeData;
  clearData.material.textureFilePath = "Resources/Texture/Item/BomItem.png";
  clearData.textureIndex =
      AssetManager::LoadTexture("Resources/Texture/Item/BomItem.png");
  clearItemModel_->Initialize(clearData);
  clearItemModel_->SetShader("ObjectShader");
  clearItemModel_->SetCullMode(kCullModeNone);
  clearItemModel_->SetLighting(false);
  clearItemModel_->SetBillboard(true);
  clearItemModel_->GetMaterial()->SetColor(kNormalColor);

  // 4. 1レーン化アイテム (Onelane)
  ModelData oneLaneData = planeData;
  oneLaneData.material.textureFilePath = "Resources/Texture/Item/Onelane.png";
  oneLaneData.textureIndex =
      AssetManager::LoadTexture("Resources/Texture/Item/Onelane.png");
  oneLaneItemModel_->Initialize(oneLaneData);
  oneLaneItemModel_->SetShader("ObjectShader");
  oneLaneItemModel_->SetCullMode(kCullModeNone);
  oneLaneItemModel_->SetLighting(false);
  oneLaneItemModel_->SetBillboard(true);
  oneLaneItemModel_->GetMaterial()->SetColor(kNormalColor);

  SetType(type);
  SetEnvironmentReflection(environmentReflectionEnabled_);
}

void Obstacle::Initialize(ModelData lowData, ModelData highData,
                          ModelData wallData, ModelData bonusData,
                          Type type) {
  Initialize(lowData, highData, wallData, bonusData, wallData, wallData, type);
}

void Obstacle::Initialize(ModelData normalData, ModelData bonusData,
                          Type type) {
  Initialize(normalData, normalData, normalData, bonusData, type);
}

void Obstacle::SetLighting(bool enabled) {
  lightingEnabled_ = enabled;
  if (lowModel_) lowModel_->SetLighting(enabled);
  if (highModel_) highModel_->SetLighting(enabled);
  if (wallModel_) wallModel_->SetLighting(enabled);
  if (bonusModel_) bonusModel_->SetLighting(enabled);
  if (itemModel_) itemModel_->SetLighting(enabled);
  if (iceBomModel_) iceBomModel_->SetLighting(enabled);
  if (reflectingAttackModel_) reflectingAttackModel_->SetLighting(enabled);
}

void Obstacle::SetEnvironmentReflection(bool enabled) {
  environmentReflectionEnabled_ = enabled;
  float coeff = enabled ? environmentCoefficient_ : 0.0f;
  if (lowModel_) lowModel_->SetEnvironmentCoefficient(coeff);
  if (highModel_) highModel_->SetEnvironmentCoefficient(coeff);
  if (wallModel_) wallModel_->SetEnvironmentCoefficient(coeff);
  if (bonusModel_) bonusModel_->SetEnvironmentCoefficient(coeff);
  if (iceBomModel_) iceBomModel_->SetEnvironmentCoefficient(coeff);
  if (reflectingAttackModel_) reflectingAttackModel_->SetEnvironmentCoefficient(coeff);
}

void Obstacle::SetEnvironmentCoefficient(float coeff) {
  environmentCoefficient_ = coeff;
  if (environmentReflectionEnabled_) {
    if (lowModel_) lowModel_->SetEnvironmentCoefficient(coeff);
    if (highModel_) highModel_->SetEnvironmentCoefficient(coeff);
    if (wallModel_) wallModel_->SetEnvironmentCoefficient(coeff);
    if (bonusModel_) bonusModel_->SetEnvironmentCoefficient(coeff);
    if (iceBomModel_) iceBomModel_->SetEnvironmentCoefficient(coeff);
    if (reflectingAttackModel_) reflectingAttackModel_->SetEnvironmentCoefficient(coeff);
  }
}

void Obstacle::ApplyProfile(Model *model, const ObstacleProfile &profile,
                            bool resetStandardMaterial, const Vector4 &color) {
  currentModel_ = model;
  if (currentModel_ && resetStandardMaterial) {
    currentModel_->GetMaterial()->SetColor(color);
    currentModel_->SetShader("ObjectShader");
    currentModel_->SetBlend(BlendMode::kBlendModeNormal);
  }
  collisionWidth_ = profile.collisionSize.x;
  collisionHeight_ = profile.collisionSize.y;
  collisionDepth_ = profile.collisionSize.z;
  transform_.scale = profile.scale;
  transform_.rotate = profile.rotate;
}

void Obstacle::SetType(Type type) {
  type_ = type;
  behavior_ = ResolveBehavior(type_);

  if (behavior_) {
    behavior_->ApplyAppearance(*this);
  }

  if (currentModel_) {
    Transform drawTransform = transform_;
    if (behavior_ && behavior_->UsesBottomOriginOffset()) {
      drawTransform.translate.y -= collisionHeight_ * kScaleOffsetFactor;
    }
    currentModel_->SetTransform(drawTransform);
  }
}

void Obstacle::Spawn(float x, float y, float z) {
  if (behavior_) {
    behavior_->OnSpawn(*this, x, y, z);
  }
  isActive_ = true;
  isHit_ = false; // 初期化
  isReflected_ = false;
  reflectTimer_ = 0.0f;
  if (currentModel_) {
    Transform drawTransform = transform_;
    if (behavior_ && behavior_->UsesBottomOriginOffset()) {
      drawTransform.translate.y -= collisionHeight_ * kScaleOffsetFactor;
    }
    currentModel_->SetTransform(drawTransform);
  }
}

void Obstacle::StageUpdate(Matrix4x4 view, float scrollSpeed) {
  if (!isActive_)
    return;

  // 前フレームの着地フラグをクリア
  justLanded_ = false;

  if (isHit_) {
    isFalling_ = false;
    // 吹き飛び演出
    transform_.translate.x += velocity_.x;
    transform_.translate.y += velocity_.y;
    transform_.translate.z += velocity_.z;
    velocity_.y -= gravity_; // 重力

    transform_.rotate.x += kBlowAwayAngularVelocity.x; // 回転させる
    transform_.rotate.y += kBlowAwayAngularVelocity.y;
    transform_.rotate.z += kBlowAwayAngularVelocity.z;

    // カメラにぶつかる演出（カメラのZ座標付近に来たら）
    if (transform_.translate.z < kBlowAwayCameraZThreshold && velocity_.z < 0.0f) {
      velocity_.z = 0.0f;  // 手前に来るのを止める（画面に張り付いたような演出）
      velocity_.y = kBlowAwayStickDropVelocityY; // 下に落ち始める
      velocity_.x = ((float)rand() / RAND_MAX - 0.5f) * kBlowAwayStickRandScaleX; // 横に少しずれる
    }

    if (transform_.translate.y < kBlowAwayDespawnYThreshold) {
      isActive_ = false; // 画面外で消す
    }
  } else if (isReflected_) {
    isFalling_ = false;

    // スクロール速度（またはtimeScale）に応じてタイマーを進める
    // 通常スクロール速度(kBaseScrollSpeed)基準で1フレームあたり1.0加算（停止時は加算しない）
    float speedFactor = (scrollSpeed > 0.0f) ? (scrollSpeed / kBaseScrollSpeed) : 0.0f;
    reflectTimer_ += speedFactor;

    float t =
        (reflectDuration_ > 0.0f) ? (reflectTimer_ / reflectDuration_) : 1.0f;
    if (t > 1.0f) {
      t = 1.0f;
    }

    // 水平方向（X, Z）は始点から目標への線形補間（放物線の等速運動）
    transform_.translate.x =
        reflectStartPos_.x + (reflectedTarget_.x - reflectStartPos_.x) * t;
    transform_.translate.z =
        reflectStartPos_.z + (reflectedTarget_.z - reflectStartPos_.z) * t;

    // 垂直方向（Y）：始点から目標への線形補間にサイン波オフセットを加えて上向きの弧（山なり）を描く
    float baseY =
        reflectStartPos_.y + (reflectedTarget_.y - reflectStartPos_.y) * t;
    float arcOffset = std::sin(t * kPi) * reflectArcHeight_;
    transform_.translate.y = baseY + arcOffset;

    // 勢いよく回転させながら飛ぶ
    transform_.rotate.x += kReflectAngularVelocity.x;
    transform_.rotate.y += kReflectAngularVelocity.y;
    transform_.rotate.z += kReflectAngularVelocity.z;
  } else {
    // 種別ごとのスクロール・落下処理へ委譲
    if (behavior_) {
      behavior_->UpdateScroll(*this, scrollSpeed);
    }
  }

  if (currentModel_) {
    Transform drawTransform = transform_;
    if (behavior_) {
      behavior_->ApplyDrawTransform(*this, drawTransform);
    }

    currentModel_->SetTransform(drawTransform);
    currentModel_->SettingWvp(view);
  }

  // コンポーネント（ColliderComponentなど）のUpdateを呼ぶ
  GameObject::Update(view, 1.0f);
}

void Obstacle::OnBlowAway() {
  if (isHit_)
    return;
  // 上と手前(画面方向)に勢いよく飛ぶ
  float randX = ((float)rand() / RAND_MAX - 0.5f) * kBlowAwayRandRangeX;
  OnBlowAway({randX, kBlowAwayInitialVelocityY, kBlowAwayInitialVelocityZ});
}

void Obstacle::OnBlowAway(const Vector3 &velocity) {
  if (isHit_)
    return;
  isHit_ = true;
  isFalling_ = false;
  velocity_ = velocity;
}

void Obstacle::OnHit() {
  if (isHit_)
    return;
  isHit_ = true;
  isFalling_ = false;

  if (behavior_) {
    behavior_->OnHit(*this);
  }
}

void Obstacle::Draw(class Draw &draw) {
  if (!isActive_ || !currentModel_)
    return;
  draw.DrawObj(currentModel_);
  GameObject::Draw(draw);
}

void Obstacle::ImGuiInnerComponents() {
  if (currentModel_) {
    currentModel_->ImGui(false);
  }
#ifdef _USE_IMGUI
  if (isReflected_) {
    ImGui::Text("Reflect Progress: %.2f", GetReflectProgress());
    ImGui::DragFloat("Arc Height", &reflectArcHeight_, 0.1f);
    ImGui::DragFloat("Duration", &reflectDuration_, 1.0f);
  }
#endif
}

bool Obstacle::OnPlayerCollision(GameScene *scene, Player *player) {
  if (!scene || !player || !behavior_)
    return false;
  return behavior_->OnPlayerCollision(*this, scene, player);
}

bool Obstacle::IsTransparent() const {
  if (currentModel_) {
    BlendMode blend = currentModel_->GetBlend();
    if (blend != BlendMode::kBlendModeNone && blend != BlendMode::kBlendModeNormal) {
      return true;
    }
    if (currentModel_->GetMaterial()) {
      if (currentModel_->GetMaterial()->GetColor().w < 0.999f) {
        return true;
      }
    }
  }
  return GameObject::IsTransparent();
}

AABB Obstacle::GetColliderAABB() const {
  return Collision::MakeAABB(transform_, collisionWidth_, collisionHeight_,
                             collisionDepth_);
}


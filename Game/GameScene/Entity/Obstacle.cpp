#include "Obstacle.h"
#include "Graphics/Render/Draw.h"
#include "Resource/AssetManager.h"
#include "Math/Calculation.h"
#include <cmath>

Obstacle::Obstacle() {
  name_ = "Obstacle";
  isActive_ = false;
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
  lowModel_->GetMaterial()->SetColor(kNormalColor);

  highModel_->Initialize(highData);
  highModel_->SetShader("ObjectShader");
  highModel_->GetMaterial()->SetColor(kNormalColor);

  wallModel_->Initialize(wallData);
  wallModel_->SetShader("ObjectShader");
  wallModel_->GetMaterial()->SetColor(kNormalColor);

  bonusModel_->Initialize(bonusData);
  bonusModel_->SetShader("ObjectShader");
  bonusModel_->GetMaterial()->SetColor(kNormalColor);

  itemModel_->Initialize(wallData);
  itemModel_->SetShader("ObjectShader");

  iceBomModel_->Initialize(iceBomData);
  iceBomModel_->SetShader("ObjectShader");
  iceBomModel_->GetMaterial()->SetColor(kNormalColor);

  reflectingAttackModel_->Initialize(reflectingAttackData);
  reflectingAttackModel_->SetShader("ObjectShader");
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

void Obstacle::SetType(Type type) {
  type_ = type;

  // タイプに応じてモデル、当たり判定サイズ、スケール、回転を設定
  // 前のサイズ: Low(幅1.5, 高さ1.0, 奥行1.0), High/Wall(幅1.5, 高さ3.0, 奥行1.0) に厳密に一致させる
  const ObstacleProfile *profile = nullptr;

  switch (type_) {
  case Type::Low:
    currentModel_ = lowModel_.get();
    profile = &kProfileLow;
    if (currentModel_) {
      currentModel_->GetMaterial()->SetColor(kNormalColor);
      currentModel_->SetShader("ObjectShader");
      currentModel_->SetBlend(BlendMode::kBlendModeNormal);
    }
    break;

  case Type::High:
    currentModel_ = highModel_.get();
    profile = &kProfileHigh;
    if (currentModel_) {
      currentModel_->GetMaterial()->SetColor(kNormalColor);
      currentModel_->SetShader("ObjectShader");
      currentModel_->SetBlend(BlendMode::kBlendModeNormal);
    }
    break;

  case Type::Wall:
    currentModel_ = wallModel_.get();
    profile = &kProfileWall;
    if (currentModel_) {
      currentModel_->GetMaterial()->SetColor({1.0f, 1.0f, 1.0f, 0.6f});
      currentModel_->SetShader("ObjectShader");
      currentModel_->SetBlend(BlendMode::kBlendModeNormal);
    }
    break;

  case Type::Bonus:
    currentModel_ = bonusModel_.get();
    profile = &kProfileBonus;
    if (currentModel_) {
      currentModel_->GetMaterial()->SetColor(kNormalColor);
      currentModel_->SetShader("ObjectShader");
      currentModel_->SetBlend(BlendMode::kBlendModeNormal);
    }
    break;

  case Type::GuideFloor:
    currentModel_ = itemModel_.get();
    profile = &kProfileGuideFloor;
    if (currentModel_) {
      currentModel_->GetMaterial()->SetColor(kGuideFloorColor);
      currentModel_->SetShader("ObjectShader");
    }
    break;

  case Type::CameraItem:
    currentModel_ = oneLaneItemModel_.get();
    profile = &kProfileItemBillboardSmall;
    break;

  case Type::BarrierItem:
    currentModel_ = barrierItemModel_.get();
    profile = &kProfileItemBillboardSmall;
    break;

  case Type::ClearItem:
    currentModel_ = clearItemModel_.get();
    profile = &kProfileItemBillboardSmall;
    break;

  case Type::BossItem:
    currentModel_ = bossItemModel_.get();
    profile = &kProfileBossItem;
    break;

  case Type::BossAttack:
    currentModel_ = wallModel_.get();
    profile = &kProfileWall;
    if (currentModel_) {
      currentModel_->GetMaterial()->SetColor(kNormalColor);
      currentModel_->SetShader("ObjectShader");
      currentModel_->SetBlend(BlendMode::kBlendModeNormal);
    }
    break;

  case Type::BossAttackReflectable:
    currentModel_ = reflectingAttackModel_.get();
    profile = &kProfileBossAttackReflectable;
    if (currentModel_) {
      currentModel_->GetMaterial()->SetColor(kNormalColor);
      currentModel_->SetShader("ObjectShader");
      currentModel_->SetBlend(BlendMode::kBlendModeNormal);
    }
    break;

  case Type::BossAttackLow:
    currentModel_ = lowModel_.get();
    profile = &kProfileLow;
    if (currentModel_) {
      currentModel_->GetMaterial()->SetColor(kNormalColor);
      currentModel_->SetShader("ObjectShader");
      currentModel_->SetBlend(BlendMode::kBlendModeNormal);
    }
    break;

  case Type::BossAttackHigh:
    currentModel_ = highModel_.get();
    profile = &kProfileHigh;
    if (currentModel_) {
      currentModel_->GetMaterial()->SetColor(kNormalColor);
      currentModel_->SetShader("ObjectShader");
      currentModel_->SetBlend(BlendMode::kBlendModeNormal);
    }
    break;
  }

  if (profile) {
    collisionWidth_ = profile->collisionSize.x;
    collisionHeight_ = profile->collisionSize.y;
    collisionDepth_ = profile->collisionSize.z;
    transform_.scale = profile->scale;
    transform_.rotate = profile->rotate;
  }

  if (currentModel_) {
    // 新モデルは底面原点(Y=0)のため、中心座標から高さの半分を引いて底面を合わせる
    Transform drawTransform = transform_;
    if (currentModel_ == lowModel_.get() || currentModel_ == highModel_.get() ||
        currentModel_ == wallModel_.get() ||
        currentModel_ == iceBomModel_.get() ||
        currentModel_ == reflectingAttackModel_.get() ||
        currentModel_ == bonusModel_.get()) {
      drawTransform.translate.y -= collisionHeight_ * kScaleOffsetFactor;
    }
    currentModel_->SetTransform(drawTransform);
  }
}

void Obstacle::Spawn(float x, float y, float z) {
  targetY_ = y;
  if (IsBossAttackType()) {
    isFalling_ = true;
    fallTimer_ = 0.0f;
    justLanded_ = false;
    transform_.translate = {x, targetY_ + dropHeight_, z};
  } else {
    isFalling_ = false;
    justLanded_ = false;
    transform_.translate = {x, y, z};
  }
  if (type_ == Type::BarrierItem || type_ == Type::BossItem ||
      type_ == Type::ClearItem || type_ == Type::CameraItem) {
    transform_.translate.y += kItemSpawnHeightOffset;
  }
  isActive_ = true;
  isHit_ = false; // 初期化
  isReflected_ = false;
  reflectTimer_ = 0.0f;
  if (currentModel_) {
    Transform drawTransform = transform_;
    if (currentModel_ == lowModel_.get() || currentModel_ == highModel_.get() ||
        currentModel_ == wallModel_.get() ||
        currentModel_ == iceBomModel_.get() ||
        currentModel_ == reflectingAttackModel_.get() ||
        currentModel_ == bonusModel_.get()) {
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
    // スクロール
    if (IsBossAttackType()) {
      // 上空からの落下演出
      if (isFalling_) {
        fallTimer_ += 1.0f;
        float t = fallTimer_ / fallDuration_;
        if (t >= 1.0f) {
          t = 1.0f;
          isFalling_ = false;
          justLanded_ = true;
          transform_.translate.y = targetY_;
        } else {
          // Ease-In (重力加速): t * t
          float easeT = t * t;
          float startY = targetY_ + dropHeight_;
          transform_.translate.y = startY + (targetY_ - startY) * easeT;
        }
      }

      // ボスの攻撃は奥から手前(+Z方向)へ
      transform_.translate.z += scrollSpeed;
      if (transform_.translate.z > kBossAttackDespawnZ) {
        isActive_ = false;
        isFalling_ = false;
      }
    } else {
      // 手前にスクロール
      transform_.translate.z -= scrollSpeed;

      // カメラの後ろ（手前）を過ぎたら非アクティブにする
      if (transform_.translate.z < kPlayerObstacleDespawnZ) {
        isActive_ = false;
      }
    }
  }

  if (currentModel_) {
    Transform drawTransform = transform_;
    if (currentModel_ == lowModel_.get() || currentModel_ == highModel_.get() ||
        currentModel_ == wallModel_.get() ||
        currentModel_ == iceBomModel_.get() ||
        currentModel_ == reflectingAttackModel_.get() ||
        currentModel_ == bonusModel_.get()) {
      drawTransform.translate.y -= collisionHeight_ * kScaleOffsetFactor;
    } else if (type_ == Type::BarrierItem || type_ == Type::BossItem ||
               type_ == Type::ClearItem || type_ == Type::CameraItem) {
      // ビルボードアイテムの浮遊ボビング演出（上下に優しく揺れる）
      itemFloatTimer_ += kItemBobbingSpeed;
      drawTransform.translate.y += std::sin(itemFloatTimer_ + transform_.translate.x * kItemBobbingPhaseMultiplier) * kItemBobbingAmplitude;
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
  isHit_ = true;
  isFalling_ = false;
  // 上と手前(画面方向)に勢いよく飛ぶ
  float randX = ((float)rand() / RAND_MAX - 0.5f) * kBlowAwayRandRangeX;
  velocity_ = {randX, kBlowAwayInitialVelocityY, kBlowAwayInitialVelocityZ};
}

void Obstacle::OnHit() {
  if (isHit_)
    return;
  isHit_ = true;
  isFalling_ = false;

  // アイテム取得時は即座に非アクティブ（消去）にする
  if (type_ == Type::BarrierItem || type_ == Type::BossItem ||
      type_ == Type::ClearItem || type_ == Type::CameraItem) {
    isActive_ = false;
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

#include "../GameScene.h"
#include "Player.h"
#include "../Camera/GameCameraController.h"
#include "../System/ScoreManager.h"
#include "../Stage/StageSettings.h"
#include "../Effect/GameSceneEffect.h"
#include "../System/SoundManager.h"

bool Obstacle::OnPlayerCollision(GameScene* scene, Player* player)
{
  if (!scene || !player) return false;

  if (type_ == Type::GuideFloor) {
    player->StartForceToCenter(kBarrierForceToCenterFrames);
    SoundManager::GetInstance()->PlaySE(SoundManager::SE::Slide);
    return false;
  }

  if (type_ == Type::Bonus) {
    OnBlowAway();
    if (scene->GetScoreManager()) {
      scene->GetScoreManager()->AddBonusEnemyHit();
    }
    if (scene->GetCameraController()) {
      scene->GetCameraController()->ShakeDefeat();
    }
    SoundManager::GetInstance()->PlaySE(SoundManager::SE::BonusHit);
    return false;
  }

  if (type_ == Type::CameraItem) {
    OnHit();
    scene->ChangePlayingState(GameScene::PlayingState::OneLane);
    SoundManager::GetInstance()->PlaySE(SoundManager::SE::ItemGet);
    return false;
  }

  if (type_ == Type::BarrierItem) {
    OnHit();
    player->SetHasBarrier(true);
    SoundManager::GetInstance()->PlaySE(SoundManager::SE::Barrier);
    if (scene->GetEffectManager()) {
      scene->GetEffectManager()->EmitBarrier(player->GetTransform().translate);
    }
    return false;
  }

  if (type_ == Type::ClearItem) {
    OnHit();
    SoundManager::GetInstance()->PlaySE(SoundManager::SE::ClearBomb);
    if (scene->GetCameraController()) {
      scene->GetCameraController()->ShakeDefeat();
    }
    if (scene->GetStageSettings()) {
      for (int j = 0; j < scene->GetStageSettings()->GetMaxObstacles(); j++) {
        Obstacle *obs = scene->GetStageSettings()->GetObstacle(j);
        if (obs && obs->GetIsActive() && obs->IsRegularObstacle()) {
          obs->OnBlowAway();
        }
      }
    }
    return false;
  }

  if (type_ == Type::BossItem) {
    OnHit();
    SoundManager::GetInstance()->PlaySE(SoundManager::SE::Barrier);
    if (scene->GetStageSettings()) {
      for (int j = 0; j < scene->GetStageSettings()->GetMaxObstacles(); j++) {
        Obstacle *obs = scene->GetStageSettings()->GetObstacle(j);
        if (obs && obs->GetIsActive() && obs->IsRegularObstacle()) {
          obs->OnBlowAway();
        }
      }
    }
    scene->ChangePlayingState(GameScene::PlayingState::Boss);
    return false;
  }

  if (IsBossAttackType()) {
    if (isReflected_ || isFalling_) {
      return false;
    }
  }

  if (player->GetHasBarrier()) {
    player->SetHasBarrier(false);
    OnBlowAway();
    if (scene->GetCameraController()) {
      scene->GetCameraController()->ShakeDefeat();
    }
    SoundManager::GetInstance()->PlaySE(SoundManager::SE::BarrierBreak);
    if (scene->GetEffectManager()) {
      scene->GetEffectManager()->BreakBarrier(player->GetTransform().translate);
    }
    return false;
  }

  // 被弾
  bool isTrip = (type_ == Type::Low || type_ == Type::BossAttackLow);
  scene->TriggerPlayerHit(isTrip);
  return true;
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

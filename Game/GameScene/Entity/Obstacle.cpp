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
  lowModel_->GetMaterial()->SetColor({1.0f, 1.0f, 1.0f, 1.0f});

  highModel_->Initialize(highData);
  highModel_->SetShader("ObjectShader");
  highModel_->GetMaterial()->SetColor({1.0f, 1.0f, 1.0f, 1.0f});

  wallModel_->Initialize(wallData);
  wallModel_->SetShader("ObjectShader");
  wallModel_->GetMaterial()->SetColor({1.0f, 1.0f, 1.0f, 1.0f});

  bonusModel_->Initialize(bonusData);
  bonusModel_->SetShader("ObjectShader");
  bonusModel_->GetMaterial()->SetColor({1.0f, 1.0f, 1.0f, 1.0f});

  itemModel_->Initialize(wallData);
  itemModel_->SetShader("ObjectShader");

  iceBomModel_->Initialize(iceBomData);
  iceBomModel_->SetShader("ObjectShader");
  iceBomModel_->GetMaterial()->SetColor({1.0f, 1.0f, 1.0f, 1.0f});

  reflectingAttackModel_->Initialize(reflectingAttackData);
  reflectingAttackModel_->SetShader("ObjectShader");
  reflectingAttackModel_->GetMaterial()->SetColor({1.0f, 1.0f, 1.0f, 1.0f});

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
  barrierItemModel_->GetMaterial()->SetColor({1.0f, 1.0f, 1.0f, 1.0f});

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
  bossItemModel_->GetMaterial()->SetColor({1.0f, 1.0f, 1.0f, 1.0f});

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
  clearItemModel_->GetMaterial()->SetColor({1.0f, 1.0f, 1.0f, 1.0f});

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
  oneLaneItemModel_->GetMaterial()->SetColor({1.0f, 1.0f, 1.0f, 1.0f});

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
  switch (type_) {
  case Type::Low:
    // 倒木（ジャンプで避ける低い障害物）
    currentModel_ = lowModel_.get();
    collisionWidth_ = 1.5f;
    collisionHeight_ = 1.0f;
    collisionDepth_ = 1.0f;
    // FallenTree: ローカルX=0.712, Y=0.639, Z=0.999 をY90度回転して幅1.5, 高1.0, 奥1.0にする
    transform_.scale = {1.404f, 1.565f, 1.502f};
    transform_.rotate = {0.0f, kPiOver2, 0.0f}; // 横向きに倒れる
    if (currentModel_) {
      currentModel_->GetMaterial()->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
      currentModel_->SetShader("ObjectShader");
      currentModel_->SetBlend(BlendMode::kBlendModeNormal);
    }
    break;

  case Type::High:
    // 氷のアーチ（転がり・スライディングで下を潜り抜ける高い障害物）
    currentModel_ = highModel_.get();
    collisionWidth_ = 1.5f;
    collisionHeight_ = 3.0f;
    collisionDepth_ = 1.0f;
    // IceArchway: ローカルX=0.331, Y=0.939, Z=0.958 をY90度回転して幅1.5, 高3.0, 奥1.0にする
    transform_.scale = {3.021f, 3.195f, 1.566f};
    transform_.rotate = {0.0f, kPiOver2, 0.0f}; // 開口部をZ軸方向に向ける
    if (currentModel_) {
      currentModel_->GetMaterial()->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
      currentModel_->SetShader("ObjectShader");
      currentModel_->SetBlend(BlendMode::kBlendModeNormal);
    }
    break;

  case Type::Wall:
    // 氷の壁（レーン移動で避ける壁）
    currentModel_ = wallModel_.get();
    collisionWidth_ = 1.5f;
    collisionHeight_ = 3.0f;
    collisionDepth_ = 1.0f;
    // IceWall: ローカルX=1.002, Y=0.772, Z=0.319 を幅1.5, 高3.0, 奥1.0にする
    transform_.scale = {1.497f, 3.886f, 3.135f};
    transform_.rotate = {0.0f, 0.0f, 0.0f};
    if (currentModel_) {
      currentModel_->GetMaterial()->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
      currentModel_->SetShader("ObjectShader");
      currentModel_->SetBlend(BlendMode::kBlendModeNormal);
    }
    break;

  case Type::Bonus:
    // 当たると吹き飛ぶボーナスエネミー
    currentModel_ = bonusModel_.get();
    collisionWidth_ = 1.0f;
    collisionHeight_ = 1.0f;
    collisionDepth_ = 1.0f;
    transform_.scale = {1.0f, 1.0f, 1.0f};
    transform_.rotate = {0.0f, kPi, 0.0f};
    if (currentModel_) {
      currentModel_->GetMaterial()->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
      currentModel_->SetShader("ObjectShader");
      currentModel_->SetBlend(BlendMode::kBlendModeNormal);
    }
    break;

  case Type::GuideFloor:
    // 中央へ誘導するトリガー床
    currentModel_ = itemModel_.get();
    collisionWidth_ = 2.0f;
    collisionHeight_ = 0.5f;
    collisionDepth_ = 2.0f;
    transform_.scale = {2.0f, 0.1f, 10.0f};
    transform_.rotate = {0.0f, 0.0f, 0.0f};
    if (currentModel_) {
      currentModel_->GetMaterial()->SetColor({0.0f, 1.0f, 1.0f, 0.5f});
      currentModel_->SetShader("ObjectShader");
    }
    break;

  case Type::CameraItem:
    // 一レーンにするアイテム（Onelane ビルボード）
    currentModel_ = oneLaneItemModel_.get();
    collisionWidth_ = 1.0f;
    collisionHeight_ = 1.0f;
    collisionDepth_ = 1.0f;
    transform_.scale = {0.8f, 0.8f, 1.0f};
    transform_.rotate = {0.0f, 0.0f, 0.0f};
    break;

  case Type::BarrierItem:
    // バリアを張るアイテム（shieldItem ビルボード）
    currentModel_ = barrierItemModel_.get();
    collisionWidth_ = 1.0f;
    collisionHeight_ = 1.0f;
    collisionDepth_ = 1.0f;
    transform_.scale = {0.8f, 0.8f, 1.0f};
    transform_.rotate = {0.0f, 0.0f, 0.0f};
    break;

  case Type::ClearItem:
    // レーン消去アイテム（BomItem ビルボード）
    currentModel_ = clearItemModel_.get();
    collisionWidth_ = 1.0f;
    collisionHeight_ = 1.0f;
    collisionDepth_ = 1.0f;
    transform_.scale = {0.8f, 0.8f, 1.0f};
    transform_.rotate = {0.0f, 0.0f, 0.0f};
    break;

  case Type::BossItem:
    // ボス戦アイテム（BossItems ビルボード）
    currentModel_ = bossItemModel_.get();
    collisionWidth_ = 1.5f;
    collisionHeight_ = 1.5f;
    collisionDepth_ = 1.5f;
    transform_.scale = {1.1f, 1.1f, 1.0f};
    transform_.rotate = {0.0f, 0.0f, 0.0f};
    break;

  case Type::BossAttack:
    // ボスの氷爆弾攻撃（IceBom / 回避専用）
    currentModel_ = iceBomModel_.get();
    collisionWidth_ = 1.6f;
    collisionHeight_ = 2.0f;
    collisionDepth_ = 1.6f;
    // IceBom: 約100x94x99 を幅約1.6, 高約1.5, 奥約1.6にする
    transform_.scale = {0.016f, 0.016f, 0.016f};
    transform_.rotate = {0.0f, 0.0f, 0.0f};
    if (currentModel_) {
      currentModel_->GetMaterial()->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
      currentModel_->SetShader("ObjectShader");
      currentModel_->SetBlend(BlendMode::kBlendModeNormal);
    }
    break;

  case Type::BossAttackReflectable:
    // ボスの跳ね返し可能攻撃（ReflectingAttack）
    currentModel_ = reflectingAttackModel_.get();
    collisionWidth_ = 1.6f;
    collisionHeight_ = 2.0f;
    collisionDepth_ = 2.5f;
    // ReflectingAttack: 約58x71x99 を幅約1.44, 高約1.78, 奥約2.49にする (+Zが前方)
    transform_.scale = {0.025f, 0.025f, 0.025f};
    transform_.rotate = {0.0f, 0.0f, 0.0f};
    if (currentModel_) {
      currentModel_->GetMaterial()->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
      currentModel_->SetShader("ObjectShader");
      currentModel_->SetBlend(BlendMode::kBlendModeNormal);
    }
    break;
  }

  if (currentModel_) {
    // 新モデルは底面原点(Y=0)のため、中心座標から高さの半分を引いて底面を合わせる
    Transform drawTransform = transform_;
    if (currentModel_ == lowModel_.get() || currentModel_ == highModel_.get() ||
        currentModel_ == wallModel_.get() ||
        currentModel_ == iceBomModel_.get() ||
        currentModel_ == reflectingAttackModel_.get() ||
        currentModel_ == bonusModel_.get()) {
      drawTransform.translate.y -= collisionHeight_ * 0.5f;
    }
    currentModel_->SetTransform(drawTransform);
  }
}

void Obstacle::Spawn(float x, float y, float z) {
  targetY_ = y;
  if (type_ == Type::BossAttack || type_ == Type::BossAttackReflectable) {
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
    transform_.translate.y += 0.5f;
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
      drawTransform.translate.y -= collisionHeight_ * 0.5f;
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

    transform_.rotate.x -= 0.15f; // 回転させる
    transform_.rotate.y += 0.1f;
    transform_.rotate.z += 0.05f;

    // カメラにぶつかる演出（カメラのZ座標付近に来たら）
    if (transform_.translate.z < -13.0f && velocity_.z < 0.0f) {
      velocity_.z = 0.0f;  // 手前に来るのを止める（画面に張り付いたような演出）
      velocity_.y = -0.1f; // 下に落ち始める
      velocity_.x = ((float)rand() / RAND_MAX - 0.5f) * 0.1f; // 横に少しずれる
    }

    if (transform_.translate.y < -20.0f) {
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
    transform_.rotate.x -= 0.35f;
    transform_.rotate.y += 0.25f;
    transform_.rotate.z += 0.15f;
  } else {
    // スクロール
    if (type_ == Type::BossAttack || type_ == Type::BossAttackReflectable) {
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
      if (transform_.translate.z > 20.0f) {
        isActive_ = false;
        isFalling_ = false;
      }
    } else {
      // 手前にスクロール
      transform_.translate.z -= scrollSpeed;

      // カメラの後ろ（手前）を過ぎたら非アクティブにする
      if (transform_.translate.z < -10.0f) {
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
      drawTransform.translate.y -= collisionHeight_ * 0.5f;
    } else if (type_ == Type::BarrierItem || type_ == Type::BossItem ||
               type_ == Type::ClearItem || type_ == Type::CameraItem) {
      // ビルボードアイテムの浮遊ボビング演出（上下に優しく揺れる）
      itemFloatTimer_ += 0.05f;
      drawTransform.translate.y += std::sin(itemFloatTimer_ + transform_.translate.x * 2.0f) * 0.15f;
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
  float randX = ((float)rand() / RAND_MAX - 0.5f) * 0.4f;
  velocity_ = {randX, 0.6f, -0.8f};
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

#include "StageSettings.h"
#include "AssetManager.h"
#include "Core/LogHandler.h"
#include "PSO/PipelineState.h"
#include "Graphics/Render/Draw.h"
#include "Graphics/Render/LineRenderer.h"
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <format>
#include <imgui.h>


void StageSettings::Initialize(
    ModelData roadModelData, ModelData fallenTreeModelData,
    ModelData iceArchwayModelData, ModelData iceWallModelData,
    ModelData bonusModelData, ModelData iceBomModelData,
    ModelData reflectingAttackModelData, class GameObjectManager *manager) {
  // 乱数の初期化
  std::srand(static_cast<unsigned int>(std::time(nullptr)));

  // 初回生成間隔の計算
  CalculateNextObstacleInterval();

  // グラウンドテクスチャをロード
  texture_->CreateTexture("Resources/Model/Ground/Ground.png");
  texture_->CreateTexture("Resources/Texture/white64x64.png");

  planeModelData_ =
      AssetManager::LoadModel("Resources/Model/obj", "ocean_plane.obj");
  driftIceModelData_ =
      AssetManager::LoadModel("Resources/Model/DriftIce", "DriftIce.obj");
  babySealModelData_ =
      AssetManager::LoadModel("Resources/Model/BabySeal", "BabySeal.obj");

  roadModelData_ = roadModelData;
  manager_ = manager;

  // 道路チャンクの初期化
  GenerateRoadChunks();

  // 海に浮かぶ流氷の初期化
  GenerateDriftIce();

  // 障害物の初期化
  for (int i = 0; i < kMaxObstacles_; i++) {
    obstacles_[i] = std::make_shared<Obstacle>();
    obstacles_[i]->SetName("Obstacle " + std::to_string(i));

    // ランダムなタイプで初期化
    Obstacle::Type type = static_cast<Obstacle::Type>(std::rand() % kInitialObstacleTypeCount);
    obstacles_[i]->Initialize(fallenTreeModelData, iceArchwayModelData,
                              iceWallModelData, bonusModelData, iceBomModelData,
                              reflectingAttackModelData, type);

    if (manager)
      manager->AddObject(obstacles_[i]);
  }

  // アイテムクールタイム設定の初期化（秒単位）
  itemCoolDowns_[Obstacle::Type::Bonus] = {kBonusCooldownDuration, 0.0f};        // ボーナス
  itemCoolDowns_[Obstacle::Type::BarrierItem] = {kBarrierCooldownDuration, 0.0f}; // バリア
  itemCoolDowns_[Obstacle::Type::ClearItem] = {kClearCooldownDuration, 0.0f};   // 全消去
  itemCoolDowns_[Obstacle::Type::CameraItem] = {
      kCameraCooldownDuration, kCameraInitialCooldown}; // カメラ（開始時猶予あり）
  itemCoolDowns_[Obstacle::Type::BossItem] = {
      kBossCooldownDuration, kBossInitialCooldown}; // ボス（開始時猶予あり）
}

void StageSettings::Initialize(ModelData roadModelData,
                               ModelData fallenTreeModelData,
                               ModelData iceArchwayModelData,
                               ModelData iceWallModelData,
                               ModelData bonusModelData,
                               class GameObjectManager *manager) {
  Initialize(roadModelData, fallenTreeModelData, iceArchwayModelData,
             iceWallModelData, bonusModelData, iceWallModelData,
             iceWallModelData, manager);
}

void StageSettings::Initialize(ModelData roadModelData,
                               ModelData obstacleModelData,
                               ModelData bonusModelData,
                               class GameObjectManager *manager) {
  Initialize(roadModelData, obstacleModelData, obstacleModelData,
             obstacleModelData, bonusModelData, manager);
}

void StageSettings::CalculateNextObstacleInterval() {
  // 猶予フレームを最小〜最大の間でランダムに選定（等間隔にならないようにバリエーションを持たせる）
  float graceFrames = minGraceFrames_; // 決定された猶予フレーム数
  int frameRange =
      static_cast<int>(maxGraceFrames_ - minGraceFrames_); // 変動フレーム幅
  if (frameRange > 0) {
    graceFrames += static_cast<float>(std::rand() % (frameRange + 1));
  }

  // 移動速度（スクロール速度）に（アクション所要フレーム＋猶予フレーム）を乗算して次回間隔（距離）を計算
  float effectiveSpeed =
      (std::max)(scrollSpeed_,
                 kMinEffectiveSpeed); // 停止時や低速時の0除算・0距離を防ぐ実効速度
  float totalFrames =
      baseActionFrames_ + graceFrames; // 回避に必要な合計フレーム数
  float calculatedDistance =
      effectiveSpeed * totalFrames; // 移動速度と猶予フレームから計算された距離

  // 確実に避けられる距離範囲（最小距離〜最大距離）に制限
  // 速度が高速になってもアクション完了＋最小猶予フレーム（5f）を下回らないよう上限を動的ガード
  float minRequiredDistance =
      effectiveSpeed * (baseActionFrames_ + minGraceFrames_);
  float dynamicMaxDistance =
      (std::max)(maxObstacleDistance_, minRequiredDistance);

  obstacleInterval_ =
      (std::min)((std::max)(minObstacleDistance_, calculatedDistance),
                 dynamicMaxDistance); // 次回生成までの距離
}

const StageSettings::ChunkRowInfo &

StageSettings::GetChunkRowInfoAtZ(float z) const {
  float defaultWidth =
      (laneCount_ == 1) ? (laneWidth_ * oneLaneWidthMultiplier_) : laneWidth_;
  static ChunkRowInfo fallbackInfo;
  fallbackInfo = {laneCount_, minLaneIndex_, maxLaneIndex_, defaultWidth};

  if (chunkRowInfos_.empty() || roadTransforms_.empty()) {
    return fallbackInfo;
  }

  int closestIdx = -1;
  float minDiff = 1000000.0f;
  for (int i = 0; i < kChunkCount_; i++) {
    if (i < roadTransforms_.size() && !roadTransforms_[i].empty()) {
      float chunkZ = roadTransforms_[i][0].translate.z;
      float diff = std::abs(chunkZ - z);
      if (diff < minDiff) {
        minDiff = diff;
        closestIdx = i;
      }
    }
  }

  if (closestIdx >= 0 && closestIdx < static_cast<int>(chunkRowInfos_.size())) {
    return chunkRowInfos_[closestIdx];
  }
  return fallbackInfo;
}

int StageSettings::GetLaneCountAtZ(float z) const {
  return GetChunkRowInfoAtZ(z).laneCount;
}

int StageSettings::GetMinLaneIndexAtZ(float z) const {
  return GetChunkRowInfoAtZ(z).minLaneIndex;
}

int StageSettings::GetMaxLaneIndexAtZ(float z) const {
  return GetChunkRowInfoAtZ(z).maxLaneIndex;
}

float StageSettings::GetEffectiveLaneWidthAtZ(float z) const {
  return GetChunkRowInfoAtZ(z).effectiveLaneWidth;
}

void StageSettings::SetLaneCount(int count) {
  if (count < 1)
    count = 1;
  if (targetLaneCount_ == count)
    return;
  targetLaneCount_ = count;

  // 画面外の奥（Z >= 70.0f）にあるチャンクを targetLaneCount_ に事前変換する
  // これにより、画面内（プレイヤー視界内）の床は変えずに、地平線奥から即座に新レーンが出現して流れてくる
  for (int i = 0; i < kChunkCount_; i++) {
    if (i < roadTransforms_.size() && !roadTransforms_[i].empty()) {
      if (roadTransforms_[i][0].translate.z >= kLaneChangeHorizonZ) {
        RebuildChunkRow(i, targetLaneCount_, roadTransforms_[i][0].translate.z,
                        IdentityMatrix());
      }
    }
  }
}

void StageSettings::SetLaneCountImmediate(int count, Matrix4x4 view) {
  if (count < 1)
    count = 1;
  targetLaneCount_ = count;
  laneCount_ = count;
  minLaneIndex_ = -(laneCount_ / 2);
  maxLaneIndex_ = (laneCount_ - 1) / 2;
  isDirty_ = true;
}

void StageSettings::SetRoadColor(const Vector4 &color) {
  roadColor_ = color;
  for (auto &row : roadChunks_) {
    for (auto &chunk : row) {
      if (chunk) {
        if (auto model = dynamic_cast<Model *>(chunk->GetObjectBase().get())) {
          model->SetColor(roadColor_);
        }
      }
    }
  }
  for (auto &chunk : chunkPool_) {
    if (chunk) {
      if (auto model = dynamic_cast<Model *>(chunk->GetObjectBase().get())) {
        model->SetColor(roadColor_);
      }
    }
  }
}

void StageSettings::SetDriftIceColor(const Vector4 &color) {
  driftIceColor_ = color;
  for (auto &ice : driftIces_) {
    if (ice.iceModel) ice.iceModel->SetColor(driftIceColor_);
    if (ice.sealModel) ice.sealModel->SetColor(driftIceColor_);
  }
}

void StageSettings::SetDriftIceEnabled(bool enabled) {
  driftIceEnabled_ = enabled;
  for (auto &ice : driftIces_) {
    if (ice.renderObj) {
      ice.renderObj->SetIsActive(enabled);
    }
  }
}

void StageSettings::SetDriftIceSizeScale(float scale) {
  if (scale <= kMinDriftIceScale)
    scale = kMinDriftIceScale;
  float ratio = scale / (std::max)(driftIceSizeScale_, kMinDriftIceScale);
  driftIceSizeScale_ = scale;
  float halfModelH = (driftIceModelData_.localAABB.max.y - driftIceModelData_.localAABB.min.y) * 0.5f;
  if (halfModelH <= 0.001f) halfModelH = 0.25f;
  for (auto &ice : driftIces_) {
    ice.scale.x *= ratio;
    ice.scale.y *= ratio;
    ice.scale.z *= ratio;
    float topY = ice.baseY + (ice.scale.y / ratio * halfModelH);
    ice.baseY = topY - (ice.scale.y * halfModelH);
  }
}

void StageSettings::SetDriftIceThicknessScale(float scale) {
  if (scale <= kMinDriftIceScale)
    scale = kMinDriftIceScale;
  float ratio = scale / (std::max)(driftIceThicknessScale_, kMinDriftIceScale);
  driftIceThicknessScale_ = scale;
  for (auto &ice : driftIces_) {
    ice.scale.y *= ratio;
  }
}

void StageSettings::SetDriftIceLighting(bool enabled) {
  driftIceLighting_ = enabled;
  for (auto &ice : driftIces_) {
    if (ice.iceModel) ice.iceModel->SetLighting(enabled);
    if (ice.sealModel) ice.sealModel->SetLighting(enabled);
  }
}

void StageSettings::SetBabySealSpinSpeed(float speed) {
  babySealSpinSpeed_ = speed;
  for (auto &ice : driftIces_) {
    if (ice.type == DriftIce::Type::BabySeal) {
      float sign = (ice.rotSpeed >= 0.0f) ? 1.0f : -1.0f;
      float speedVariation = 0.85f + static_cast<float>(std::rand() % 31) / 100.0f;
      ice.rotSpeed = sign * babySealSpinSpeed_ * speedVariation;
    }
  }
}

void StageSettings::SetWaterForwardExtension(float ext) {
  waterForwardExtension_ = (std::max)(0.0f, ext);
  UpdateSidePlanesTransform();
}

void StageSettings::SetWaterBackwardExtension(float ext) {
  waterBackwardExtension_ = (std::max)(0.0f, ext);
  UpdateSidePlanesTransform();
}

void StageSettings::SetWaterWidthScale(float scale) {
  waterWidthScale_ = (std::max)(kMinWaterWidthScale, scale);
  UpdateSidePlanesTransform();
}

void StageSettings::RebuildChunkRow(int rowIndex, int newLaneCount, float newZ,
                                    Matrix4x4 view) {
  if (rowIndex < 0 || rowIndex >= kChunkCount_)
    return;

  if (chunkRowInfos_.size() < kChunkCount_) {
    chunkRowInfos_.resize(kChunkCount_);
  }
  if (roadChunks_.size() < kChunkCount_) {
    roadChunks_.resize(kChunkCount_);
  }
  if (roadTransforms_.size() < kChunkCount_) {
    roadTransforms_.resize(kChunkCount_);
  }

  int currentCount = static_cast<int>(roadChunks_[rowIndex].size());
  if (currentCount > newLaneCount) {
    for (int l = newLaneCount; l < currentCount; l++) {
      if (manager_ && roadChunks_[rowIndex][l]) {
        manager_->RemoveObject(roadChunks_[rowIndex][l]);
      }
      chunkPool_.push_back(roadChunks_[rowIndex][l]);
    }
    roadChunks_[rowIndex].resize(newLaneCount);
    roadTransforms_[rowIndex].resize(newLaneCount);
  } else if (currentCount < newLaneCount) {
    roadChunks_[rowIndex].resize(newLaneCount);
    roadTransforms_[rowIndex].resize(newLaneCount);
    for (int l = currentCount; l < newLaneCount; l++) {
      std::shared_ptr<RenderObject> renderObj;
      if (!chunkPool_.empty()) {
        renderObj = chunkPool_.back();
        chunkPool_.pop_back();
        if (renderObj) {
          if (auto model = dynamic_cast<Model *>(renderObj->GetObjectBase().get())) {
            model->SetColor(roadColor_);
          }
        }
      } else {
        auto roadModel = std::make_shared<Model>();
        roadModel->Initialize(roadModelData_);
        if (auto matComp = roadModel->GetComponent<MaterialComponent>()) {
          matComp->SetTexturePath("Resources/Model/Ground/Ground.png");
        }
        roadModel->SetLighting(false);
        roadModel->SetTexture(
            texture_->TextureData("Resources/Model/Ground/Ground.png"));
        roadModel->SetColor(roadColor_);

        renderObj = std::make_shared<RenderObject>(roadModel);
      }
      roadChunks_[rowIndex][l] = renderObj;
      if (manager_) {
        manager_->AddObject(renderObj);
      }
    }
  }

  ChunkRowInfo &info = chunkRowInfos_[rowIndex];
  info.laneCount = newLaneCount;
  info.minLaneIndex = -(newLaneCount / 2);
  info.maxLaneIndex = (newLaneCount - 1) / 2;
  info.effectiveLaneWidth =
      (newLaneCount == 1) ? (laneWidth_ * oneLaneWidthMultiplier_) : laneWidth_;

  float modelWidth =
      roadModelData_.localAABB.max.x - roadModelData_.localAABB.min.x;
  float modelHeight =
      roadModelData_.localAABB.max.y - roadModelData_.localAABB.min.y;
  float modelDepth =
      roadModelData_.localAABB.max.z - roadModelData_.localAABB.min.z;
  if (modelWidth <= 0.001f)
    modelWidth = 1.0f;
  if (modelHeight <= 0.001f)
    modelHeight = 1.0f;
  if (modelDepth <= 0.001f)
    modelDepth = 1.0f;

  for (int laneIdx = 0; laneIdx < newLaneCount; laneIdx++) {
    auto &renderObj = roadChunks_[rowIndex][laneIdx];
    renderObj->SetName("RoadChunk_" + std::to_string(rowIndex) + "_Lane_" +
                       std::to_string(laneIdx));

    int logicalLane = info.minLaneIndex + laneIdx;
    float x = static_cast<float>(logicalLane) * info.effectiveLaneWidth;

    Transform t;
    t.scale = {info.effectiveLaneWidth / modelWidth, kRoadChunkHeight / modelHeight,
               kRoadChunkDepthScale / modelDepth};
    t.rotate = {0.0f, 0.0f, 0.0f};
    constexpr float kRoadTopY = 2.0f;
    t.translate = {
        x - t.scale.x * ((roadModelData_.localAABB.min.x +
                          roadModelData_.localAABB.max.x) *
                         0.5f),
        kRoadTopY - (t.scale.y * roadModelData_.localAABB.max.y),
        newZ - t.scale.z * ((roadModelData_.localAABB.min.z +
                            roadModelData_.localAABB.max.z) *
                           0.5f)};

    renderObj->SetTransform(t);
    renderObj->Update(view, 0.0f);

    roadTransforms_[rowIndex][laneIdx] = t;
  }
}

void StageSettings::GenerateRoadChunks(Matrix4x4 view) {
  // 既存のZ座標を保存（ゲーム中にレーン数が変わった際に地面が飛ぶのを防ぐため）
  std::vector<float> currentZs(kChunkCount_);
  for (int zIndex = 0; zIndex < kChunkCount_; zIndex++) {
    if (zIndex < roadTransforms_.size() && !roadTransforms_[zIndex].empty()) {
      currentZs[zIndex] = roadTransforms_[zIndex][0].translate.z;
    } else {
      currentZs[zIndex] =
          static_cast<float>(zIndex - kBackwardChunks_) * chunkLength_;
    }
  }

  targetLaneCount_ = laneCount_;
  chunkRowInfos_.resize(kChunkCount_);
  roadChunks_.resize(kChunkCount_);
  roadTransforms_.resize(kChunkCount_);

  for (int zIndex = 0; zIndex < kChunkCount_; zIndex++) {
    RebuildChunkRow(zIndex, laneCount_, currentZs[zIndex], view);
  }

  // サイドプレーンの生成/更新
  const char *planeNames[2] = {"SidePlaneL", "SidePlaneR"};

  for (int i = 0; i < 2; ++i) {
    LOG_INFO(std::format("GenerateRoadChunks: sidePlane {}", i));
    if (!sidePlanes_[i]) {
      LOG_INFO(
          std::format("GenerateRoadChunks: creating sidePlane model {}", i));
      auto model = std::make_shared<Model>();
      model->Initialize(planeModelData_);
      model->SetShader(WaterShader);
      model->SetColor(kDefaultWaterColor);
      model->SetLighting(true);
      model->SetCullMode(kCullModeNone);
      if (auto matComp = model->GetComponent<MaterialComponent>()) {
        matComp->SetTexturePath("Resources/Texture/white64x64.png");
        matComp->SetShader(WaterShader);
      }
      model->SetTexture(
          texture_->TextureData("Resources/Texture/white64x64.png"));
      sidePlanes_[i] = std::make_shared<RenderObject>(model);
      sidePlanes_[i]->SetName(planeNames[i]);
      if (manager_)
        manager_->AddObject(sidePlanes_[i]);
    }
  }

  UpdateSidePlanesTransform(view);
}

void StageSettings::UpdateSidePlanesTransform(Matrix4x4 view) {
  float effectiveLaneWidth = GetEffectiveLaneWidth();
  float halfLane = effectiveLaneWidth * kHalfFactor;
  float bounds[2] = {static_cast<float>(minLaneIndex_) * effectiveLaneWidth - halfLane,
                     static_cast<float>(maxLaneIndex_) * effectiveLaneWidth + halfLane};
  float halfWaterWidth = waterWidthScale_ * kHalfFactor;
  float offsets[2] = {-halfWaterWidth, halfWaterWidth};

  float minRoadZ = -static_cast<float>(kBackwardChunks_) * chunkLength_;
  float maxRoadZ = static_cast<float>(kForwardChunks_) * chunkLength_;

  // 波のZ範囲：手前・奥ともに道路を大きく超えてカバー
  float minSeaZ = minRoadZ - waterBackwardExtension_;
  float maxSeaZ = maxRoadZ + waterForwardExtension_;
  float seaLength = maxSeaZ - minSeaZ;
  float centerSeaZ = (minSeaZ + maxSeaZ) * kHalfFactor;

  for (int i = 0; i < 2; ++i) {
    if (!sidePlanes_[i])
      continue;
    Transform t;
    // plane.objは2x2 (-1〜+1) なので、Yスケール * 2 = seaLength
    t.scale = {waterWidthScale_, seaLength * kHalfFactor, 1.0f};
    t.rotate = {kPlaneRotationX, 0.0f, 0.0f};
    t.translate = {bounds[i] + offsets[i], 0.0f, centerSeaZ};
    sidePlanes_[i]->SetTransform(t);
    sidePlanes_[i]->Update(view, 0.0f);
  }
}

float StageSettings::CalculateWaterHeight(float x, float z, float time) const {
  struct WaveParam {
    Vector2 dir;
    float amplitude;
    float wavelength;
    float speed;
  };
  static const WaveParam waves[4] = {
    { { 1.0f,   0.25f }, 0.60f, 20.0f, 1.2f },
    { {-0.35f,  0.93f }, 0.35f, 11.0f, 1.4f },
    { { 0.80f, -0.60f }, 0.18f,  5.5f, 1.8f },
    { {-0.50f, -0.86f }, 0.08f,  2.2f, 2.4f },
  };

  constexpr float kPI = 3.14159265f;
  float totalY = 0.0f;
  for (int i = 0; i < 4; ++i) {
    const auto &w = waves[i];
    float len = std::sqrt(w.dir.x * w.dir.x + w.dir.y * w.dir.y);
    float dx = (len > 0.0001f) ? (w.dir.x / len) : 1.0f;
    float dz = (len > 0.0001f) ? (w.dir.y / len) : 0.0f;

    float k = 2.0f * kPI / w.wavelength;
    float c = std::sqrt(kGravityAcceleration / k) * w.speed;
    float phase = k * (dx * x + dz * z - c * time);
    totalY += w.amplitude * std::sin(phase);
  }
  return totalY;
}

void StageSettings::GenerateDriftIce(Matrix4x4 view) {
  auto createModel = [&](const ModelData &mData) {
    auto model = std::make_shared<Model>();
    model->Initialize(mData);
    if (mData.subMeshes.empty() || mData.subMeshes[0].textureIndex == -1) {
      if (auto matComp = model->GetComponent<MaterialComponent>()) {
        matComp->SetTexturePath("Resources/Texture/white64x64.png");
      }
      model->SetTexture(texture_->TextureData("Resources/Texture/white64x64.png"));
    }
    model->SetLighting(driftIceLighting_);
    model->SetColor(driftIceColor_);
    model->SetBlend(kBlendModeNormal);
    return model;
  };

  if (driftIces_.empty()) {
    driftIces_.resize(kDriftIceCount_);
    for (int i = 0; i < kDriftIceCount_; ++i) {
      driftIces_[i].iceModel = createModel(driftIceModelData_);
      driftIces_[i].sealModel = createModel(babySealModelData_);

      auto renderObj = std::make_shared<RenderObject>(driftIces_[i].iceModel);
      renderObj->SetName("DriftIce_" + std::to_string(i));
      renderObj->SetIsActive(driftIceEnabled_);
      if (manager_) {
        manager_->AddObject(renderObj);
      }
      driftIces_[i].renderObj = renderObj;
      SetupSingleDriftIce(driftIces_[i], false, i);
    }
  } else {
    for (int i = 0; i < kDriftIceCount_; ++i) {
      SetupSingleDriftIce(driftIces_[i], false, i);
    }
  }

  UpdateDriftIce(view, 0.0f, 0.0f);
}

void StageSettings::SetupSingleDriftIce(DriftIce &ice, bool spawnFarAway, int /*index*/) {
  // アザラシの出現確率（約15%：およそ6〜7個に1個の割合で時々アザラシが流れてくる）
  bool isSeal = (std::rand() % 100 < kBabySealSpawnChancePercent);
  ice.type = isSeal ? DriftIce::Type::BabySeal : DriftIce::Type::DriftIce;

  const auto &modelData = (ice.type == DriftIce::Type::BabySeal) ? babySealModelData_ : driftIceModelData_;
  auto currentModel = (ice.type == DriftIce::Type::BabySeal) ? ice.sealModel : ice.iceModel;
  if (ice.renderObj && currentModel) {
    ice.renderObj->SetObjectBase(currentModel);
  }

  float modelWidth = modelData.localAABB.max.x - modelData.localAABB.min.x;
  float modelHeight = modelData.localAABB.max.y - modelData.localAABB.min.y;
  float modelDepth = modelData.localAABB.max.z - modelData.localAABB.min.z;
  if (modelWidth <= 0.001f) modelWidth = 1.0f;
  if (modelHeight <= 0.001f) modelHeight = 0.5f;
  if (modelDepth <= 0.001f) modelDepth = 1.0f;

  // Z座標:
  // 規則性を排除し、完全ランダムな広がりで奥から流れてくるように配置
  if (spawnFarAway) {
    float forwardZ = static_cast<float>(kForwardChunks_) * chunkLength_; // 140.0f
    float randomOffset = static_cast<float>(std::rand() % 1000) / 10.0f; // 0.0 〜 100.0m
    ice.posZ = forwardZ + randomOffset;
  } else {
    // 初回配置: 手前 -60m から 奥 160m にかけて一様にランダム配置
    float minZ = -static_cast<float>(kBackwardChunks_) * chunkLength_; // -60.0f
    float totalLength = static_cast<float>(kChunkCount_ + 2) * chunkLength_; // 220.0f
    ice.posZ = minZ + static_cast<float>(std::rand() % static_cast<int>(totalLength * 10)) / 10.0f;
  }

  float targetWidth = DriftIce::kDefaultTargetWidth;
  float targetDepth = DriftIce::kDefaultTargetDepth;
  float targetHeight = DriftIce::kDefaultTargetHeight;
  float floatingHeight = DriftIce::kDefaultFloatingHeight;

  if (ice.type == DriftIce::Type::BabySeal) {
    // 流氷に乗ったアザラシのサイズ感
    targetWidth = kSealIceBaseWidth + static_cast<float>(std::rand() % 12) / 10.0f;
    targetDepth = kSealIceBaseDepth + static_cast<float>(std::rand() % 14) / 10.0f;
    targetHeight = kSealIceBaseHeight + static_cast<float>(std::rand() % 41) / 100.0f;
    floatingHeight = kSealIceBaseFloat + static_cast<float>(std::rand() % 15) / 100.0f;
  } else {
    // 通常の流氷のサイズ設計 (極小 45%, 小〜中 35%, 中〜大型 20%)
    int sizeCategory = std::rand() % 100;
    if (sizeCategory < kIceSmallCategoryChance) {
      targetWidth = kIceSmallWidth + static_cast<float>(std::rand() % 11) / 10.0f;
      targetDepth = kIceSmallDepth + static_cast<float>(std::rand() % 13) / 10.0f;
      targetHeight = kIceSmallHeight + static_cast<float>(std::rand() % 31) / 100.0f;
      floatingHeight = kIceSmallFloat + static_cast<float>(std::rand() % 11) / 100.0f;
    } else if (sizeCategory < kIceMediumCategoryChance) {
      targetWidth = kIceMediumWidth + static_cast<float>(std::rand() % 17) / 10.0f;
      targetDepth = kIceMediumDepth + static_cast<float>(std::rand() % 21) / 10.0f;
      targetHeight = kIceMediumHeight + static_cast<float>(std::rand() % 41) / 100.0f;
      floatingHeight = kIceMediumFloat + static_cast<float>(std::rand() % 16) / 100.0f;
    } else {
      targetWidth = kIceLargeWidth + static_cast<float>(std::rand() % 31) / 10.0f;
      targetDepth = kIceLargeDepth + static_cast<float>(std::rand() % 36) / 10.0f;
      targetHeight = kIceLargeHeight + static_cast<float>(std::rand() % 61) / 100.0f;
      floatingHeight = kIceLargeFloat + static_cast<float>(std::rand() % 21) / 100.0f;
    }
  }

  // 全体サイズ倍率 & 厚み倍率を適用
  targetWidth *= driftIceSizeScale_;
  targetDepth *= driftIceSizeScale_;
  targetHeight *= driftIceSizeScale_ * driftIceThicknessScale_;

  ice.scale = {
    targetWidth / modelWidth,
    targetHeight / modelHeight,
    targetDepth / modelDepth
  };

  ice.floatingHeight = floatingHeight;

  // 潮流・波による横方向の揺らぎ（ドリフト）
  ice.driftAmount = kIceDriftBaseAmount + static_cast<float>(std::rand() % 60) / 100.0f;
  ice.driftSpeed = kIceDriftBaseSpeed + static_cast<float>(std::rand() % 60) / 100.0f;
  ice.driftPhase = static_cast<float>(std::rand() % 628) / 100.0f;

  // 流れる向きの多様化：斜め方向への微小な潮流移動（-0.12m/s 〜 +0.12m/s）
  ice.driftVelocityX = static_cast<float>(std::rand() % 241 - 120) / 1000.0f;

  // 左右の振り分け（左右均等にランダム配置）
  bool isLeft = (std::rand() % 100 < 50);
  float sideSign = isLeft ? -1.0f : 1.0f;

  // 【レーンの床には絶対に来ないようにX座標を決定】
  // そのZ座標での道路端と、将来の最大レーン数を考慮した道路半幅を計算
  auto rowInfo = GetChunkRowInfoAtZ(ice.posZ);
  float roadEdgeAtZ = (std::max)(std::abs(rowInfo.minLaneIndex - 0.5f), std::abs(rowInfo.maxLaneIndex + 0.5f)) * rowInfo.effectiveLaneWidth;
  float targetRoadEdge = (static_cast<float>(targetLaneCount_) * 0.5f) * laneWidth_;
  float roadBoundary = (std::max)(roadEdgeAtZ, targetRoadEdge);

  // 道路端 + 流氷の半径(halfWidth) + 最大横揺れ幅 + 安全マージン
  float halfWidth = targetWidth * 0.5f;
  float minClearance = roadBoundary + halfWidth + ice.driftAmount + kDriftIceClearanceMargin;

  // 最小クリアランスから外洋側（+30m）へ滑らかに分布
  float dist = minClearance + static_cast<float>(std::rand() % 300) / 10.0f;
  ice.posX = sideSign * dist;

  // 【流れる向きなどのバラバラ化】
  // 初期の向き（回転角度Y）：0〜360度ランダム
  ice.rotY = static_cast<float>(std::rand() % 628) / 100.0f;
  if (ice.type == DriftIce::Type::BabySeal) {
    // アザラシは水面をくるくるとスピン（自転）しながら流れる
    float dir = (std::rand() % 2 == 0) ? 1.0f : -1.0f;
    float speedVariation = kIceSealSpinVariationBase + static_cast<float>(std::rand() % 31) / 100.0f;
    ice.rotSpeed = dir * babySealSpinSpeed_ * speedVariation;
  } else {
    // 通常の流氷も右回り・左回り、回転速度にバリエーションを持たせて向きの変化を豊かに
    float dir = (std::rand() % 2 == 0) ? 1.0f : -1.0f;
    ice.rotSpeed = dir * (kIceRotSpeedBase + static_cast<float>(std::rand() % 120) / 1000.0f);
  }

  // 上下の揺れ（ボビング）
  ice.bobbingPhase = static_cast<float>(std::rand() % 628) / 100.0f;
  ice.bobbingSpeed = kIceBobbingBaseSpeed + static_cast<float>(std::rand() % 80) / 100.0f;
  ice.bobbingAmount = kIceBobbingBaseAmount + static_cast<float>(std::rand() % 70) / 1000.0f;
  ice.rollPitchMultiplier = kIceRollPitchBaseMultiplier + static_cast<float>(std::rand() % 18) / 1000.0f;

  // 【流れるスピードは全部同じ（1.0倍に統一）】
  ice.speedMultiplier = 1.0f;
}

void StageSettings::UpdateDriftIce(Matrix4x4 view, float currentScroll, float timeScale) {
  if (!driftIceEnabled_) return;

  float dt = Engine::kDefaultDeltaTime * timeScale;
  float backwardThreshold = -static_cast<float>(kBackwardChunks_ + 1) * chunkLength_ - kDriftIceRespawnSafeDistance;

  for (size_t i = 0; i < driftIces_.size(); ++i) {
    auto &ice = driftIces_[i];
    if (!ice.renderObj) continue;

    const auto &modelData = (ice.type == DriftIce::Type::BabySeal) ? babySealModelData_ : driftIceModelData_;
    float modelWidth = modelData.localAABB.max.x - modelData.localAABB.min.x;
    float modelHeight = modelData.localAABB.max.y - modelData.localAABB.min.y;
    float modelDepth = modelData.localAABB.max.z - modelData.localAABB.min.z;
    if (modelWidth <= 0.001f) modelWidth = 1.0f;
    if (modelHeight <= 0.001f) modelHeight = 0.5f;
    if (modelDepth <= 0.001f) modelDepth = 1.0f;

    // 【流れるスピードは全部同じ】全流氷等速でスクロール
    ice.posZ -= currentScroll;

    // 画面手前を通り過ぎたら奥へリスポーン
    if (ice.posZ < backwardThreshold) {
      SetupSingleDriftIce(ice, true, static_cast<int>(i));
    }

    // 自転ドリフト（アザラシはスピン、流氷も多様に向きが変化）
    ice.rotY += ice.rotSpeed * dt;
    if (ice.rotY > kTwoPi) {
      ice.rotY -= kTwoPi;
    } else if (ice.rotY < 0.0f) {
      ice.rotY += kTwoPi;
    }

    // 斜め方向への緩やかな潮流移動
    ice.posX += ice.driftVelocityX * dt;

    // 横方向（X軸）の漂流
    float sideSign = (ice.posX >= 0.0f) ? 1.0f : -1.0f;
    float drift = sinf(ice.driftPhase + waterTime_ * ice.driftSpeed) * ice.driftAmount;
    float rawX = ice.posX + sideSign * driftIceDistanceOffset_ + drift;

    // 流氷の厚み・幅・奥行きの半分
    float halfWidth = ice.scale.x * (modelWidth * 0.5f);
    float halfDepth = ice.scale.z * (modelDepth * 0.5f);
    float halfHeight = ice.scale.y * (modelHeight * 0.5f);

    // 【レーンの床には絶対に来ないようにリアルタイム安全ガード】
    auto rowInfo = GetChunkRowInfoAtZ(ice.posZ);
    float roadEdgeAtZ = (std::max)(std::abs(rowInfo.minLaneIndex - 0.5f), std::abs(rowInfo.maxLaneIndex + 0.5f)) * rowInfo.effectiveLaneWidth;
    float targetRoadEdge = (static_cast<float>(targetLaneCount_) * 0.5f) * laneWidth_;
    float roadBoundary = (std::max)(roadEdgeAtZ, targetRoadEdge);
    float minSafeX = roadBoundary + halfWidth + kRoadSafeMargin;

    float posXWithOffset = rawX;
    if (sideSign > 0.0f) {
      if (posXWithOffset < minSafeX) {
        posXWithOffset = minSafeX;
        // 斜め潮流で内側へ寄りすぎた場合は外側へ反転
        if (ice.driftVelocityX < 0.0f) ice.driftVelocityX = -ice.driftVelocityX;
      }
    } else {
      if (posXWithOffset > -minSafeX) {
        posXWithOffset = -minSafeX;
        // 斜め潮流で内側へ寄りすぎた場合は外側へ反転
        if (ice.driftVelocityX > 0.0f) ice.driftVelocityX = -ice.driftVelocityX;
      }
    }

    // 【波の傾斜（前後左右の勾配）をサンプリングして波乗り姿勢（pitch/roll）を算出】
    float sampleDistX = (std::max)(halfWidth * 0.8f, 1.0f);
    float sampleDistZ = (std::max)(halfDepth * 0.8f, 1.0f);

    float hCenter = CalculateWaterHeight(posXWithOffset, ice.posZ, waterTime_);
    float hFront  = CalculateWaterHeight(posXWithOffset, ice.posZ + sampleDistZ, waterTime_);
    float hBack   = CalculateWaterHeight(posXWithOffset, ice.posZ - sampleDistZ, waterTime_);
    float hRight  = CalculateWaterHeight(posXWithOffset + sampleDistX, ice.posZ, waterTime_);
    float hLeft   = CalculateWaterHeight(posXWithOffset - sampleDistX, ice.posZ, waterTime_);

    // 波の斜面に沿ったワールド基準ピッチ・ロール
    float waveSlopePitch = -std::atan2(hFront - hBack, 2.0f * sampleDistZ);
    float waveSlopeRoll  =  std::atan2(hRight - hLeft, 2.0f * sampleDistX);

    // 【自転（ice.rotY）を考慮したローカル軸への射影】
    // 流氷が自転しているため、ワールド空間の傾斜勾配を自転角に合わせてローカルのピッチ・ロールに射影する
    float cosYaw = std::cos(ice.rotY);
    float sinYaw = std::sin(ice.rotY);
    float localWavePitch = (waveSlopePitch * cosYaw - waveSlopeRoll * sinYaw);
    float localWaveRoll  = (waveSlopeRoll * cosYaw + waveSlopePitch * sinYaw);
    localWavePitch = Clamp(localWavePitch, -kMaxLocalWaveTilt, kMaxLocalWaveTilt);
    localWaveRoll  = Clamp(localWaveRoll,  -kMaxLocalWaveTilt, kMaxLocalWaveTilt);

    // 波による揺れ（上下・傾き）の更新
    ice.bobbingPhase += ice.bobbingSpeed * driftIceBobbingSpeedScale_ * dt;
    if (ice.bobbingPhase > kTwoPi * 10.0f) {
      ice.bobbingPhase -= kTwoPi * 10.0f;
    }

    // 独自の上下揺れ
    float bobbing = sinf(ice.bobbingPhase) * ice.bobbingAmount * driftIceBobbingScale_;

    // 波が上昇している（波の山にいる）ときは、下向きのボビング揺れをソフトに減衰させて水面への沈み込みを防ぐ
    if (hCenter > 0.0f && bobbing < 0.0f) {
      float damping = 1.0f - Clamp(hCenter / 0.6f, 0.0f, 0.85f);
      bobbing *= damping;
    }

    // 波乗り傾斜に独自の微小揺らぎを加算
    float pitchSway = sinf(ice.bobbingPhase * 0.7f) * ice.rollPitchMultiplier * driftIceBobbingScale_;
    float rollSway  = cosf(ice.bobbingPhase * 0.8f) * ice.rollPitchMultiplier * driftIceBobbingScale_;

    float pitch = localWavePitch * 0.60f + pitchSway;
    float roll  = localWaveRoll  * 0.60f + rollSway;

    // 回転行列の計算
    Matrix4x4 rotMatrix = Rotation(Vector3(pitch, ice.rotY, roll));

    // 【上面9点（中央・四隅・四辺中点）のワールド高さを検証し、絶対に水面に潜らない下限translateYを算出】
    float minX = modelData.localAABB.min.x * ice.scale.x;
    float maxX = modelData.localAABB.max.x * ice.scale.x;
    float minZ = modelData.localAABB.min.z * ice.scale.z;
    float maxZ = modelData.localAABB.max.z * ice.scale.z;
    float maxY = modelData.localAABB.max.y * ice.scale.y; // ローカル上面 (全高)

    Vector3 topPoints[9] = {
      { 0.0f, maxY, 0.0f },
      { minX, maxY, minZ },
      { maxX, maxY, minZ },
      { minX, maxY, maxZ },
      { maxX, maxY, maxZ },
      { 0.0f, maxY, minZ },
      { 0.0f, maxY, maxZ },
      { minX, maxY, 0.0f },
      { maxX, maxY, 0.0f }
    };

    // 最低でも上面のどの角・端も水面から常に出ていることを保証する安全余白
    // 流氷の厚みに応じて、最低でも 40cm〜厚みの60%以上は水面から上に露出させる
    float minSafeAboveWater = (std::max)(kMinSafeAboveWater, halfHeight * kSafeAboveWaterHeightRatio);
    float minAllowedTranslateY = -999999.0f;

    for (int pIdx = 0; pIdx < 9; ++pIdx) {
      Vector3 rotOffset = TransformNormal(topPoints[pIdx], rotMatrix);
      float ptWorldX = posXWithOffset + rotOffset.x;
      float ptWorldZ = ice.posZ + rotOffset.z;
      float waterYAtPt = CalculateWaterHeight(ptWorldX, ptWorldZ, waterTime_);

      // ptWorldY = translateY + rotOffset.y >= waterYAtPt + minSafeAboveWater
      // => translateY >= waterYAtPt + minSafeAboveWater - rotOffset.y
      float reqTranslateY = waterYAtPt + minSafeAboveWater - rotOffset.y;
      if (reqTranslateY > minAllowedTranslateY) {
        minAllowedTranslateY = reqTranslateY;
      }
    }

    // 通常時の目標上面高さ（元のしっかり浮く高さを完全復元）:
    // 上面中心が波より halfHeight + floatingHeight + driftIceHeightOffset_ + bobbing 高い位置になる
    Vector3 centerTopRot = TransformNormal({0.0f, maxY, 0.0f}, rotMatrix);
    float targetTopCenterY = hCenter + halfHeight + ice.floatingHeight + driftIceHeightOffset_ + bobbing;
    float targetTranslateY = targetTopCenterY - centerTopRot.y;

    // 【絶対に潜らない下限ガード】
    // 波が上に行くとき、流氷が下に行くとき、または傾きによって角が下がったときでも、
    // 上面のすべての点が水面より上に露出することを 100% 保証
    if (targetTranslateY < minAllowedTranslateY) {
      targetTranslateY = minAllowedTranslateY;
    }

    Transform t;
    t.scale = ice.scale;
    t.rotate = {pitch, ice.rotY, roll};
    t.translate = {posXWithOffset, targetTranslateY, ice.posZ};

    ice.renderObj->SetTransform(t);
    ice.renderObj->Update(view, 0.0f);
  }
}

void StageSettings::ResetDriftIce() {
  for (int i = 0; i < static_cast<int>(driftIces_.size()); ++i) {
    SetupSingleDriftIce(driftIces_[i], false, i);
  }
}

void StageSettings::Update(Matrix4x4 view, float timeScale, float waterTime) {
  if (waterTime >= 0.0f) {
    waterTime_ = waterTime;
  } else {
    waterTime_ += Engine::kDefaultDeltaTime * timeScale;
  }

  if (isDirty_) {
    GenerateRoadChunks(view);
    isDirty_ = false;
  }

  if (isGameOver_)
    return;

  // スクロール速度の加速（最大速度まで徐々に上がる）
  // ※ baseScrollSpeed_ が 0 以下の場合は停止状態（リスタートカウントダウン中等）なので加速せず 0 を維持
  if (baseScrollSpeed_ > 0.0f) {
    if (scrollSpeed_ < maxScrollSpeed_) {
      scrollSpeed_ += scrollAcceleration_ * timeScale;
      if (scrollSpeed_ > maxScrollSpeed_) {
        scrollSpeed_ = maxScrollSpeed_;
      }
    }
  } else {
    scrollSpeed_ = 0.0f;
  }

  float currentScroll = scrollSpeed_ * timeScale;

  // 道路チャンクのスクロール
  for (int i = 0; i < kChunkCount_; i++) {
    for (size_t laneIdx = 0; laneIdx < roadTransforms_[i].size(); laneIdx++) {
      roadTransforms_[i][laneIdx].translate.z -= currentScroll;
    }
  }

  // 最も奥にあるチャンクのZ座標を探す
  float maxZ = -999999.0f;
  for (int j = 0; j < kChunkCount_; j++) {
    if (!roadTransforms_[j].empty()) {
      if (roadTransforms_[j][0].translate.z > maxZ) {
        maxZ = roadTransforms_[j][0].translate.z;
      }
    }
  }

  // チャンクが手前の最端（カメラ背後）を通り過ぎたら、一番奥に再配置
  float recycleThreshold =
      -static_cast<float>(kBackwardChunks_ + 1) * chunkLength_;
  for (int i = 0; i < kChunkCount_; i++) {
    if (!roadTransforms_[i].empty() &&
        roadTransforms_[i][0].translate.z < recycleThreshold) {
      float newChunkZ = maxZ + chunkLength_;
      RebuildChunkRow(i, targetLaneCount_, newChunkZ, view);
      maxZ = newChunkZ;
    }

    for (size_t laneIdx = 0; laneIdx < roadChunks_[i].size(); laneIdx++) {
      roadChunks_[i][laneIdx]->SetTransform(roadTransforms_[i][laneIdx]);
    }
  }

  // 海に浮かぶ流氷の更新（道路が停止中・カウントダウン中も海の潮の流れによって自然に流れる）
  float driftIceScroll = currentScroll;
  if ((baseScrollSpeed_ <= 0.0f || driftIceScroll < kDefaultDriftIceFlowSpeed * timeScale) &&
      isDriftIceFlowingWhenStopped_) {
    driftIceScroll = kDefaultDriftIceFlowSpeed * timeScale;
  }
  UpdateDriftIce(view, driftIceScroll, timeScale);

  // アイテムのクールタイム減算（実時間・秒単位）
  float dt = Engine::kDefaultDeltaTime * timeScale;
  for (auto &pair : itemCoolDowns_) {
    // スポーン停止中（ボス戦中やカメラ遷移中）はボスアイテムのクールタイムを減算しない
    if (pair.first == Obstacle::Type::BossItem && isSpawningPaused_) {
      continue;
    }
    if (pair.second.currentTimer > 0.0f) {
      pair.second.currentTimer -= dt;
      if (pair.second.currentTimer < 0.0f) {
        pair.second.currentTimer = 0.0f;
      }
    }
  }

  // 障害物・アイテムの定期生成
  distanceSinceLastSpawn_ += currentScroll;
  distanceSinceLastCameraItem_ += currentScroll;

  while (distanceSinceLastSpawn_ >= obstacleInterval_) {
    // 奥の固定位置(チャンクの向こう側)に生成
    if (!isSpawningPaused_) {
      SpawnObstacles(spawnObstacleZ_);
    }
    distanceSinceLastSpawn_ -= obstacleInterval_;

    // 次回生成までの間隔を移動速度＋猶予フレームに基づいて動的に再計算
    CalculateNextObstacleInterval();
  }

  // 障害物の更新
  for (int i = 0; i < kMaxObstacles_; i++) {
    obstacles_[i]->StageUpdate(view, currentScroll);
  }
}

void StageSettings::EditorUpdate(Matrix4x4 view, float waterTime) {
  if (waterTime >= 0.0f) {
    waterTime_ = waterTime;
  } else {
    waterTime_ += Engine::kDefaultDeltaTime;
  }

  if (isDirty_) {
    GenerateRoadChunks(view);
    isDirty_ = false;
  }

  // 海に浮かぶ流氷の更新（スクロールなしで揺れのみ更新）
  UpdateDriftIce(view, 0.0f, 1.0f);

  // Editor中はスクロールさせないため、スピードを0として更新（WVPのみ更新させる）
  for (int i = 0; i < kMaxObstacles_; i++) {
    obstacles_[i]->StageUpdate(view, 0.0f);
  }
}

void StageSettings::Draw(class Draw &draw) {
#ifdef _USE_IMGUI
  if (showSpawnGizmo_) {
    LineRenderer *lineRenderer = draw.GetLineRenderer();
    if (lineRenderer) {
      float z = spawnObstacleZ_;
      int currentMinLane = GetMinLaneIndexAtZ(z);
      int currentMaxLane = GetMaxLaneIndexAtZ(z);
      float currentLaneWidth = GetEffectiveLaneWidthAtZ(z);

      float xLeft = (static_cast<float>(currentMinLane) - 0.5f) * currentLaneWidth;
      float xRight = (static_cast<float>(currentMaxLane) + 0.5f) * currentLaneWidth;
      float yGround = 0.05f;
      float yTop = 5.5f;

      Vector4 gateColor = {0.0f, 0.9f, 1.0f, 1.0f};      // シアン：スポーンゲート
      Vector4 subGateColor = {0.0f, 0.5f, 0.8f, 0.6f};

      // 外枠ゲート
      lineRenderer->AddLine({xLeft, yGround, z}, {xRight, yGround, z}, gateColor);
      lineRenderer->AddLine({xLeft, yTop, z}, {xRight, yTop, z}, gateColor);
      lineRenderer->AddLine({xLeft, yGround, z}, {xLeft, yTop, z}, gateColor);
      lineRenderer->AddLine({xRight, yGround, z}, {xRight, yTop, z}, gateColor);

      // 各レーンの中心線と高さ指標
      for (int lane = currentMinLane; lane <= currentMaxLane; lane++) {
        float x = static_cast<float>(lane) * currentLaneWidth;

        // レーン中心の縦線
        lineRenderer->AddLine({x, yGround, z}, {x, yTop, z}, subGateColor);

        // Low (木 / ジャンプ) 高さ: 緑
        lineRenderer->AddLine({x - 0.45f, kObstacleSpawnYLow, z},
                              {x + 0.45f, kObstacleSpawnYLow, z},
                              {0.2f, 1.0f, 0.3f, 1.0f});

        // High (アーチ / スライド) 高さ: 黄
        lineRenderer->AddLine({x - 0.45f, kObstacleSpawnYHigh, z},
                              {x + 0.45f, kObstacleSpawnYHigh, z},
                              {1.0f, 0.9f, 0.2f, 1.0f});

        // Wall (氷壁) 高さ: 赤橙
        lineRenderer->AddLine({x - 0.45f, kObstacleSpawnYWall, z},
                              {x + 0.45f, kObstacleSpawnYWall, z},
                              {1.0f, 0.4f, 0.1f, 1.0f});
      }

      // アイテムスポーンライン（オフセット手前）
      float itemZ = z + kItemSpawnZOffset;
      Vector4 itemColor = {1.0f, 0.84f, 0.0f, 0.8f}; // ゴールド
      lineRenderer->AddLine({xLeft, yGround, itemZ}, {xRight, yGround, itemZ}, itemColor);
      lineRenderer->AddLine({xLeft, kItemSpawnY, itemZ}, {xRight, kItemSpawnY, itemZ}, itemColor);
    }
  }
#endif
}

void StageSettings::SpawnObstacles(float z) {
  int currentLaneCount = GetLaneCountAtZ(z);
  int currentMinLane = GetMinLaneIndexAtZ(z);
  float currentLaneWidth = GetEffectiveLaneWidthAtZ(z);

  // === 狭まる区間の専用処理 ===
  if (isNarrowingSection_ && currentLaneCount == 3) {
    // 左右レーンに GuideFloor を配置する
    obstacles_[nextObstacleIndex_]->SetType(Obstacle::Type::GuideFloor);
    obstacles_[nextObstacleIndex_]->Spawn(-currentLaneWidth, kGuideFloorSpawnY, z);
    nextObstacleIndex_ = (nextObstacleIndex_ + 1) % kMaxObstacles_;

    obstacles_[nextObstacleIndex_]->SetType(Obstacle::Type::GuideFloor);
    obstacles_[nextObstacleIndex_]->Spawn(currentLaneWidth, kGuideFloorSpawnY, z);
    nextObstacleIndex_ = (nextObstacleIndex_ + 1) % kMaxObstacles_;

    consecutiveNoSpawnCount_ =
        0; // 障害物が配置されたため連続空ウェーブをリセット
    return;
  }
  // =============================

  // レーンの状態を決定 (0: None, 1: Low, 2: High, 3: Wall, 4: Bonus)
  std::vector<int> laneSpawns(currentLaneCount, 0);

  if (currentPattern_ == DeathDemoPattern::AllWall) {
    // ========================================================
    // パターン1: すべて壁のパターン（全レーンWall）
    // 逃げ場なし！必ず壁に激突して正面クラッシュのゲームオーバー演出へ
    // ========================================================
    for (int i = 0; i < currentLaneCount; i++) {
      laneSpawns[i] = 3; // 全レーンWall
    }
  } else {
    // ========================================================
    // パターン2: ジャンプする障害物だけでるパターン（全レーンLow）
    // 全レーンにLowが並ぶ！ジャンプで飛び越えるか、当たって雪玉転倒のゲームオーバー演出へ
    // ========================================================
    for (int i = 0; i < currentLaneCount; i++) {
      laneSpawns[i] = 1; // 全レーンLow
    }
  }

  // 決定した内容で各レーンに生成
  for (int i = 0; i < currentLaneCount; i++) {
    if (laneSpawns[i] == 0)
      continue; // Noneならスキップ

    int lane = currentMinLane + i; // レーンインデックス（-1, 0, 1）
    float x = static_cast<float>(lane) * currentLaneWidth; // レーンのX座標

    // 障害物の生成
    Obstacle::Type type = Obstacle::Type::Low; // 障害物タイプ
    float y = kObstacleSpawnYLow;              // 障害物のY座標

    if (laneSpawns[i] == 1) {
      type = Obstacle::Type::Low;
      y = kObstacleSpawnYLow;
    } else if (laneSpawns[i] == 2) {
      type = Obstacle::Type::High;
      y = kObstacleSpawnYHigh;
    } else if (laneSpawns[i] == 3) {
      type = Obstacle::Type::Wall;
      y = kObstacleSpawnYWall;
    }

    // 障害物のタイプを設定して配置
    obstacles_[nextObstacleIndex_]->SetType(type);
    obstacles_[nextObstacleIndex_]->Spawn(x, y, z);

    // 次のインデックスへ（リングバッファ的に使う）
    nextObstacleIndex_ = (nextObstacleIndex_ + 1) % kMaxObstacles_;
  }
}

void StageSettings::ClearObstacles(float safeDistance) {
  // 画面内および奥の障害物を全て非アクティブにしてクリアにする
  for (int i = 0; i < kMaxObstacles_; i++) {
    if (obstacles_[i]) {
      obstacles_[i]->Deactivate();
    }
  }
  nextObstacleIndex_ = 0;

  // ゲーム開始直後に安全に走れる助走区間（約15m）を設ける
  distanceSinceLastSpawn_ = -safeDistance;
  distanceSinceLastCameraItem_ = 0.0f;

  CalculateNextObstacleInterval();

  // アイテムのクールタイムを開始時用に設定
  itemCoolDowns_[Obstacle::Type::Bonus].currentTimer = 0.0f;
  itemCoolDowns_[Obstacle::Type::BarrierItem].currentTimer = 0.0f;
  itemCoolDowns_[Obstacle::Type::ClearItem].currentTimer = 0.0f;
  itemCoolDowns_[Obstacle::Type::CameraItem].currentTimer = kCameraInitialCooldown;
  itemCoolDowns_[Obstacle::Type::BossItem].currentTimer = kBossInitialCooldown;
}

void StageSettings::Reset() {
  isGameOver_ = false;
  isNarrowingSection_ = false;
  isSpawningPaused_ = false;

  consecutiveNoSpawnCount_ = 0; // 連続空ウェーブ回数をリセット

  // スクロール速度を初期値にリセット
  scrollSpeed_ = baseScrollSpeed_;

  // 初回生成間隔の計算
  CalculateNextObstacleInterval();

  // 道路チャンクの位置をリセット
  targetLaneCount_ = kDefaultLaneCount;
  laneCount_ = kDefaultLaneCount;
  minLaneIndex_ = kDefaultMinLaneIndex;
  maxLaneIndex_ = kDefaultMaxLaneIndex;
  chunkRowInfos_.resize(kChunkCount_);
  for (int i = 0; i < kChunkCount_; i++) {
    float z = static_cast<float>(i - kBackwardChunks_) * chunkLength_;
    RebuildChunkRow(i, kDefaultLaneCount, z, IdentityMatrix());
  }

  // 海に浮かぶ流氷のリセット
  ResetDriftIce();

  // 障害物を全て非アクティブに
  for (int i = 0; i < kMaxObstacles_; i++) {
    obstacles_[i]->Deactivate();
  }
  nextObstacleIndex_ = 0;
  // リセット後も間を空けずすぐに障害物が出現するように設定
  distanceSinceLastSpawn_ = (std::max)(0.0f, obstacleInterval_ - kResetSpawnLeadDistance);
  distanceSinceLastCameraItem_ = 0.0f;

  // アイテムのクールタイムをリセット（秒単位）
  itemCoolDowns_[Obstacle::Type::Bonus].currentTimer = 0.0f;
  itemCoolDowns_[Obstacle::Type::BarrierItem].currentTimer = 0.0f;
  itemCoolDowns_[Obstacle::Type::ClearItem].currentTimer = 0.0f;
  itemCoolDowns_[Obstacle::Type::CameraItem].currentTimer =
      kCameraInitialCooldown; // 開始後すぐのカメラ変更を防ぐ猶予
  itemCoolDowns_[Obstacle::Type::BossItem].currentTimer =
      kBossInitialCooldown; // 開始後すぐのボス突入を防ぐ猶予
}

float StageSettings::GetItemCoolDownDuration(Obstacle::Type type) const {
  auto it = itemCoolDowns_.find(type);
  if (it != itemCoolDowns_.end()) {
    return it->second.duration;
  }
  return 0.0f;
}

void StageSettings::SetItemCoolDownDuration(Obstacle::Type type,
                                            float duration) {
  itemCoolDowns_[type].duration = (std::max)(0.0f, duration);
}

float StageSettings::GetItemCoolDownTimer(Obstacle::Type type) const {
  auto it = itemCoolDowns_.find(type);
  if (it != itemCoolDowns_.end()) {
    return it->second.currentTimer;
  }
  return 0.0f;
}

void StageSettings::SetItemCoolDownTimer(Obstacle::Type type, float timer) {
  itemCoolDowns_[type].currentTimer = (std::max)(0.0f, timer);
}

bool StageSettings::IsItemCoolDownReady(Obstacle::Type type) const {
  auto it = itemCoolDowns_.find(type);
  if (it != itemCoolDowns_.end()) {
    return it->second.currentTimer <= 0.0f;
  }
  return true;
}

#ifdef _USE_IMGUI

void StageSettings::ImGui() {
  // ステージデバッグパネル
  if (ImGui::CollapsingHeader("ステージ設定デバッグ (Stage Settings)")) {
    int currentLane = GetLaneCount();
    ImGui::Text("現在レーン (プレイヤー位置): %d", currentLane);

    int targetLane = GetTargetLaneCount();
    if (ImGui::SliderInt("目標レーン数", &targetLane, 1, 11)) {
      SetLaneCount(targetLane);
    }

    float laneWidth = GetLaneWidth();
    if (ImGui::SliderFloat("レーン幅", &laneWidth, 1.0f, 10.0f)) {
      SetLaneWidth(laneWidth);
    }

    float oneLaneMult = GetOneLaneWidthMultiplier();
    if (ImGui::SliderFloat("1レーン時幅倍率", &oneLaneMult, 1.0f,
                           3.0f, "%.2f")) {
      SetOneLaneWidthMultiplier(oneLaneMult);
    }

    float baseSpeed = GetBaseScrollSpeed();
    if (ImGui::SliderFloat("基本スクロール速度", &baseSpeed, 0.0f, 1.0f)) {
      SetBaseScrollSpeed(baseSpeed);
    }

    float maxSpeed = GetMaxScrollSpeed();
    if (ImGui::SliderFloat("最高スクロール速度", &maxSpeed, baseSpeed, 2.0f)) {
      SetMaxScrollSpeed(maxSpeed);
    }

    float accel = GetScrollAcceleration();
    if (ImGui::SliderFloat("スクロール加速度", &accel, 0.0f, 0.01f)) {
      SetScrollAcceleration(accel);
    }

    float minDistance = GetMinObstacleDistance();
    float maxDistance = GetMaxObstacleDistance();
    if (ImGui::SliderFloat("最小障害物間隔", &minDistance, 3.0f, 30.0f,
                           "%.1f m")) {
      if (minDistance > maxDistance)
        maxDistance = minDistance;
      SetMinObstacleDistance(minDistance);
      SetMaxObstacleDistance(maxDistance);
    }
    if (ImGui::SliderFloat("最大障害物間隔", &maxDistance, 5.0f, 60.0f,
                           "%.1f m")) {
      if (maxDistance < minDistance)
        minDistance = maxDistance;
      SetMinObstacleDistance(minDistance);
      SetMaxObstacleDistance(maxDistance);
    }

    float baseActionFrames = GetBaseActionFrames();
    if (ImGui::SliderFloat("基本アクション所要フレーム", &baseActionFrames, 0.0f,
                           60.0f, "%.0f f")) {
      SetBaseActionFrames(baseActionFrames);
    }

    float minGrace = GetMinGraceFrames();
    float maxGrace = GetMaxGraceFrames();
    if (ImGui::SliderFloat("最小猶予フレーム", &minGrace, 5.0f, 60.0f,
                           "%.0f f")) {
      if (minGrace > maxGrace)
        maxGrace = minGrace;
      SetMinGraceFrames(minGrace);
      SetMaxGraceFrames(maxGrace);
    }
    if (ImGui::SliderFloat("最大猶予フレーム", &maxGrace, 5.0f, 80.0f,
                           "%.0f f")) {
      if (maxGrace < minGrace)
        minGrace = maxGrace;
      SetMinGraceFrames(minGrace);
      SetMaxGraceFrames(maxGrace);
    }

    float noSpawnChance = GetNoSpawnChance() * 100.0f;
    if (ImGui::SliderFloat("障害物スキップ確率 (%)", &noSpawnChance, 0.0f, 30.0f,
                           "%.1f %%")) {
      SetNoSpawnChance(noSpawnChance / 100.0f);
    }

    float currentSpeed = GetScrollSpeed();
    float curInterval = GetObstacleInterval();
    float estFrames =
        currentSpeed > 0.0f ? (curInterval / currentSpeed) : 0.0f;
    ImGui::Text("次回スポーン間隔: %.1f m (約 %.0f フレーム / %.2f 秒)",
                curInterval, estFrames, estFrames / 60.0f);
    ImGui::Text("回避可能間隔範囲: %.1f m 〜 %.1f m", minDistance, maxDistance);
    if (ImGui::SliderFloat("障害物スポーンZ距離", &spawnObstacleZ_, 20.0f, 160.0f, "%.1f m")) {
    }
    ImGui::Text("デスポーンZ: %.1f m (プール上限: %d)",
                Obstacle::kPlayerObstacleDespawnZ, kMaxObstacles_);

    Vector4 roadColor = GetRoadColor();
    float roadColorArr[4] = {roadColor.x, roadColor.y, roadColor.z,
                             roadColor.w};
    if (ImGui::ColorEdit4("地面の色 (Ground Color)", roadColorArr)) {
      SetRoadColor({roadColorArr[0], roadColorArr[1],
                    roadColorArr[2], roadColorArr[3]});
    }

    bool driftIceEnabled = GetDriftIceEnabled();
    if (ImGui::Checkbox("流氷を有効化 (Drift Ice)", &driftIceEnabled)) {
      SetDriftIceEnabled(driftIceEnabled);
    }
    if (driftIceEnabled) {
      Vector4 driftIceColor = GetDriftIceColor();
      float driftIceColorArr[4] = {driftIceColor.x, driftIceColor.y,
                                   driftIceColor.z, driftIceColor.w};
      if (ImGui::ColorEdit4("流氷の色", driftIceColorArr)) {
        SetDriftIceColor(
            {driftIceColorArr[0], driftIceColorArr[1], driftIceColorArr[2],
             driftIceColorArr[3]});
      }
      float bobbingScale = GetDriftIceBobbingScale();
      if (ImGui::SliderFloat("流氷の揺れ幅",
                             &bobbingScale, 0.0f, 3.0f, "%.2f")) {
        SetDriftIceBobbingScale(bobbingScale);
      }
      float bobbingSpeed = GetDriftIceBobbingSpeedScale();
      if (ImGui::SliderFloat("流氷の揺れ速度",
                             &bobbingSpeed, 0.0f, 3.0f, "%.2f")) {
        SetDriftIceBobbingSpeedScale(bobbingSpeed);
      }
      float sizeScale = GetDriftIceSizeScale();
      if (ImGui::SliderFloat("流氷の大きさ倍率", &sizeScale,
                             0.1f, 3.0f, "%.2f")) {
        SetDriftIceSizeScale(sizeScale);
      }
      float thicknessScale = GetDriftIceThicknessScale();
      if (ImGui::SliderFloat("流氷の厚み (縦の太さ)",
                             &thicknessScale, 0.5f, 6.0f, "%.2f")) {
        SetDriftIceThicknessScale(thicknessScale);
      }
      float heightOffset = GetDriftIceHeightOffset();
      if (ImGui::SliderFloat("流氷の浮遊高度", &heightOffset,
                             -1.0f, 5.0f, "%.2f m")) {
        SetDriftIceHeightOffset(heightOffset);
      }
      float distOffset = GetDriftIceDistanceOffset();
      if (ImGui::SliderFloat("流氷の配置間隔 (地面からの距離)",
                             &distOffset, 0.0f, 20.0f, "%.1f m")) {
        SetDriftIceDistanceOffset(distOffset);
      }
      bool lighting = GetDriftIceLighting();
      if (ImGui::Checkbox("流氷ライティングを有効化", &lighting)) {
        SetDriftIceLighting(lighting);
      }
      float sealSpin = GetBabySealSpinSpeed();
      if (ImGui::SliderFloat("アザラシ回転速度",
                             &sealSpin, 0.0f, 10.0f, "%.1f rad/s")) {
        SetBabySealSpinSpeed(sealSpin);
      }
    }

    ImGui::Separator();
    ImGui::Text("後方道路チャンク数: %d 個 (-%.0f m)",
                GetBackwardChunks(),
                static_cast<float>(GetBackwardChunks()) * GetChunkLength());
    float waterForwardExt = GetWaterForwardExtension();
    if (ImGui::SliderFloat("水面・前方拡張距離 (波奥)",
                           &waterForwardExt, 50.0f, 1200.0f, "%.0f m")) {
      SetWaterForwardExtension(waterForwardExt);
    }
    float waterBackwardExt = GetWaterBackwardExtension();
    if (ImGui::SliderFloat("水面・後方拡張距離 (波手前)",
                           &waterBackwardExt, 50.0f, 800.0f, "%.0f m")) {
      SetWaterBackwardExtension(waterBackwardExt);
    }
    float waterWidthScale = GetWaterWidthScale();
    if (ImGui::SliderFloat("水面・横幅スケール", &waterWidthScale,
                           50.0f, 400.0f, "%.0f m")) {
      SetWaterWidthScale(waterWidthScale);
    }
  }

  // 障害物スポーン地点・状況モニター
  if (ImGui::CollapsingHeader("Obstacle Spawn & Monitor (障害物スポーン地点・状況)",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    float currentSpeed = GetScrollSpeed();

    // 1. スポーン地点の位置・設定
    ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "[ スポーン地点の位置・設定 ]");
    ImGui::SliderFloat("Spawn Distance Z (奥の生成距離)", &spawnObstacleZ_,
                       20.0f, 160.0f, "%.1f m");
    ImGui::Text("プリセット:");
    ImGui::SameLine();
    if (ImGui::SmallButton("50m")) { spawnObstacleZ_ = 50.0f; }
    ImGui::SameLine();
    if (ImGui::SmallButton("80m")) { spawnObstacleZ_ = 80.0f; }
    ImGui::SameLine();
    if (ImGui::SmallButton("105m(標準)")) { spawnObstacleZ_ = kDefaultSpawnObstacleZ; }
    ImGui::SameLine();
    if (ImGui::SmallButton("130m")) { spawnObstacleZ_ = 130.0f; }

    float framesToPlayer = currentSpeed > 0.0f ? (spawnObstacleZ_ / currentSpeed) : 0.0f;
    ImGui::Text("スポーン位置: Z = +%.1f m (プレイヤーから前方 %.1f m)", spawnObstacleZ_, spawnObstacleZ_);
    ImGui::Text("到達予測時間: 約 %.0f フレーム (%.2f 秒) [@ 速度 %.3f]",
                framesToPlayer, framesToPlayer / 60.0f, currentSpeed);

    ImGui::Checkbox("3Dビューにスポーンゲート・マーカーを描画 (Show 3D Spawn Gate)",
                    &showSpawnGizmo_);
    if (showSpawnGizmo_) {
      ImGui::TextDisabled("  シアン: 外枠ゲート | 緑: Low(2.5m) | 黄: High(4.6m) | 赤橙: Wall(3.5m) | 金: アイテム");
    }

    ImGui::Separator();

    // 2. 各レーンのスポーン座標 (X, Y, Z)
    ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "[ 各レーンのスポーン座標 (Z = +%.1f m) ]", spawnObstacleZ_);
    int currentLaneCount = GetLaneCountAtZ(spawnObstacleZ_);
    int currentMinLane = GetMinLaneIndexAtZ(spawnObstacleZ_);
    int currentMaxLane = GetMaxLaneIndexAtZ(spawnObstacleZ_);
    float currentLaneWidth = GetEffectiveLaneWidthAtZ(spawnObstacleZ_);

    ImGui::Text("レーン数: %d | レーン幅: %.2f m (範囲: %d 〜 %d)",
                currentLaneCount, currentLaneWidth, currentMinLane, currentMaxLane);

    if (ImGui::BeginTable("SpawnPointsTable", 6,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                              ImGuiTableFlags_SizingStretchProp)) {
      ImGui::TableSetupColumn("レーン", ImGuiTableColumnFlags_WidthFixed, 80.0f);
      ImGui::TableSetupColumn("World X", ImGuiTableColumnFlags_WidthFixed, 80.0f);
      ImGui::TableSetupColumn("Low(木/飛越)", ImGuiTableColumnFlags_WidthStretch);
      ImGui::TableSetupColumn("High(氷/潜り)", ImGuiTableColumnFlags_WidthStretch);
      ImGui::TableSetupColumn("Wall(氷壁/回避)", ImGuiTableColumnFlags_WidthStretch);
      ImGui::TableSetupColumn("Item(アイテム)", ImGuiTableColumnFlags_WidthStretch);
      ImGui::TableHeadersRow();

      for (int lane = currentMinLane; lane <= currentMaxLane; lane++) {
        ImGui::TableNextRow();
        float x = static_cast<float>(lane) * currentLaneWidth;

        ImGui::TableSetColumnIndex(0);
        if (lane == 0) {
          ImGui::Text("中央 (%+d)", lane);
        } else if (lane < 0) {
          ImGui::Text("左 (%+d)", lane);
        } else {
          ImGui::Text("右 (%+d)", lane);
        }

        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%+.2f m", x);

        ImGui::TableSetColumnIndex(2);
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.3f, 1.0f), "Y=%.2fm", kObstacleSpawnYLow);

        ImGui::TableSetColumnIndex(3);
        ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.2f, 1.0f), "Y=%.2fm", kObstacleSpawnYHigh);

        ImGui::TableSetColumnIndex(4);
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.1f, 1.0f), "Y=%.2fm", kObstacleSpawnYWall);

        ImGui::TableSetColumnIndex(5);
        ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "Y=%.2fm (Z%+.1fm)",
                           kItemSpawnY, kItemSpawnZOffset);
      }
      ImGui::EndTable();
    }

    ImGui::Separator();

    // 3. 次回スポーン進行度・タイミング
    ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "[ 次回スポーン進行度 ]");
    if (isSpawningPaused_) {
      ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "状態: 一時停止中 (PAUSED)");
    } else {
      ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "状態: スポーン稼働中 (ACTIVE)");
    }

    float remainingDist = (std::max)(0.0f, obstacleInterval_ - distanceSinceLastSpawn_);
    float remFrames = currentSpeed > 0.0f ? (remainingDist / currentSpeed) : 0.0f;
    float remSec = remFrames / 60.0f;

    if (distanceSinceLastSpawn_ < 0.0f) {
      ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f),
                         "助走区間中: 残り %.1f m 走行後に通常スポーン開始",
                         -distanceSinceLastSpawn_);
    } else {
      float progress = (obstacleInterval_ > 0.0f)
                           ? (distanceSinceLastSpawn_ / obstacleInterval_)
                           : 0.0f;
      if (progress < 0.0f) progress = 0.0f;
      if (progress > 1.0f) progress = 1.0f;

      char progressBuf[96];
      snprintf(progressBuf, sizeof(progressBuf),
               "次回まで: %.1f m / %.1f m (約 %.0f f / %.2f s)",
               remainingDist, obstacleInterval_, remFrames, remSec);
      ImGui::ProgressBar(progress, ImVec2(-1, 0), progressBuf);
    }

    // 4. スポーン操作・テスト
    ImGui::Text("現在のスポーンパターン: %s",
                currentPattern_ == DeathDemoPattern::AllWall
                    ? "すべて壁 (全レーンWall)"
                    : "ジャンプ障害物のみ (全レーンLow)");
    if (ImGui::RadioButton("すべて壁 (AllWall)", currentPattern_ == DeathDemoPattern::AllWall)) {
      currentPattern_ = DeathDemoPattern::AllWall;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("ジャンプのみ (AllJump)", currentPattern_ == DeathDemoPattern::AllJump)) {
      currentPattern_ = DeathDemoPattern::AllJump;
    }

    if (ImGui::Button("今すぐスポーン実行 (Spawn Now)")) {
      SpawnObstacles(spawnObstacleZ_);
    }
    ImGui::SameLine();
    if (ImGui::Button(isSpawningPaused_ ? "スポーン再開 (Resume)" : "一時停止 (Pause)")) {
      isSpawningPaused_ = !isSpawningPaused_;
    }
    ImGui::SameLine();
    if (ImGui::Button("全障害物を消去 (Clear All)")) {
      ClearObstacles();
    }

    ImGui::Separator();

    // 5. 出現中の障害物一覧
    int activeCount = 0;
    for (int i = 0; i < kMaxObstacles_; i++) {
      if (obstacles_[i] && obstacles_[i]->GetIsActive()) {
        activeCount++;
      }
    }

    char activeHeader[64];
    snprintf(activeHeader, sizeof(activeHeader), "出現中の障害物一覧 (%d / %d 個使用中)###ActiveObsList",
             activeCount, kMaxObstacles_);
    if (ImGui::TreeNode(activeHeader)) {
      if (activeCount == 0) {
        ImGui::TextDisabled("現在アクティブな障害物はありません。");
      } else {
        if (ImGui::BeginTable("ActiveObstaclesTable", 6,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable,
                              ImVec2(0.0f, 180.0f))) {
          ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 40.0f);
          ImGui::TableSetupColumn("種別", ImGuiTableColumnFlags_WidthFixed, 130.0f);
          ImGui::TableSetupColumn("座標 (X, Y, Z)", ImGuiTableColumnFlags_WidthStretch);
          ImGui::TableSetupColumn("プレイヤー距離", ImGuiTableColumnFlags_WidthFixed, 90.0f);
          ImGui::TableSetupColumn("到達予測", ImGuiTableColumnFlags_WidthFixed, 80.0f);
          ImGui::TableSetupColumn("状態", ImGuiTableColumnFlags_WidthFixed, 70.0f);
          ImGui::TableHeadersRow();

          for (int i = 0; i < kMaxObstacles_; i++) {
            Obstacle *obs = obstacles_[i].get();
            if (!obs || !obs->GetIsActive())
              continue;

            ImGui::TableNextRow();
            const Transform &t = obs->GetTransform();
            float distZ = t.translate.z; // プレイヤーは Z ≈ 0
            float etaSec = (distZ > 0.0f && currentSpeed > 0.0f)
                               ? (distZ / currentSpeed / 60.0f)
                               : 0.0f;

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("#%d", i);

            ImGui::TableSetColumnIndex(1);
            const char *typeName = "不明";
            ImVec4 typeColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            switch (obs->GetType()) {
            case Obstacle::Type::Low:
              typeName = "Low (木/ジャンプ)";
              typeColor = ImVec4(0.2f, 1.0f, 0.3f, 1.0f);
              break;
            case Obstacle::Type::High:
              typeName = "High (アーチ/潜り)";
              typeColor = ImVec4(1.0f, 0.9f, 0.2f, 1.0f);
              break;
            case Obstacle::Type::Wall:
              typeName = "Wall (氷壁/レーン)";
              typeColor = ImVec4(1.0f, 0.4f, 0.1f, 1.0f);
              break;
            case Obstacle::Type::Bonus:
              typeName = "Bonus (ボーナス)";
              typeColor = ImVec4(0.9f, 0.4f, 1.0f, 1.0f);
              break;
            case Obstacle::Type::GuideFloor:
              typeName = "Guide (誘導床)";
              typeColor = ImVec4(0.0f, 0.8f, 1.0f, 1.0f);
              break;
            case Obstacle::Type::BarrierItem:
              typeName = "Item: Barrier";
              typeColor = ImVec4(0.2f, 0.8f, 1.0f, 1.0f);
              break;
            case Obstacle::Type::ClearItem:
              typeName = "Item: Bomb";
              typeColor = ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
              break;
            case Obstacle::Type::CameraItem:
              typeName = "Item: 1Lane";
              typeColor = ImVec4(0.3f, 1.0f, 0.6f, 1.0f);
              break;
            case Obstacle::Type::BossItem:
              typeName = "Item: Boss";
              typeColor = ImVec4(1.0f, 0.8f, 0.0f, 1.0f);
              break;
            case Obstacle::Type::BossAttack:
            case Obstacle::Type::BossAttackReflectable:
            case Obstacle::Type::BossAttackLow:
            case Obstacle::Type::BossAttackHigh:
              typeName = "Boss Attack";
              typeColor = ImVec4(1.0f, 0.3f, 0.6f, 1.0f);
              break;
            }
            ImGui::TextColored(typeColor, "%s", typeName);

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("(%+.1f, %+.1f, %+.1f)", t.translate.x, t.translate.y,
                        t.translate.z);

            ImGui::TableSetColumnIndex(3);
            if (distZ > 0.0f) {
              ImGui::Text("%+.1f m", distZ);
            } else {
              ImGui::TextDisabled("通過済み (%+.1fm)", distZ);
            }

            ImGui::TableSetColumnIndex(4);
            if (distZ > 0.0f && currentSpeed > 0.0f) {
              if (etaSec < 1.0f) {
                ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "%.2f s (近接!)",
                                   etaSec);
              } else if (etaSec < 2.5f) {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%.2f s", etaSec);
              } else {
                ImGui::Text("%.2f s", etaSec);
              }
            } else {
              ImGui::TextDisabled("---");
            }

            ImGui::TableSetColumnIndex(5);
            if (obs->GetIsHit()) {
              ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "衝突");
            } else if (distZ < 0.0f) {
              ImGui::TextDisabled("通過");
            } else {
              ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "接近中");
            }
          }
          ImGui::EndTable();
        }
      }
      ImGui::TreePop();
    }
  }

  // アイテムクールタイム設定
  if (ImGui::CollapsingHeader("アイテムクールタイム設定 (Item CoolDowns)",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    float spawnChance = GetItemSpawnChance() * 100.0f;
    if (ImGui::SliderFloat("アイテム生成確率 (%)", &spawnChance, 0.0f,
                           100.0f, "%.1f %%")) {
      SetItemSpawnChance(spawnChance / 100.0f);
    }
    ImGui::Separator();

    struct ItemInfo {
      Obstacle::Type type;
      const char *name;
      const char *desc;
    };
    ItemInfo items[] = {
        {Obstacle::Type::Bonus, "Bonus (ボーナス)", "スコア加算"},
        {Obstacle::Type::BarrierItem, "Barrier (バリア)", "ミスを1回防御"},
        {Obstacle::Type::ClearItem, "Clear (障害物全消去)",
         "画面内の障害物を一掃"},
        {Obstacle::Type::CameraItem, "Camera (視点切替)",
         "1レーンモードへ移行"},
        {Obstacle::Type::BossItem, "Boss (ボス戦突入)", "ボスバトル開始"},
    };

    for (const auto &item : items) {
      ImGui::PushID(static_cast<int>(item.type));
      float duration = GetItemCoolDownDuration(item.type);
      float current = GetItemCoolDownTimer(item.type);

      ImGui::Text("%s - %s", item.name, item.desc);
      if (ImGui::SliderFloat("クールタイム (秒)", &duration, 0.0f, 120.0f,
                             "%.1f s")) {
        SetItemCoolDownDuration(item.type, duration);
      }

      if (current <= 0.0f) {
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f),
                           "  状態: 出現可能 (READY)");
      } else {
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f),
                           "  状態: クールダウン中 (残り %.1f 秒)", current);
      }

      ImGui::SameLine();
      if (ImGui::SmallButton("即時可能にする (リセット)")) {
        SetItemCoolDownTimer(item.type, 0.0f);
      }
      ImGui::SameLine();
      if (ImGui::SmallButton("クールダウン開始")) {
        SetItemCoolDownTimer(item.type, duration);
      }
      ImGui::Separator();
      ImGui::PopID();
    }
  }
}
#else

void StageSettings::ImGui() {}
#endif


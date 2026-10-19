#include "StageSettings.h"
#include "AssetManager.h"
#include "Core/LogHandler.h"
#include "PSO/PipelineState.h"
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
  if (scrollSpeed_ > maxScrollSpeed_ / 2) {
    // PostEffect::SetActivePostEffect(PostEffect::Type::Vignetting);
  } else {
    // PostEffect::SetActivePostEffect(PostEffect::Type::Normal);
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
      SpawnObstacles(kSpawnObstacleZ);
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
  // 描画はGameObjectManagerが一括で行うため、ここでは何もしない
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

  // === 時々障害物が出ない（空ウェーブ/安全区間）判定 ===
  bool isEmptyWave = false; // 障害物を一切生成しない空ウェーブフラグ
  if (consecutiveNoSpawnCount_ < maxConsecutiveNoSpawn_) {
    int roll = std::rand() % 100; // 0〜99の乱数
    int chancePercent =
        static_cast<int>(noSpawnChance_ * 100.0f); // 確率をパーセンテージに変換
    if (roll < chancePercent) {
      isEmptyWave = true;
      consecutiveNoSpawnCount_++; // 連続空ウェーブ回数をカウント
    } else {
      consecutiveNoSpawnCount_ = 0; // 障害物が出るためリセット
    }
  } else {
    consecutiveNoSpawnCount_ = 0; // 連続上限に達したためリセット
  }

  // たまにボーナスまたはアイテムを配置する（各アイテムのクールタイムを考慮）
  // ただし1レーンの場合は出さない
  int bonusLane =
      -1; // ボーナスまたはアイテムを配置するレーン番号（-1は配置なし）
  Obstacle::Type itemType = Obstacle::Type::Bonus; // アイテムの種類
  if (currentLaneCount > 1) {
    int roll = std::rand() % 100;
    int spawnPercent = static_cast<int>(itemSpawnChance_ * 100.0f);
    if (roll < spawnPercent) {
      // クールタイムが終了している（currentTimer <=
      // 0.0f）アイテムを候補として収集
      std::vector<Obstacle::Type> availableItems;
      for (const auto &pair : itemCoolDowns_) {
        // 1レーン時はCameraItemは除外
        if (currentLaneCount == 1 && pair.first == Obstacle::Type::CameraItem) {
          continue;
        }
        if (pair.second.currentTimer <= 0.0f) {
          availableItems.push_back(pair.first);
        }
      }

      // 候補が存在する場合のみアイテムを配置
      if (!availableItems.empty()) {
        bonusLane = std::rand() % currentLaneCount;
        int selectedIndex = std::rand() % availableItems.size();
        itemType = availableItems[selectedIndex];

        // 選ばれたアイテムのクールタイムを再設定（カウントダウン開始）
        itemCoolDowns_[itemType].currentTimer =
            itemCoolDowns_[itemType].duration;
      }
    }
  }

  // 空ウェーブの場合は障害物を配置せず、アイテムのみ生成（または完全な安全区間）
  if (isEmptyWave) {
    if (bonusLane != -1) {
      int lane = currentMinLane + bonusLane; // アイテム配置対象のレーン番号
      float x =
          static_cast<float>(lane) * currentLaneWidth; // レーンのワールドX座標
      obstacles_[nextObstacleIndex_]->SetType(itemType);
      obstacles_[nextObstacleIndex_]->Spawn(x, kItemSpawnY, z + kItemSpawnZOffset);
      nextObstacleIndex_ = (nextObstacleIndex_ + 1) % kMaxObstacles_;
    }
    return; // 障害物は出さずに終了
  }

  // レーンの状態を決定 (0: None, 1: Low, 2: High, 3: Wall, 4: Bonus)
  std::vector<int> laneSpawns(currentLaneCount);
  int wallCount = 0;
  int noneCount = 0;

  for (int i = 0; i < currentLaneCount; i++) {
    laneSpawns[i] = std::rand() % 4; // 0~3
    if (laneSpawns[i] == 3) {
      wallCount++;
    } else if (laneSpawns[i] == 0) {
      noneCount++;
    }
  }

  // すべて「何もない(None)」の場合は、最低1つの障害物を配置する
  if (noneCount == currentLaneCount) {
    int changeIndex = std::rand() % currentLaneCount;
    if (currentLaneCount == 1) {
      // レーンが1つの場合はWall(3)を生成しないようにする(1:Low, 2:High)
      laneSpawns[changeIndex] = 1 + (std::rand() % 2);
    } else {
      laneSpawns[changeIndex] = 1 + (std::rand() % 3); // 1, 2, 3 のどれか
    }
  }

  // noneCountの処理でWallが増えた可能性があるのでwallCountを再計算
  wallCount = 0;
  for (int i = 0; i < currentLaneCount; i++) {
    if (laneSpawns[i] == 3) {
      wallCount++;
    }
  }

  // 全てWallの場合は1つを確実に通れるようにする
  if (wallCount == currentLaneCount) {
    int changeIndex = std::rand() % currentLaneCount;
    if (currentLaneCount == 1) {
      // 1レーンしかなく全てWallの場合は、必ず通れる障害物にする
      laneSpawns[changeIndex] = 1 + (std::rand() % 2); // 1:Low または 2:High
    } else {
      laneSpawns[changeIndex] = 0; // Noneに変更
    }
  }

  // ボーナス配置時に障害物があるレーンを優先する処理（通常ウェーブ時）
  if (bonusLane != -1) {
    std::vector<int> obstacleLanes; // 障害物が存在するレーンのリスト
    for (int i = 0; i < currentLaneCount; i++) {
      if (laneSpawns[i] != 0) {
        obstacleLanes.push_back(i);
      }
    }
    if (!obstacleLanes.empty()) {
      bonusLane = obstacleLanes[std::rand() % obstacleLanes.size()];
    }
  }

  // 決定した内容で各レーンに生成
  for (int i = 0; i < currentLaneCount; i++) {
    if (laneSpawns[i] == 0 && i != bonusLane)
      continue; // None 且つ ボーナスも無いならスキップ

    int lane = currentMinLane + i; // レーンインデックス（-1, 0, 1）
    float x = static_cast<float>(lane) * currentLaneWidth; // レーンのX座標

    // ボーナスまたはアイテムの生成（障害物の手前に配置）
    if (i == bonusLane) {
      obstacles_[nextObstacleIndex_]->SetType(itemType);
      obstacles_[nextObstacleIndex_]->Spawn(x, kItemSpawnY, z + kItemSpawnZOffset);
      nextObstacleIndex_ = (nextObstacleIndex_ + 1) % kMaxObstacles_;
    }

    // 障害物の生成
    if (laneSpawns[i] != 0) {
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

      // 障害物のタイプを変更して配置
      obstacles_[nextObstacleIndex_]->SetType(type);
      obstacles_[nextObstacleIndex_]->Spawn(x, y, z);

      // 次のインデックスへ（リングバッファ的に使う）
      nextObstacleIndex_ = (nextObstacleIndex_ + 1) % kMaxObstacles_;
    }
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
  if (ImGui::CollapsingHeader("Stage Settings Debug")) {
    int currentLane = GetLaneCount();
    ImGui::Text("Current (Player Foot) Lane: %d", currentLane);

    int targetLane = GetTargetLaneCount();
    if (ImGui::SliderInt("Target Lane Count", &targetLane, 1, 11)) {
      SetLaneCount(targetLane);
    }

    float laneWidth = GetLaneWidth();
    if (ImGui::SliderFloat("Lane Width", &laneWidth, 1.0f, 10.0f)) {
      SetLaneWidth(laneWidth);
    }

    float oneLaneMult = GetOneLaneWidthMultiplier();
    if (ImGui::SliderFloat("One Lane Width Multiplier", &oneLaneMult, 1.0f,
                           3.0f, "%.2f")) {
      SetOneLaneWidthMultiplier(oneLaneMult);
    }

    float baseSpeed = GetBaseScrollSpeed();
    if (ImGui::SliderFloat("Base Scroll Speed", &baseSpeed, 0.0f, 1.0f)) {
      SetBaseScrollSpeed(baseSpeed);
    }

    float maxSpeed = GetMaxScrollSpeed();
    if (ImGui::SliderFloat("Max Scroll Speed", &maxSpeed, baseSpeed, 2.0f)) {
      SetMaxScrollSpeed(maxSpeed);
    }

    float accel = GetScrollAcceleration();
    if (ImGui::SliderFloat("Scroll Acceleration", &accel, 0.0f, 0.01f)) {
      SetScrollAcceleration(accel);
    }

    float minDistance = GetMinObstacleDistance();
    float maxDistance = GetMaxObstacleDistance();
    if (ImGui::SliderFloat("Min Obstacle Distance", &minDistance, 3.0f, 30.0f,
                           "%.1f m")) {
      if (minDistance > maxDistance)
        maxDistance = minDistance;
      SetMinObstacleDistance(minDistance);
      SetMaxObstacleDistance(maxDistance);
    }
    if (ImGui::SliderFloat("Max Obstacle Distance", &maxDistance, 5.0f, 60.0f,
                           "%.1f m")) {
      if (maxDistance < minDistance)
        minDistance = maxDistance;
      SetMinObstacleDistance(minDistance);
      SetMaxObstacleDistance(maxDistance);
    }

    float baseActionFrames = GetBaseActionFrames();
    if (ImGui::SliderFloat("Base Action Frames", &baseActionFrames, 0.0f,
                           60.0f, "%.0f f")) {
      SetBaseActionFrames(baseActionFrames);
    }

    float minGrace = GetMinGraceFrames();
    float maxGrace = GetMaxGraceFrames();
    if (ImGui::SliderFloat("Min Grace Frames", &minGrace, 5.0f, 60.0f,
                           "%.0f f")) {
      if (minGrace > maxGrace)
        maxGrace = minGrace;
      SetMinGraceFrames(minGrace);
      SetMaxGraceFrames(maxGrace);
    }
    if (ImGui::SliderFloat("Max Grace Frames", &maxGrace, 5.0f, 80.0f,
                           "%.0f f")) {
      if (maxGrace < minGrace)
        minGrace = maxGrace;
      SetMinGraceFrames(minGrace);
      SetMaxGraceFrames(maxGrace);
    }

    float noSpawnChance = GetNoSpawnChance() * 100.0f;
    if (ImGui::SliderFloat("No Spawn Chance (%)", &noSpawnChance, 0.0f, 30.0f,
                           "%.1f %%")) {
      SetNoSpawnChance(noSpawnChance / 100.0f);
    }

    float currentSpeed = GetScrollSpeed();
    float curInterval = GetObstacleInterval();
    float estFrames =
        currentSpeed > 0.0f ? (curInterval / currentSpeed) : 0.0f;
    ImGui::Text("Next Spawn: %.1f m (approx. %.0f frames / %.2f s)",
                curInterval, estFrames, estFrames / 60.0f);
    ImGui::Text("Dodgeable Range: %.1f m ~ %.1f m", minDistance, maxDistance);
    ImGui::Text("Spawn Z: +%.1f m | Despawn Z: %.1f m (Max Pool: %d)",
                kSpawnObstacleZ, Obstacle::kPlayerObstacleDespawnZ, kMaxObstacles_);

    Vector4 roadColor = GetRoadColor();
    float roadColorArr[4] = {roadColor.x, roadColor.y, roadColor.z,
                             roadColor.w};
    if (ImGui::ColorEdit4("Ground Color", roadColorArr)) {
      SetRoadColor({roadColorArr[0], roadColorArr[1],
                    roadColorArr[2], roadColorArr[3]});
    }

    bool driftIceEnabled = GetDriftIceEnabled();
    if (ImGui::Checkbox("Enable Drift Ice (流氷)", &driftIceEnabled)) {
      SetDriftIceEnabled(driftIceEnabled);
    }
    if (driftIceEnabled) {
      Vector4 driftIceColor = GetDriftIceColor();
      float driftIceColorArr[4] = {driftIceColor.x, driftIceColor.y,
                                   driftIceColor.z, driftIceColor.w};
      if (ImGui::ColorEdit4("Drift Ice Color", driftIceColorArr)) {
        SetDriftIceColor(
            {driftIceColorArr[0], driftIceColorArr[1], driftIceColorArr[2],
             driftIceColorArr[3]});
      }
      float bobbingScale = GetDriftIceBobbingScale();
      if (ImGui::SliderFloat("Drift Ice Bobbing Scale (揺れ幅)",
                             &bobbingScale, 0.0f, 3.0f, "%.2f")) {
        SetDriftIceBobbingScale(bobbingScale);
      }
      float bobbingSpeed = GetDriftIceBobbingSpeedScale();
      if (ImGui::SliderFloat("Drift Ice Bobbing Speed (揺れ速度)",
                             &bobbingSpeed, 0.0f, 3.0f, "%.2f")) {
        SetDriftIceBobbingSpeedScale(bobbingSpeed);
      }
      float sizeScale = GetDriftIceSizeScale();
      if (ImGui::SliderFloat("Drift Ice Size Scale (大きさ倍率)", &sizeScale,
                             0.1f, 3.0f, "%.2f")) {
        SetDriftIceSizeScale(sizeScale);
      }
      float thicknessScale = GetDriftIceThicknessScale();
      if (ImGui::SliderFloat("Drift Ice Thickness (厚み・縦の太さ)",
                             &thicknessScale, 0.5f, 6.0f, "%.2f")) {
        SetDriftIceThicknessScale(thicknessScale);
      }
      float heightOffset = GetDriftIceHeightOffset();
      if (ImGui::SliderFloat("Drift Ice Height (浮遊高さ)", &heightOffset,
                             -1.0f, 5.0f, "%.2f m")) {
        SetDriftIceHeightOffset(heightOffset);
      }
      float distOffset = GetDriftIceDistanceOffset();
      if (ImGui::SliderFloat("Drift Ice Distance (地面からの離れ具合)",
                             &distOffset, 0.0f, 20.0f, "%.1f m")) {
        SetDriftIceDistanceOffset(distOffset);
      }
      bool lighting = GetDriftIceLighting();
      if (ImGui::Checkbox("Drift Ice Lighting (ライティング)", &lighting)) {
        SetDriftIceLighting(lighting);
      }
      float sealSpin = GetBabySealSpinSpeed();
      if (ImGui::SliderFloat("Baby Seal Spin Speed (アザラシ回転速度)",
                             &sealSpin, 0.0f, 10.0f, "%.1f rad/s")) {
        SetBabySealSpinSpeed(sealSpin);
      }
    }

    ImGui::Separator();
    ImGui::Text("Road Backwards: %d chunks (-%.0f m)",
                GetBackwardChunks(),
                static_cast<float>(GetBackwardChunks()) * GetChunkLength());
    float waterForwardExt = GetWaterForwardExtension();
    if (ImGui::SliderFloat("Water Forward Ext (波の奥への拡張)",
                           &waterForwardExt, 50.0f, 1200.0f, "%.0f m")) {
      SetWaterForwardExtension(waterForwardExt);
    }
    float waterBackwardExt = GetWaterBackwardExtension();
    if (ImGui::SliderFloat("Water Backward Ext (波の手前への拡張)",
                           &waterBackwardExt, 50.0f, 800.0f, "%.0f m")) {
      SetWaterBackwardExtension(waterBackwardExt);
    }
    float waterWidthScale = GetWaterWidthScale();
    if (ImGui::SliderFloat("Water Width Scale (波の横幅)", &waterWidthScale,
                           50.0f, 400.0f, "%.0f m")) {
      SetWaterWidthScale(waterWidthScale);
    }
  }

  // アイテムクールタイム設定
  if (ImGui::CollapsingHeader("Item CoolDowns (アイテムクールタイム設定)",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    float spawnChance = GetItemSpawnChance() * 100.0f;
    if (ImGui::SliderFloat("Item Spawn Chance (%)", &spawnChance, 0.0f,
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
      if (ImGui::SliderFloat("CoolTime (s)", &duration, 0.0f, 120.0f,
                             "%.1f s")) {
        SetItemCoolDownDuration(item.type, duration);
      }

      if (current <= 0.0f) {
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f),
                           "  Status: READY (出現可能)");
      } else {
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f),
                           "  Status: COOLDOWN (残り %.1f s)", current);
      }

      ImGui::SameLine();
      if (ImGui::SmallButton("Reset CD (即時可能)")) {
        SetItemCoolDownTimer(item.type, 0.0f);
      }
      ImGui::SameLine();
      if (ImGui::SmallButton("Trigger CD")) {
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

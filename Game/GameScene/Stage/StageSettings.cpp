#include "StageSettings.h"
#include "AssetManager.h"
#include "Core/LogHandler.h"
#include "PSO/PipelineState.h"
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <format>


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
    Obstacle::Type type = static_cast<Obstacle::Type>(std::rand() % 3);
    obstacles_[i]->Initialize(fallenTreeModelData, iceArchwayModelData,
                              iceWallModelData, bonusModelData, iceBomModelData,
                              reflectingAttackModelData, type);

    if (manager)
      manager->AddObject(obstacles_[i]);
  }

  // アイテムクールタイム設定の初期化（秒単位）
  itemCoolDowns_[Obstacle::Type::Bonus] = {5.0f, 0.0f};        // ボーナス: 5秒
  itemCoolDowns_[Obstacle::Type::BarrierItem] = {15.0f, 0.0f}; // バリア: 15秒
  itemCoolDowns_[Obstacle::Type::ClearItem] = {20.0f, 0.0f};   // 全消去: 20秒
  itemCoolDowns_[Obstacle::Type::CameraItem] = {
      30.0f, 10.0f}; // カメラ: 30秒（開始時10秒猶予）
  itemCoolDowns_[Obstacle::Type::BossItem] = {
      45.0f, 20.0f}; // ボス: 45秒（開始時20秒猶予）
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
                 0.05f); // 停止時や低速時の0除算・0距離を防ぐ実効速度
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
      if (roadTransforms_[i][0].translate.z >= 70.0f) {
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
  if (scale <= 0.05f)
    scale = 0.05f;
  float ratio = scale / (std::max)(driftIceSizeScale_, 0.05f);
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
  if (scale <= 0.05f)
    scale = 0.05f;
  float ratio = scale / (std::max)(driftIceThicknessScale_, 0.05f);
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
  waterWidthScale_ = (std::max)(10.0f, scale);
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
    t.scale = {info.effectiveLaneWidth / modelWidth, 50.0f / modelHeight,
               10.1f / modelDepth};
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
      model->SetColor({0.15f, 0.55f, 0.85f, 0.9f});
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
  float bounds[2] = {static_cast<float>(minLaneIndex_) * effectiveLaneWidth -
                         (effectiveLaneWidth / 2.0f),
                     static_cast<float>(maxLaneIndex_) * effectiveLaneWidth +
                         (effectiveLaneWidth / 2.0f)};
  float offsets[2] = {-(waterWidthScale_ * 0.5f), waterWidthScale_ * 0.5f};

  float minRoadZ = -static_cast<float>(kBackwardChunks_) * chunkLength_;
  float maxRoadZ = static_cast<float>(kForwardChunks_) * chunkLength_;

  // 波のZ範囲：手前・奥ともに道路を大きく超えてカバー
  float minSeaZ = minRoadZ - waterBackwardExtension_;
  float maxSeaZ = maxRoadZ + waterForwardExtension_;
  float seaLength = maxSeaZ - minSeaZ;
  float centerSeaZ = (minSeaZ + maxSeaZ) * 0.5f;

  for (int i = 0; i < 2; ++i) {
    if (!sidePlanes_[i])
      continue;
    Transform t;
    // plane.objは2x2 (-1〜+1) なので、Yスケール * 2 = seaLength
    t.scale = {waterWidthScale_, seaLength * 0.5f, 1.0f};
    t.rotate = {-1.570796f, 0.0f, 0.0f};
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
    float c = std::sqrt(9.8f / k) * w.speed;
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
  bool isSeal = (std::rand() % 100 < 15);
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

  float targetWidth = 1.5f;
  float targetDepth = 1.5f;
  float targetHeight = 1.0f;
  float floatingHeight = 0.25f;

  if (ice.type == DriftIce::Type::BabySeal) {
    // 流氷に乗ったアザラシのサイズ感
    targetWidth = 2.0f + static_cast<float>(std::rand() % 12) / 10.0f;      // 2.0m 〜 3.1m
    targetDepth = 2.0f + static_cast<float>(std::rand() % 14) / 10.0f;      // 2.0m 〜 3.3m
    targetHeight = 0.90f + static_cast<float>(std::rand() % 41) / 100.0f;   // 0.9m 〜 1.3m
    floatingHeight = 0.25f + static_cast<float>(std::rand() % 15) / 100.0f; // 25〜39cm水上に出す
  } else {
    // 通常の流氷のサイズ設計 (極小 45%, 小〜中 35%, 中〜大型 20%)
    int sizeCategory = std::rand() % 100;
    if (sizeCategory < 45) {
      // 極小〜小型流氷塊: 幅1.2〜2.2m, 奥行1.2〜2.4m, 厚み0.6〜0.9m
      targetWidth = 1.2f + static_cast<float>(std::rand() % 11) / 10.0f;
      targetDepth = 1.2f + static_cast<float>(std::rand() % 13) / 10.0f;
      targetHeight = 0.60f + static_cast<float>(std::rand() % 31) / 100.0f; // 0.6m 〜 0.9m
      floatingHeight = 0.20f + static_cast<float>(std::rand() % 11) / 100.0f; // 20〜30cm水上に出す
    } else if (sizeCategory < 80) {
      // 中型流氷塊: 幅2.2〜3.8m, 奥行2.2〜4.2m, 厚み0.9〜1.3m
      targetWidth = 2.2f + static_cast<float>(std::rand() % 17) / 10.0f;
      targetDepth = 2.2f + static_cast<float>(std::rand() % 21) / 10.0f;
      targetHeight = 0.90f + static_cast<float>(std::rand() % 41) / 100.0f; // 0.9m 〜 1.3m
      floatingHeight = 0.25f + static_cast<float>(std::rand() % 16) / 100.0f; // 25〜40cm水上に出す
    } else {
      // 大型流氷塊: 幅3.5〜6.5m, 奥行3.5〜7.0m, 厚み1.2〜1.8m
      targetWidth = 3.5f + static_cast<float>(std::rand() % 31) / 10.0f;
      targetDepth = 3.5f + static_cast<float>(std::rand() % 36) / 10.0f;
      targetHeight = 1.20f + static_cast<float>(std::rand() % 61) / 100.0f; // 1.2m 〜 1.8m
      floatingHeight = 0.30f + static_cast<float>(std::rand() % 21) / 100.0f; // 30〜50cm水上に出す
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
  ice.driftAmount = 0.2f + static_cast<float>(std::rand() % 60) / 100.0f; // 0.2m 〜 0.8m
  ice.driftSpeed = 0.3f + static_cast<float>(std::rand() % 60) / 100.0f;  // 0.3 〜 0.9 rad/s
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

  // 道路端 + 流氷の半径(halfWidth) + 最大横揺れ幅 + 安全マージン(1.5m)
  float halfWidth = targetWidth * 0.5f;
  float minClearance = roadBoundary + halfWidth + ice.driftAmount + 1.5f;

  // 最小クリアランスから外洋側（+30m）へ滑らかに分布
  float dist = minClearance + static_cast<float>(std::rand() % 300) / 10.0f;
  ice.posX = sideSign * dist;

  // 【流れる向きなどのバラバラ化】
  // 初期の向き（回転角度Y）：0〜360度ランダム
  ice.rotY = static_cast<float>(std::rand() % 628) / 100.0f;
  if (ice.type == DriftIce::Type::BabySeal) {
    // アザラシは水面をくるくるとスピン（自転）しながら流れる
    float dir = (std::rand() % 2 == 0) ? 1.0f : -1.0f;
    float speedVariation = 0.85f + static_cast<float>(std::rand() % 31) / 100.0f; // 0.85〜1.15倍
    ice.rotSpeed = dir * babySealSpinSpeed_ * speedVariation;
  } else {
    // 通常の流氷も右回り・左回り、回転速度にバリエーションを持たせて向きの変化を豊かに
    float dir = (std::rand() % 2 == 0) ? 1.0f : -1.0f;
    ice.rotSpeed = dir * (0.04f + static_cast<float>(std::rand() % 120) / 1000.0f); // ±0.04 〜 ±0.16 rad/s (毎秒2.3〜9.2度)
  }

  // 上下の揺れ（ボビング）
  ice.bobbingPhase = static_cast<float>(std::rand() % 628) / 100.0f;
  ice.bobbingSpeed = 0.5f + static_cast<float>(std::rand() % 80) / 100.0f; // 0.5〜1.3 rad/s
  ice.bobbingAmount = 0.05f + static_cast<float>(std::rand() % 70) / 1000.0f; // 0.05〜0.12m
  ice.rollPitchMultiplier = 0.012f + static_cast<float>(std::rand() % 18) / 1000.0f;

  // 【流れるスピードは全部同じ（1.0倍に統一）】
  ice.speedMultiplier = 1.0f;
}

void StageSettings::UpdateDriftIce(Matrix4x4 view, float currentScroll, float timeScale) {
  if (!driftIceEnabled_) return;

  float dt = (1.0f / 60.0f) * timeScale;
  waterTime_ += dt;
  float backwardThreshold = -static_cast<float>(kBackwardChunks_ + 1) * chunkLength_ - 15.0f;

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
    if (ice.rotY > 6.283185f) {
      ice.rotY -= 6.283185f;
    } else if (ice.rotY < 0.0f) {
      ice.rotY += 6.283185f;
    }

    // 斜め方向への緩やかな潮流移動
    ice.posX += ice.driftVelocityX * dt;

    // 波による揺れ（上下・傾き）の更新
    ice.bobbingPhase += ice.bobbingSpeed * driftIceBobbingSpeedScale_ * dt;
    if (ice.bobbingPhase > 6.283185f * 10.0f) {
      ice.bobbingPhase -= 6.283185f * 10.0f;
    }

    float currentBobbing = sinf(ice.bobbingPhase) * ice.bobbingAmount * driftIceBobbingScale_;
    float pitch = sinf(ice.bobbingPhase * 0.7f) * ice.rollPitchMultiplier * driftIceBobbingScale_;
    float roll = cosf(ice.bobbingPhase * 0.8f) * ice.rollPitchMultiplier * driftIceBobbingScale_;

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
    constexpr float kRoadSafeMargin = 1.0f; // 床端からの確実な安全余白 (1.0m)
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

    // 流氷の中心および四隅における波の高さをサンプリングし、最大波高を取得
    float maxWaterY = CalculateWaterHeight(posXWithOffset, ice.posZ, waterTime_);
    maxWaterY = (std::max)(maxWaterY, CalculateWaterHeight(posXWithOffset - halfWidth, ice.posZ - halfDepth, waterTime_));
    maxWaterY = (std::max)(maxWaterY, CalculateWaterHeight(posXWithOffset + halfWidth, ice.posZ - halfDepth, waterTime_));
    maxWaterY = (std::max)(maxWaterY, CalculateWaterHeight(posXWithOffset - halfWidth, ice.posZ + halfDepth, waterTime_));
    maxWaterY = (std::max)(maxWaterY, CalculateWaterHeight(posXWithOffset + halfWidth, ice.posZ + halfDepth, waterTime_));

    // 目標上面高さ:
    // 波の最高点 + 浮遊高さ + 上下ボビング揺れ + 高さオフセット
    float targetTopY = maxWaterY + ice.floatingHeight + driftIceHeightOffset_ + currentBobbing;

    // 【絶対に沈まない下限ガード】
    // 最低でも波の最高地点より 0.15m (15cm) 以上は上面が水面から常に出ていることを保証
    float minAllowedTopY = maxWaterY + 0.15f;
    if (targetTopY < minAllowedTopY) {
      targetTopY = minAllowedTopY;
    }

    // 中心Y座標を計算（分厚い氷塊の大部分が水面下に潜り、上面が頭を出す）
    float centerY = targetTopY - halfHeight;

    Transform t;
    t.scale = ice.scale;
    t.rotate = {pitch, ice.rotY, roll};
    t.translate = {posXWithOffset, centerY, ice.posZ};

    ice.renderObj->SetTransform(t);
    ice.renderObj->Update(view, 0.0f);
  }
}

void StageSettings::ResetDriftIce() {
  waterTime_ = 0.0f;
  for (int i = 0; i < static_cast<int>(driftIces_.size()); ++i) {
    SetupSingleDriftIce(driftIces_[i], false, i);
  }
}

void StageSettings::Update(Matrix4x4 view, float timeScale) {
  if (isDirty_) {
    GenerateRoadChunks(view);
    isDirty_ = false;
  }

  if (isGameOver_)
    return;

  // スクロール速度の加速（最大速度まで徐々に上がる）
  if (scrollSpeed_ < maxScrollSpeed_) {
    scrollSpeed_ += scrollAcceleration_ * timeScale;
    if (scrollSpeed_ > maxScrollSpeed_) {
      scrollSpeed_ = maxScrollSpeed_;
    }
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

  // 海に浮かぶ流氷の更新
  UpdateDriftIce(view, currentScroll, timeScale);

  // アイテムのクールタイム減算（実時間・秒単位）
  float dt = (1.0f / 60.0f) * timeScale;
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
      SpawnObstacles(45.0f);
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

void StageSettings::EditorUpdate(Matrix4x4 view) {
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
    obstacles_[nextObstacleIndex_]->Spawn(-currentLaneWidth, 2.0f, z);
    nextObstacleIndex_ = (nextObstacleIndex_ + 1) % kMaxObstacles_;

    obstacles_[nextObstacleIndex_]->SetType(Obstacle::Type::GuideFloor);
    obstacles_[nextObstacleIndex_]->Spawn(currentLaneWidth, 2.0f, z);
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
      obstacles_[nextObstacleIndex_]->Spawn(x, 2.5f, z - 5.0f);
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
      obstacles_[nextObstacleIndex_]->Spawn(x, 2.5f, z - 5.0f);
      nextObstacleIndex_ = (nextObstacleIndex_ + 1) % kMaxObstacles_;
    }

    // 障害物の生成
    if (laneSpawns[i] != 0) {
      Obstacle::Type type = Obstacle::Type::Low; // 障害物タイプ
      float y = 2.5f;                            // 障害物のY座標

      if (laneSpawns[i] == 1) {
        type = Obstacle::Type::Low;
        y = 2.5f;
      } else if (laneSpawns[i] == 2) {
        type = Obstacle::Type::High;
        y = 4.6f;
      } else if (laneSpawns[i] == 3) {
        type = Obstacle::Type::Wall;
        y = 3.5f;
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
  itemCoolDowns_[Obstacle::Type::CameraItem].currentTimer = 10.0f;
  itemCoolDowns_[Obstacle::Type::BossItem].currentTimer = 20.0f;
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
  targetLaneCount_ = 3;
  laneCount_ = 3;
  minLaneIndex_ = -1;
  maxLaneIndex_ = 1;
  chunkRowInfos_.resize(kChunkCount_);
  for (int i = 0; i < kChunkCount_; i++) {
    float z = static_cast<float>(i - kBackwardChunks_) * chunkLength_;
    RebuildChunkRow(i, 3, z, IdentityMatrix());
  }

  // 海に浮かぶ流氷のリセット
  ResetDriftIce();

  // 障害物を全て非アクティブに
  for (int i = 0; i < kMaxObstacles_; i++) {
    obstacles_[i]->Deactivate();
  }
  nextObstacleIndex_ = 0;
  // リセット後も間を空けずすぐに障害物が出現するように設定
  distanceSinceLastSpawn_ = (std::max)(0.0f, obstacleInterval_ - 3.0f);
  distanceSinceLastCameraItem_ = 0.0f;

  // アイテムのクールタイムをリセット（秒単位）
  itemCoolDowns_[Obstacle::Type::Bonus].currentTimer = 0.0f;
  itemCoolDowns_[Obstacle::Type::BarrierItem].currentTimer = 0.0f;
  itemCoolDowns_[Obstacle::Type::ClearItem].currentTimer = 0.0f;
  itemCoolDowns_[Obstacle::Type::CameraItem].currentTimer =
      10.0f; // 開始後すぐのカメラ変更を防ぐ猶予
  itemCoolDowns_[Obstacle::Type::BossItem].currentTimer =
      20.0f; // 開始後すぐのボス突入を防ぐ猶予
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

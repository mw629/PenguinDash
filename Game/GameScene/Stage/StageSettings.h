#pragma once
#include "GameObjectManager.h"
#include "RenderObject.h"
#include <Engine.h>
#include <Entity/Obstacle.h>
#include <map>
#include <memory>
#include <vector>

/// <summary>
/// アイテムごとのクールタイム設定（秒単位）
/// </summary>
struct ItemCoolDownSetting {
  static constexpr float kDefaultDuration = 10.0f;
  float duration = kDefaultDuration;    // クールタイム（秒）
  float currentTimer = 0.0f; // 現在の残りクールタイム（秒、0以下で出現可能）
};

/// <summary>
/// ステージの無限スクロールと障害物の管理を行うクラス
/// 道路チャンクを手前に移動させ、画面外に出たチャンクを奥に再配置する
/// </summary>
class StageSettings {
private:
  static constexpr int kDefaultLaneCount = 3;
  static constexpr int kDefaultMinLaneIndex = -1;
  static constexpr int kDefaultMaxLaneIndex = 1;
  static constexpr float kDefaultLaneWidth = 2.0f;
  static constexpr float kDefaultOneLaneWidthMultiplier = 1.0f;
  static constexpr float kDefaultBaseScrollSpeed = 0.2f;
  static constexpr float kDefaultMaxScrollSpeed = 1.0f;
  static constexpr float kDefaultScrollAcceleration = 0.0001f;
  static constexpr float kDefaultChunkLength = 10.0f;

  static constexpr float kBonusCooldownDuration = 5.0f;
  static constexpr float kBarrierCooldownDuration = 15.0f;
  static constexpr float kClearCooldownDuration = 20.0f;
  static constexpr float kCameraCooldownDuration = 30.0f;
  static constexpr float kCameraInitialCooldown = 10.0f;
  static constexpr float kBossCooldownDuration = 45.0f;
  static constexpr float kBossInitialCooldown = 20.0f;

  static constexpr float kDefaultWaterForwardExtension = 500.0f;
  static constexpr float kDefaultWaterBackwardExtension = 200.0f;
  static constexpr float kDefaultWaterWidthScale = 120.0f;
  static constexpr Vector4 kDefaultWaterColor = {0.15f, 0.55f, 0.85f, 0.9f};
  static constexpr float kPlaneRotationX = -1.5707963f;
  static constexpr float kHalfFactor = 0.5f;

  static constexpr float kDefaultBabySealSpinSpeed = 3.5f;
  static constexpr float kDefaultObstacleInterval = 7.0f;
  static constexpr float kDefaultBaseActionFrames = 30.0f;
  static constexpr float kDefaultMinGraceFrames = 5.0f;
  static constexpr float kDefaultMaxGraceFrames = 15.0f;
  static constexpr float kDefaultMinObstacleDistance = 6.0f;
  static constexpr float kDefaultMaxObstacleDistance = 45.0f;
  static constexpr float kDefaultNoSpawnChance = 0.05f;
  static constexpr int kDefaultMaxConsecutiveNoSpawn = 1;
  static constexpr float kDefaultItemSpawnChance = 0.15f;
  static constexpr float kDefaultCameraItemInterval = 500.0f;
  static constexpr float kLaneChangeHorizonZ = 70.0f;
  static constexpr float kResetSpawnLeadDistance = 3.0f;
  static constexpr float kMinEffectiveSpeed = 0.05f;
  static constexpr int kInitialObstacleTypeCount = 3;

  static constexpr float kDefaultClearObstacleSafeDistance = 15.0f;
  static constexpr float kDriftIceRespawnSafeDistance = 15.0f;
  static constexpr int kBabySealSpawnChancePercent = 15;
  static constexpr float kTwoPi = 6.283185f;
  static constexpr float kRoadChunkHeight = 50.0f;
  static constexpr float kRoadChunkDepthScale = 10.1f;
  static constexpr float kObstacleSpawnYLow = 2.5f;
  static constexpr float kObstacleSpawnYHigh = 4.6f;
  static constexpr float kObstacleSpawnYWall = 3.5f;
  static constexpr float kGuideFloorSpawnY = 2.0f;
  static constexpr float kItemSpawnY = 2.5f;
  static constexpr float kItemSpawnZOffset = -5.0f;
  static constexpr float kSpawnObstacleZ = 45.0f;

  static constexpr Vector4 kDefaultRoadColor = {1.0f, 1.0f, 1.0f, 1.0f};
  static constexpr Vector4 kDefaultDriftIceColor = {1.0f, 1.0f, 1.0f, 1.0f};
  static constexpr float kDefaultScale = 1.0f;
  static constexpr float kDefaultTimeScale = 1.0f;
  static constexpr float kAutoWaterTime = -1.0f;
  static constexpr float kMinDriftIceScale = 0.05f;
  static constexpr float kMinWaterWidthScale = 10.0f;
  static constexpr float kDriftIceClearanceMargin = 1.5f;
  static constexpr float kRoadSafeMargin = 1.0f;
  static constexpr float kMaxLocalWaveTilt = 0.22f;
  static constexpr float kMinSafeAboveWater = 0.40f;
  static constexpr float kSafeAboveWaterHeightRatio = 0.6f;

  // レーンの管理
  int laneCount_ = kDefaultLaneCount;
  int minLaneIndex_ = kDefaultMinLaneIndex;
  int maxLaneIndex_ = kDefaultMaxLaneIndex;
  float laneWidth_ = kDefaultLaneWidth;              // 1レーンの床の幅（元の2.0fの2倍）
  float oneLaneWidthMultiplier_ = kDefaultOneLaneWidthMultiplier; // 1レーン時の追加倍率

  // スクロール速度
  float scrollSpeed_ = kDefaultBaseScrollSpeed;           // 現在のスクロール速度
  float baseScrollSpeed_ = kDefaultBaseScrollSpeed;       // 初期スクロール速度
  float maxScrollSpeed_ = kDefaultMaxScrollSpeed;        // 最大スクロール速度
  float scrollAcceleration_ = kDefaultScrollAcceleration; // 毎フレームの加速量

  // 道路チャンク
  static constexpr int kBackwardChunks_ = 20; // プレイヤーより手前（カメラ側・背後）のチャンク数（従来の6から20に拡張）
  static constexpr int kForwardChunks_ = 14; // プレイヤーより奥（進行方向）のチャンク数
  static constexpr int kChunkCount_ = kBackwardChunks_ + kForwardChunks_; // チャンクの総数 (34)
  float chunkLength_ = kDefaultChunkLength; // 1チャンクの奥行き（Z軸方向のサイズ）

public:
  struct ChunkRowInfo {
    int laneCount = kDefaultLaneCount;
    int minLaneIndex = kDefaultMinLaneIndex;
    int maxLaneIndex = kDefaultMaxLaneIndex;
    float effectiveLaneWidth = kDefaultLaneWidth;
  };

private:
  int targetLaneCount_ = kDefaultLaneCount;
  std::vector<ChunkRowInfo> chunkRowInfos_;

  std::vector<std::vector<std::shared_ptr<RenderObject>>> roadChunks_;
  std::vector<std::vector<Transform>> roadTransforms_;

  // サイドプレーン用
  std::shared_ptr<RenderObject> sidePlanes_[2];
  ModelData planeModelData_;
  float waterForwardExtension_ = kDefaultWaterForwardExtension;  // 道路奥端からさらに奥への波の拡張距離（m）
  float waterBackwardExtension_ = kDefaultWaterBackwardExtension; // 道路手前端からさらに手前への波の拡張距離（m）
  float waterWidthScale_ = kDefaultWaterWidthScale;        // 波の横幅スケール（片側、m）

  // 海に浮かべる流氷・アザラシ（Drift Ice / Baby Seal）
public:
  struct DriftIce {
    static constexpr float kDefaultBaseY = 0.3f;
    static constexpr float kDefaultFloatingHeight = 0.25f;
    static constexpr float kDefaultBobbingSpeed = 1.0f;
    static constexpr float kDefaultBobbingAmount = 0.06f;
    static constexpr float kDefaultSpeedMultiplier = 1.0f;
    static constexpr float kDefaultRollPitchMultiplier = 1.0f;
    static constexpr float kDefaultTargetWidth = 1.5f;
    static constexpr float kDefaultTargetDepth = 1.5f;
    static constexpr float kDefaultTargetHeight = 1.0f;

    enum class Type {
      DriftIce,
      BabySeal
    };
    Type type = Type::DriftIce;

    std::shared_ptr<RenderObject> renderObj;
    std::shared_ptr<Model> iceModel;
    std::shared_ptr<Model> sealModel;

    float posX = 0.0f;
    float posZ = 0.0f;
    float baseY = kDefaultBaseY;
    Vector3 scale = {1.0f, 1.0f, 1.0f};
    float floatingHeight = kDefaultFloatingHeight;
    float rotY = 0.0f;
    float rotSpeed = 0.0f;             // ゆっくりとした自転ドリフト (rad/s)
    float driftAmount = 0.0f;          // 潮流・波による横方向の揺らぎ振幅 (m)
    float driftSpeed = 0.0f;           // 横方向の揺らぎ周期速度 (rad/s)
    float driftPhase = 0.0f;           // 横方向の揺らぎ初期位相
    float driftVelocityX = 0.0f;       // 斜め方向への緩やかな潮流移動速度 (m/s)
    float bobbingPhase = 0.0f;
    float bobbingSpeed = kDefaultBobbingSpeed;
    float bobbingAmount = kDefaultBobbingAmount;
    float speedMultiplier = kDefaultSpeedMultiplier;
    float rollPitchMultiplier = kDefaultRollPitchMultiplier;
  };

private:
  static constexpr float kGravityAcceleration = 9.8f;

  // アザラシ流氷サイズ・浮力
  static constexpr float kSealIceBaseWidth = 2.0f;
  static constexpr float kSealIceBaseDepth = 2.0f;
  static constexpr float kSealIceBaseHeight = 0.90f;
  static constexpr float kSealIceBaseFloat = 0.25f;

  // 通常流氷（小型・中型・大型）サイズ・浮力
  static constexpr int kIceSmallCategoryChance = 45;
  static constexpr int kIceMediumCategoryChance = 80;

  static constexpr float kIceSmallWidth = 1.2f;
  static constexpr float kIceSmallDepth = 1.2f;
  static constexpr float kIceSmallHeight = 0.60f;
  static constexpr float kIceSmallFloat = 0.20f;

  static constexpr float kIceMediumWidth = 2.2f;
  static constexpr float kIceMediumDepth = 2.2f;
  static constexpr float kIceMediumHeight = 0.90f;
  static constexpr float kIceMediumFloat = 0.25f;

  static constexpr float kIceLargeWidth = 3.5f;
  static constexpr float kIceLargeDepth = 3.5f;
  static constexpr float kIceLargeHeight = 1.20f;
  static constexpr float kIceLargeFloat = 0.30f;

  // 流氷ドリフト・ボビング定数
  static constexpr float kIceDriftBaseAmount = 0.2f;
  static constexpr float kIceDriftBaseSpeed = 0.3f;
  static constexpr float kIceRotSpeedBase = 0.04f;
  static constexpr float kIceBobbingBaseSpeed = 0.5f;
  static constexpr float kIceBobbingBaseAmount = 0.05f;
  static constexpr float kIceRollPitchBaseMultiplier = 0.012f;
  static constexpr float kIceSealSpinVariationBase = 0.85f;
  static constexpr float kDefaultDriftIceFlowSpeed = 0.20f; // 停止時・潮流による流氷の基本流速 (m/frame)

  static constexpr int kDriftIceCount_ = 72;
  std::vector<DriftIce> driftIces_;
  bool driftIceEnabled_ = true;
  bool isDriftIceFlowingWhenStopped_ = true; // 道路が停止中（カウントダウン時等）も流氷を潮流で流すフラグ
  Vector4 driftIceColor_ = kDefaultDriftIceColor;
  float driftIceBobbingScale_ = kDefaultScale;
  float driftIceBobbingSpeedScale_ = kDefaultScale;
  float driftIceSizeScale_ = kDefaultScale;
  float driftIceThicknessScale_ = kDefaultScale; // size.y（縦の厚み）倍率
  float driftIceHeightOffset_ = 0.0f;
  float driftIceDistanceOffset_ = 0.0f;
  bool driftIceLighting_ = false;
  float waterTime_ = 0.0f;
  ModelData driftIceModelData_;
  ModelData babySealModelData_;
  float babySealSpinSpeed_ = kDefaultBabySealSpinSpeed; // アザラシが水面をスピンする速度 (rad/s)

  float CalculateWaterHeight(float x, float z, float time) const;
  void GenerateDriftIce(Matrix4x4 view = IdentityMatrix());
  void UpdateDriftIce(Matrix4x4 view, float currentScroll, float timeScale);
  void ResetDriftIce();
  void SetupSingleDriftIce(DriftIce &ice, bool spawnFarAway, int index);

  ModelData roadModelData_;
  Vector4 roadColor_ = kDefaultRoadColor;
  class GameObjectManager *manager_ = nullptr;
  std::vector<std::shared_ptr<RenderObject>> chunkPool_;

  void GenerateRoadChunks(Matrix4x4 view = IdentityMatrix());
  void RebuildChunkRow(int rowIndex, int newLaneCount, float newZ, Matrix4x4 view = IdentityMatrix());

  // テクスチャ
  std::unique_ptr<Texture> texture_ = std::make_unique<Texture>();

  // 障害物管理
  static constexpr int kMaxObstacles_ = 20; // 障害物の最大保持数
  std::shared_ptr<Obstacle>
      obstacles_[kMaxObstacles_]; // 障害物インスタンス配列
  int nextObstacleIndex_ = 0;     // 次に使用する障害物のインデックス

  float obstacleInterval_ =
      kDefaultObstacleInterval; // 次回生成までの距離（動的計算値・さらに狭めた設定）
  float distanceSinceLastSpawn_ = 0.0f; // 前回生成からの移動距離

  float baseActionFrames_ =
      kDefaultBaseActionFrames; // 回避アクション所要フレーム数（ジャンプ・しゃがみ完了所要フレーム）
  float minGraceFrames_ =
      kDefaultMinGraceFrames; // 最小猶予フレーム数（着地・立ち上がり後最低5フレームの猶予を保証）
  float maxGraceFrames_ =
      kDefaultMaxGraceFrames; // 最大猶予フレーム数（5〜15フレームの適度な揺らぎ）
  float minObstacleDistance_ =
      kDefaultMinObstacleDistance; // 最小生成間隔（距離の下限・物理的接触防止）
  float maxObstacleDistance_ =
      kDefaultMaxObstacleDistance; // 最大生成間隔（高速時にもフレーム猶予を圧縮しない上限値）

  float noSpawnChance_ =
      kDefaultNoSpawnChance; // 障害物が出現しない空ウェーブの発生確率（密度を保つためさらに控えめに設定）
  int consecutiveNoSpawnCount_ = 0; // 連続して空ウェーブが発生した回数
  int maxConsecutiveNoSpawn_ = kDefaultMaxConsecutiveNoSpawn;   // 連続空ウェーブの最大許容回数

  float cameraItemInterval_ =
      kDefaultCameraItemInterval; // カメラアイテムの生成間隔（旧・互換性用）
  float distanceSinceLastCameraItem_ =
      0.0f; // 前回カメラアイテム生成からの移動距離（旧・互換性用）

  // アイテムクールタイム管理（秒単位）
  std::map<Obstacle::Type, ItemCoolDownSetting> itemCoolDowns_;
  float itemSpawnChance_ = kDefaultItemSpawnChance; // アイテムが出現するウェーブの確率（15%）

  // ゲームオーバーフラグ
  bool isGameOver_ = false;

  // 生成の一時停止フラグ
  bool isSpawningPaused_ = false;

  // 変更フラグ
  bool isDirty_ = false;

  // 狭まる区間（レーン減少）の管理
  bool isNarrowingSection_ = false;
  float narrowingTimer_ = 0.0f;

  // 障害物をスポーンする
  void SpawnObstacles(float spawnZ);

  // 移動速度と猶予フレームから次回生成距離を計算する
  void CalculateNextObstacleInterval();

public:
  void Initialize(ModelData roadModelData, ModelData fallenTreeModelData,
                  ModelData iceArchwayModelData, ModelData iceWallModelData,
                  ModelData bonusModelData, ModelData iceBomModelData,
                  ModelData reflectingAttackModelData,
                  class GameObjectManager *manager);
  void Initialize(ModelData roadModelData, ModelData fallenTreeModelData,
                  ModelData iceArchwayModelData, ModelData iceWallModelData,
                  ModelData bonusModelData, class GameObjectManager *manager);
  void Initialize(ModelData roadModelData, ModelData obstacleModelData,
                  ModelData bonusModelData, class GameObjectManager *manager);
  void Update(Matrix4x4 view, float timeScale = kDefaultTimeScale, float waterTime = kAutoWaterTime);
  void EditorUpdate(Matrix4x4 view, float waterTime = kAutoWaterTime);
  void Draw(class Draw &draw);

  // ゲッター
  int GetTargetLaneCount() const { return targetLaneCount_; }
  int GetLaneCount() const { return GetLaneCountAtZ(0.0f); }
  int GetMinLaneIndex() const { return GetMinLaneIndexAtZ(0.0f); }
  int GetMaxLaneIndex() const { return GetMaxLaneIndexAtZ(0.0f); }
  float GetLaneWidth() const { return laneWidth_; }
  float GetEffectiveLaneWidth() const { return GetEffectiveLaneWidthAtZ(0.0f); }
  float GetOneLaneWidthMultiplier() const { return oneLaneWidthMultiplier_; }
  int GetChunkCount() const { return kChunkCount_; }
  int GetBackwardChunks() const { return kBackwardChunks_; }
  int GetForwardChunks() const { return kForwardChunks_; }
  float GetChunkLength() const { return chunkLength_; }

  // 任意Z座標におけるレーン情報の取得
  const ChunkRowInfo& GetChunkRowInfoAtZ(float z) const;
  int GetLaneCountAtZ(float z) const;
  int GetMinLaneIndexAtZ(float z) const;
  int GetMaxLaneIndexAtZ(float z) const;
  float GetEffectiveLaneWidthAtZ(float z) const;

  const Vector4 &GetRoadColor() const { return roadColor_; }
  void SetRoadColor(const Vector4 &color);

  // セッター
  // レーン数変更（奥から新しく出現する床から変化させる）
  void SetLaneCount(int count);
  // 全チャンク即時変更用（リセット等）
  void SetLaneCountImmediate(int count, Matrix4x4 view = IdentityMatrix());

  void SetLaneWidth(float width) {
    if (laneWidth_ == width)
      return;
    laneWidth_ = width;
    isDirty_ = true;
  }
  void SetOneLaneWidthMultiplier(float mult) {
    if (oneLaneWidthMultiplier_ == mult)
      return;
    oneLaneWidthMultiplier_ = mult;
    isDirty_ = true;
  }
  float GetScrollSpeed() const { return scrollSpeed_; }
  float GetBaseScrollSpeed() const { return baseScrollSpeed_; }
  float GetMaxScrollSpeed() const { return maxScrollSpeed_; }
  float GetObstacleInterval() const {
    return obstacleInterval_;
  } // 現在の生成間隔（距離）を取得

  // 設定
  void SetObstacleInterval(float interval) {
    obstacleInterval_ = interval;
  } // 生成間隔を設定

  float GetMinObstacleDistance() const {
    return minObstacleDistance_;
  } // 確実に避けられる最小生成間隔（距離）を取得
  void SetMinObstacleDistance(float dist) {
    minObstacleDistance_ = dist;
  } // 確実に避けられる最小生成間隔（距離）を設定

  float GetMaxObstacleDistance() const {
    return maxObstacleDistance_;
  } // 最大生成間隔（距離）を取得
  void SetMaxObstacleDistance(float dist) {
    maxObstacleDistance_ = dist;
  } // 最大生成間隔（距離）を設定

  float GetBaseActionFrames() const {
    return baseActionFrames_;
  } // 回避アクション所要フレーム数を取得
  void SetBaseActionFrames(float frames) {
    baseActionFrames_ = frames;
  } // 回避アクション所要フレーム数を設定

  float GetMinGraceFrames() const {
    return minGraceFrames_;
  } // 最小猶予フレーム数を取得
  void SetMinGraceFrames(float frames) {
    minGraceFrames_ = frames;
  } // 最小猶予フレーム数を設定

  float GetMaxGraceFrames() const {
    return maxGraceFrames_;
  } // 最大猶予フレーム数を取得
  void SetMaxGraceFrames(float frames) {
    maxGraceFrames_ = frames;
  } // 最大猶予フレーム数を設定

  float GetNoSpawnChance() const {
    return noSpawnChance_;
  } // 空ウェーブ（障害物なし）の確率を取得
  void SetNoSpawnChance(float chance) {
    noSpawnChance_ = chance;
  } // 空ウェーブ（障害物なし）の確率を設定

  // スクロール速度の設定
  void SetBaseScrollSpeed(float speed) {
    baseScrollSpeed_ = speed;
    scrollSpeed_ = speed;
  }
  void SetMaxScrollSpeed(float speed) { maxScrollSpeed_ = speed; }
  void SetScrollAcceleration(float accel) { scrollAcceleration_ = accel; }
  float GetScrollAcceleration() const { return scrollAcceleration_; }
  bool GetIsGameOver() const { return isGameOver_; }

  // ゲームオーバー設定
  void SetGameOver(bool isGameOver) { isGameOver_ = isGameOver; }

  // 生成の一時停止設定
  void SetSpawningPaused(bool paused) { isSpawningPaused_ = paused; }

  // 狭まる区間の設定
  void SetNarrowingSection(bool isNarrowing) {
    isNarrowingSection_ = isNarrowing;
  }

  // 障害物リストへのアクセス（当たり判定用）
  Obstacle *GetObstacle(int index) { return obstacles_[index].get(); }
  int GetMaxObstacles() const { return kMaxObstacles_; }

  // アイテムクールタイム設定・取得（秒単位）
  float GetItemCoolDownDuration(Obstacle::Type type) const;
  void SetItemCoolDownDuration(Obstacle::Type type, float duration);
  float GetItemCoolDownTimer(Obstacle::Type type) const;
  void SetItemCoolDownTimer(Obstacle::Type type, float timer);
  bool IsItemCoolDownReady(Obstacle::Type type) const;

  float GetItemSpawnChance() const { return itemSpawnChance_; }
  void SetItemSpawnChance(float chance) { itemSpawnChance_ = chance; }

  // 障害物を一掃する（ゲーム開始時用）
  void ClearObstacles(float safeDistance = kDefaultClearObstacleSafeDistance);

  // 流氷（Drift Ice）の設定・取得
  bool GetDriftIceEnabled() const { return driftIceEnabled_; }
  void SetDriftIceEnabled(bool enabled);
  const Vector4 &GetDriftIceColor() const { return driftIceColor_; }
  void SetDriftIceColor(const Vector4 &color);
  float GetDriftIceBobbingScale() const { return driftIceBobbingScale_; }
  void SetDriftIceBobbingScale(float scale) { driftIceBobbingScale_ = scale; }
  float GetDriftIceBobbingSpeedScale() const { return driftIceBobbingSpeedScale_; }
  void SetDriftIceBobbingSpeedScale(float scale) { driftIceBobbingSpeedScale_ = scale; }
  float GetDriftIceSizeScale() const { return driftIceSizeScale_; }
  void SetDriftIceSizeScale(float scale);
  float GetDriftIceThicknessScale() const { return driftIceThicknessScale_; }
  void SetDriftIceThicknessScale(float scale);
  float GetDriftIceHeightOffset() const { return driftIceHeightOffset_; }
  void SetDriftIceHeightOffset(float offset) { driftIceHeightOffset_ = offset; }
  float GetDriftIceDistanceOffset() const { return driftIceDistanceOffset_; }
  void SetDriftIceDistanceOffset(float offset) { driftIceDistanceOffset_ = offset; }
  bool GetDriftIceLighting() const { return driftIceLighting_; }
  void SetDriftIceLighting(bool enabled);
  float GetBabySealSpinSpeed() const { return babySealSpinSpeed_; }
  void SetBabySealSpinSpeed(float speed);
  bool GetDriftIceFlowingWhenStopped() const { return isDriftIceFlowingWhenStopped_; }
  void SetDriftIceFlowingWhenStopped(bool flowing) { isDriftIceFlowingWhenStopped_ = flowing; }

  float GetWaterTime() const { return waterTime_; }
  void SetWaterTime(float time) { waterTime_ = time; }

  // 波（海面プレーン）の範囲設定・取得
  float GetWaterForwardExtension() const { return waterForwardExtension_; }
  void SetWaterForwardExtension(float ext);
  float GetWaterBackwardExtension() const { return waterBackwardExtension_; }
  void SetWaterBackwardExtension(float ext);
  float GetWaterWidthScale() const { return waterWidthScale_; }
  void SetWaterWidthScale(float scale);
  void UpdateSidePlanesTransform(Matrix4x4 view = IdentityMatrix());

  // リセット
  void Reset();
};

#pragma once
#include <Engine.h>
#include <memory>

#include "GameObject.h"
#include "Collision.h"

/// <summary>
/// 障害物を表すクラス
/// ステージ上に配置され、手前に向かってスクロールする
/// </summary>
class Obstacle : public GameObject {
public:
  enum class Type {
    Low,                  // ジャンプで避ける（低い障害物）
    High,                 // 転がりで避ける（高い障害物・バー）
    Wall,                 // レーン移動で避ける（壁）
    Bonus,                // 当たると吹き飛ぶボーナスエネミー
    GuideFloor,           // 中央へ誘導するトリガー床
    CameraItem,           // 取るとカメラが移動するアイテム
    BarrierItem,          // バリアを張るアイテム
    ClearItem,            // 障害物を消すアイテム
    BossItem,             // ボス戦へ移行するアイテム
    BossAttack,           // ボスの攻撃（白、飛び越え不可）
    BossAttackReflectable, // ボスの攻撃（緑、跳ね返し可能）
    BossAttackLow,        // ボスの低空攻撃（ジャンプで避ける）
    BossAttackHigh        // ボスの高空攻撃（スライディング/ロールで避ける）
  };
  static constexpr float kPlayerObstacleDespawnZ = -100.0f;

private:
  std::unique_ptr<Model> lowModel_ = std::make_unique<Model>();      // FallenTree
  std::unique_ptr<Model> highModel_ = std::make_unique<Model>();     // IceArchway
  std::unique_ptr<Model> wallModel_ = std::make_unique<Model>();     // IceWall
  std::unique_ptr<Model> bonusModel_ = std::make_unique<Model>();    // Bonus
  std::unique_ptr<Model> itemModel_ = std::make_unique<Model>();     // Items / Fallback
  std::unique_ptr<Model> iceBomModel_ = std::make_unique<Model>();           // IceBom
  std::unique_ptr<Model> reflectingAttackModel_ = std::make_unique<Model>(); // ReflectingAttack
  std::unique_ptr<Model> barrierItemModel_ = std::make_unique<Model>();      // shieldItem (Billboard)
  std::unique_ptr<Model> bossItemModel_ = std::make_unique<Model>();         // BossItems (Billboard)
  std::unique_ptr<Model> clearItemModel_ = std::make_unique<Model>();        // BomItem (Billboard)
  std::unique_ptr<Model> oneLaneItemModel_ = std::make_unique<Model>();      // Onelane (Billboard)
  Model *currentModel_ = nullptr;

  float itemFloatTimer_ = 0.0f; // アイテムの浮遊ボビング演出用タイマー

  ModelData lowModelData_;
  ModelData highModelData_;
  ModelData wallModelData_;
  ModelData bonusModelData_;
  ModelData iceBomModelData_;
  ModelData reflectingAttackModelData_;
  Type type_ = Type::Wall;

private:
  static constexpr float kDefaultReflectDuration = 36.0f;
  static constexpr float kDefaultReflectArcHeight = 7.0f;
  static constexpr float kDefaultGravity = 0.015f;
  static constexpr float kDefaultFallDuration = 25.0f;
  static constexpr float kDefaultDropHeight = 15.0f;
  static constexpr float kBaseScrollSpeed = 0.2f;
  static constexpr Vector4 kNormalColor = {1.0f, 1.0f, 1.0f, 1.0f};
  static constexpr float kScaleOffsetFactor = 0.5f;
  static constexpr float kItemSpawnHeightOffset = 0.5f;
  static constexpr float kBlowAwayCameraZThreshold = -13.0f;
  static constexpr float kBlowAwayDespawnYThreshold = -20.0f;
  static constexpr float kBossAttackDespawnZ = 20.0f;
  static constexpr float kItemBobbingSpeed = 0.05f;
  static constexpr float kItemBobbingAmplitude = 0.15f;
  static constexpr float kItemBobbingPhaseMultiplier = 2.0f;
  static constexpr Vector4 kGuideFloorColor = {0.0f, 1.0f, 1.0f, 0.5f};
  static constexpr float kBlowAwayInitialVelocityY = 0.6f;
  static constexpr float kBlowAwayInitialVelocityZ = -0.8f;
  static constexpr float kBlowAwayRandRangeX = 0.4f;
  static constexpr Vector3 kBlowAwayAngularVelocity = {-0.15f, 0.1f, 0.05f};
  static constexpr Vector3 kReflectAngularVelocity = {-0.35f, 0.25f, 0.15f};
  static constexpr float kDefaultCollisionSize = 1.0f;
  static constexpr float kDefaultSpeedMultiplier = 1.0f;
  static constexpr float kBlowAwayStickDropVelocityY = -0.1f;
  static constexpr float kBlowAwayStickRandScaleX = 0.1f;
  static constexpr float kBarrierForceToCenterFrames = 30.0f;

  struct ObstacleProfile {
    Vector3 collisionSize; // width, height, depth
    Vector3 scale;
    Vector3 rotate;
  };

  static constexpr ObstacleProfile kProfileLow = {
    {1.5f, 1.0f, 1.0f}, {1.404f, 1.565f, 1.502f}, {0.0f, kPiOver2, 0.0f}
  };
  static constexpr ObstacleProfile kProfileHigh = {
    {1.5f, 3.0f, 1.0f}, {3.021f, 3.195f, 1.566f}, {0.0f, kPiOver2, 0.0f}
  };
  static constexpr ObstacleProfile kProfileWall = {
    {1.5f, 3.0f, 1.0f}, {1.497f, 3.886f, 3.135f}, {0.0f, 0.0f, 0.0f}
  };
  static constexpr ObstacleProfile kProfileBonus = {
    {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, kPi, 0.0f}
  };
  static constexpr ObstacleProfile kProfileGuideFloor = {
    {2.0f, 0.5f, 2.0f}, {2.0f, 0.1f, 10.0f}, {0.0f, 0.0f, 0.0f}
  };
  static constexpr ObstacleProfile kProfileItemBillboardSmall = {
    {1.0f, 1.0f, 1.0f}, {0.8f, 0.8f, 1.0f}, {0.0f, 0.0f, 0.0f}
  };
  static constexpr ObstacleProfile kProfileBossItem = {
    {1.5f, 1.5f, 1.5f}, {1.1f, 1.1f, 1.0f}, {0.0f, 0.0f, 0.0f}
  };
  static constexpr ObstacleProfile kProfileBossAttack = {
    {1.6f, 2.0f, 1.6f}, {0.016f, 0.016f, 0.016f}, {0.0f, 0.0f, 0.0f}
  };
  static constexpr ObstacleProfile kProfileBossAttackReflectable = {
    {1.6f, 2.0f, 2.5f}, {0.025f, 0.025f, 0.025f}, {0.0f, 0.0f, 0.0f}
  };

  bool isHit_ = false;
  bool isReflected_ = false;                  // ボスへの跳ね返しフラグ
  Vector3 reflectedTarget_{0.0f, 0.0f, 0.0f}; // 跳ね返された際の目標座標
  Vector3 reflectStartPos_{0.0f, 0.0f, 0.0f}; // 跳ね返された瞬間の座標
  float reflectTimer_ = 0.0f;                 // 跳ね返り経過フレーム数
  float reflectDuration_ = kDefaultReflectDuration; // 跳ね返りにかかるフレーム数 (約0.6秒)
  float reflectArcHeight_ = kDefaultReflectArcHeight; // 弧の高さ（Y方向の膨らみ）
  Vector3 velocity_{0.0f, 0.0f, 0.0f};
  float gravity_ = kDefaultGravity;

  // 上空からの落下演出用
  bool isFalling_ = false;
  float fallTimer_ = 0.0f;
  float fallDuration_ = kDefaultFallDuration; // 落下にかかるフレーム数
  float dropHeight_ = kDefaultDropHeight;     // 落下開始の高さオフセット
  float targetY_ = 0.0f;                      // 着地目標Y座標
  bool justLanded_ = false;                   // 着地した瞬間フラグ

  // 当たり判定のサイズ
  float collisionWidth_ = kDefaultCollisionSize;
  float collisionHeight_ = kDefaultCollisionSize;
  float collisionDepth_ = kDefaultCollisionSize;

public:
  /// <summary>
  /// コンストラクタ
  /// </summary>
  Obstacle();

  /// <summary>
  /// デストラクタ
  /// </summary>
  ~Obstacle();

  /// <summary>
  /// 全障害物・攻撃・アイテムモデルデータの初期化
  /// </summary>
  void Initialize(ModelData lowData, ModelData highData, ModelData wallData,
                  ModelData bonusData, ModelData iceBomData,
                  ModelData reflectingAttackData, Type type);

  /// <summary>
  /// 障害物モデルデータの初期化（基本セット）
  /// </summary>
  void Initialize(ModelData lowData, ModelData highData, ModelData wallData,
                  ModelData bonusData, Type type);

  /// <summary>
  /// 障害物モデルデータの初期化（最小セット）
  /// </summary>
  void Initialize(ModelData normalData, ModelData bonusData, Type type);

  /// <summary>
  /// 障害物・アイテムのタイプを設定し、対応するモデル・当たり判定サイズを切り替え
  /// </summary>
  /// <param name="type">障害物タイプ</param>
  void SetType(Type type);

  /// <summary>
  /// 障害物を指定位置に配置しアクティブ化
  /// </summary>
  /// <param name="x">X座標（レーン位置）</param>
  /// <param name="y">Y座標</param>
  /// <param name="z">Z座標（出現距離）</param>
  void Spawn(float x, float y, float z);

  /// <summary>
  /// ステージスクロールに伴う毎フレーム移動・アニメーション更新
  /// </summary>
  /// <param name="view">ビュー行列</param>
  /// <param name="scrollSpeed">スクロール速度</param>
  void StageUpdate(Matrix4x4 view, float scrollSpeed);

  /// <summary>
  /// プレイヤー衝突時の吹き飛びアニメーション処理（ボーナスエネミー等）
  /// </summary>
  void OnBlowAway();

  /// <summary>
  /// 障害物被弾処理
  /// </summary>
  void OnHit();
  void SetReflected(bool reflected) {
    isReflected_ = reflected;
    if (reflected) {
      isFalling_ = false;
      reflectStartPos_ = transform_.translate;
      reflectTimer_ = 0.0f;
    }
  }
  bool GetIsReflected() const { return isReflected_; }
  void SetReflectedTarget(const Vector3 &target) { reflectedTarget_ = target; }
  void SetReflectArcHeight(float height) { reflectArcHeight_ = height; }
  float GetReflectArcHeight() const { return reflectArcHeight_; }
  void SetReflectDuration(float duration) { reflectDuration_ = duration; }
  float GetReflectDuration() const { return reflectDuration_; }
  float GetReflectProgress() const {
    return (reflectDuration_ > 0.0f) ? (reflectTimer_ / reflectDuration_) : 1.0f;
  }

  // 落下関連
  bool GetIsFalling() const { return isFalling_; }
  bool GetJustLanded() const { return justLanded_; }
  void SetDropHeight(float height) { dropHeight_ = height; }
  float GetDropHeight() const { return dropHeight_; }
  void SetFallDuration(float duration) { fallDuration_ = duration; }
  float GetFallDuration() const { return fallDuration_; }

  // Override standard Update and Draw to avoid GameObjectManager from
  // automatically updating scroll/drawing without context
  void Update(Matrix4x4 /*view*/, float /*speedMultiplier*/) override {}
  void Update(Matrix4x4 view) { Update(view, kDefaultSpeedMultiplier); }
  void Draw(class Draw &draw) override;
  void ImGuiInnerComponents() override;

  bool HasMaterial() const override {
    return (currentModel_ && currentModel_->GetComponent<MaterialComponent>() != nullptr) ||
           GameObject::HasMaterial();
  }

  bool IsTransparent() const override;

  /// <summary>
  /// 障害物を非アクティブ化してリセット
  /// </summary>
  void Deactivate() {
    isActive_ = false;
    isFalling_ = false;
    justLanded_ = false;
    isReflected_ = false;
    reflectTimer_ = 0.0f;
  }

  // ゲッター
  Type GetType() const { return type_; }
  bool GetIsHit() const { return isHit_; }
  float GetCollisionWidth() const { return collisionWidth_; }
  float GetCollisionHeight() const { return collisionHeight_; }
  float GetCollisionDepth() const { return collisionDepth_; }
  AABB GetColliderAABB() const;

  // ポリモーフィズム用メソッド
  virtual bool IsRegularObstacle() const {
    return type_ == Type::Low || type_ == Type::High || type_ == Type::Wall;
  }
  virtual bool IsItem() const {
    return type_ == Type::CameraItem || type_ == Type::BarrierItem ||
           type_ == Type::ClearItem || type_ == Type::BossItem;
  }
  virtual bool IsBossAttackType() const {
    return type_ == Type::BossAttack || type_ == Type::BossAttackReflectable ||
           type_ == Type::BossAttackLow || type_ == Type::BossAttackHigh;
  }
  virtual bool CanRollUnder() const {
    return type_ == Type::High || type_ == Type::BossAttackHigh;
  }
  virtual bool RequiresLaneChangeToAvoid() const {
    return type_ == Type::Wall || type_ == Type::BossAttack || type_ == Type::BossAttackReflectable;
  }
  virtual bool IsLargeObstacle() const {
    return type_ == Type::Wall || type_ == Type::High ||
           type_ == Type::BossAttack || type_ == Type::BossAttackHigh;
  }

  /// <summary>
  /// プレイヤーとの衝突処理
  /// </summary>
  /// <param name="scene">ゲームシーンポインタ</param>
  /// <param name="player">プレイヤーポインタ</param>
  /// <returns>衝突によりループ処理を中断する場合true</returns>
  virtual bool OnPlayerCollision(class GameScene* scene, class Player* player);
};

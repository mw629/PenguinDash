#pragma once
#include <Engine.h>
#include <memory>

#include "GameObject.h"

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
    BossAttackReflectable // ボスの攻撃（緑、跳ね返し可能）
  };

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

public:
  static constexpr float kDefaultReflectDuration = 36.0f;
  static constexpr float kDefaultReflectArcHeight = 7.0f;
  static constexpr float kDefaultGravity = 0.015f;
  static constexpr float kDefaultFallDuration = 25.0f;
  static constexpr float kDefaultDropHeight = 15.0f;
  static constexpr float kBaseScrollSpeed = 0.2f;

private:
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
  float collisionWidth_ = 1.0f;
  float collisionHeight_ = 1.0f;
  float collisionDepth_ = 1.0f;

public:
  Obstacle();
  ~Obstacle();

  void Initialize(ModelData lowData, ModelData highData, ModelData wallData,
                  ModelData bonusData, ModelData iceBomData,
                  ModelData reflectingAttackData, Type type);
  void Initialize(ModelData lowData, ModelData highData, ModelData wallData,
                  ModelData bonusData, Type type);
  void Initialize(ModelData normalData, ModelData bonusData, Type type);
  void SetType(Type type);

  /// <summary>
  /// 障害物を指定位置に配置しアクティブにする
  /// </summary>
  void Spawn(float x, float y, float z);

  /// <summary>
  /// 毎フレームの更新（手前にスクロール）
  /// </summary>
  void StageUpdate(Matrix4x4 view, float scrollSpeed);

  /// <summary>
  /// プレイヤーに当たった時のふっとび処理
  /// </summary>
  void OnBlowAway();

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
  void Update(Matrix4x4 view, float speedMultiplier = 1.0f) override {}
  void Draw(class Draw &draw) override;
  void ImGuiInnerComponents() override;

  bool HasMaterial() const override {
    return (currentModel_ && currentModel_->GetComponent<MaterialComponent>() != nullptr) ||
           GameObject::HasMaterial();
  }

  /// <summary>
  /// 画面の手前を過ぎたら非アクティブにする
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
};

#pragma once
#include "GameObject.h"
#include <Engine.h>
#include <memory>

enum class BossState {
  Appearance, // 出現
  Battle,     // 戦闘
  Defeat,     // 撃破
  Victory     // 勝利（プレイヤー敗北）
};

class Boss : public GameObject {
public:
  static constexpr int kMaxHP = 5;
  static constexpr float kAppearanceDuration = 2.0f;
  static constexpr float kDefeatDuration = 3.0f;
  static constexpr float kDamageFlashDuration = 0.5f;
  static constexpr Vector3 kDefaultStartPos = {-18.0f, 20.0f, -40.0f};
  static constexpr Vector3 kDefaultTargetPos = {-6.0f, 3.0f, -2.0f};
  static constexpr Vector3 kDefaultBossScale = {5.0f, 5.0f, 5.0f};
  static constexpr Vector4 kDamageColor = {2.0f, 0.4f, 0.4f, 1.0f};

private:
  std::unique_ptr<Model> model_ = std::make_unique<Model>();
  int hp_ = kMaxHP;
  bool isHit_ = false;
  float hitTimer_ = 0.0f;
  BossState state_ = BossState::Appearance;
  float stateTimer_ = 0.0f;
  float battleAnimTimer_ = 0.0f;

  Vector3 startPos_;
  Vector3 targetPos_;

public:
  Boss();
  ~Boss();

  void Initialize(ModelData modelData);
  void Reset();
  void Spawn(float x, float y, float z);

  void Update(Matrix4x4 view, float speedMultiplier = 1.0f) override;
  void Draw(class Draw &draw) override;
  void ImGuiInnerComponents() override;

  bool HasMaterial() const override {
    return (model_ && model_->GetComponent<MaterialComponent>() != nullptr) ||
           GameObject::HasMaterial();
  }

  void OnDamage();
  int GetHP() const { return hp_; }
  int GetMaxHP() const { return kMaxHP; }

  BossState GetState() const { return state_; }
  float GetStateTimer() const { return stateTimer_; }
  void ChangeState(BossState nextState);

  Vector3 GetTargetPos() const { return targetPos_; }
  void SetTargetPos(const Vector3 &targetPos) { targetPos_ = targetPos; }
  Vector3 &GetTargetPosRef() { return targetPos_; }
};

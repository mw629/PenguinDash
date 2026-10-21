#pragma once
#include <Engine.h>
#include <GameObjects/Object/2d/Sprite.h>
#include <Math/Calculation.h>
#include <memory>
#include <vector>

/// @brief ランキング1位獲得時にリザルト画面前へ挿入されるペンギンカットイン演出
class RankingCutinEffect {
public:
  enum class State {
    Inactive,
    FlashAndSlash, // 0.00s ~ 0.25s: 閃光・衝撃音・スライドアタック
    BannerHold,    // 0.25s ~ 1.70s: カットイン帯ホールド・目力・集中線・文字ポップ
    SlashExit,     // 1.70s ~ 2.10s: 切断エフェクト・リザルト移行
    Finished       // 完了
  };

  struct ShardParticle {
    Vector2 pos;
    Vector2 velocity;
    float rotation = 0.0f;
    float rotSpeed = 0.0f;
    Vector2 size = {16.0f, 16.0f};
    Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};
    float life = 1.0f;
    float maxLife = 1.0f;
  };

private:
  static constexpr float kDefaultTotalDuration = 2.10f;
  static constexpr float kFlashDuration = 0.20f;
  static constexpr float kSlideInDuration = 0.30f;
  static constexpr float kExitStartTimer = 1.65f;
  static constexpr float kEyeGlintStart = 0.32f;
  static constexpr float kEyeGlintDuration = 0.45f;

  State state_ = State::Inactive;
  float timer_ = 0.0f;
  float totalDuration_ = kDefaultTotalDuration;

  // カットイン本体スプライト
  std::unique_ptr<Sprite> cutinSprite_;
  SpriteData cutinSpriteData_;
  int cutinTextureHandle_ = -1;

  // 上下スラッシュ帯スプライト（白テクスチャを色付け）
  std::unique_ptr<Sprite> topBarSprite_;
  std::unique_ptr<Sprite> bottomBarSprite_;
  std::unique_ptr<Sprite> accentLineSprite_;
  std::unique_ptr<Sprite> eyeGlintSprite_;
  int whiteTextureHandle_ = -1;

  // シャードパーティクル
  std::vector<ShardParticle> shards_;

  // トランスフォーム設定
  Vector2 bannerCenterPos_ = {640.0f, 360.0f};
  Vector2 bannerSize_ = {1280.0f, 520.0f};
  float bannerAngle_ = -0.06f; // 斜めスラント傾き
  float screenShake_ = 0.0f;

  bool isSoundPlayedImpact_ = false;
  bool isSoundPlayedHold_ = false;
  bool isSoundPlayedExit_ = false;

public:
  RankingCutinEffect() = default;
  ~RankingCutinEffect() = default;

  void Initialize(class Texture *texture);
  void Trigger();
  void Reset();
  void Update(float dt);
  void Render(class Draw &draw);
  void ImGui();

  bool IsActive() const { return state_ != State::Inactive && state_ != State::Finished; }
  bool IsFinished() const { return state_ == State::Finished; }

  /// @brief 入力によるスキップ判定（開始から一定時間経過後のみスキップ可）
  void RequestSkip();

private:
  void SpawnShards();
  void UpdateShards(float dt);
};

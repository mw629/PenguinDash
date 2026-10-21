#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "RankingCutinEffect.h"
#include <Graphics/Render/Draw.h>
#include <Resource/Texture.h>
#include <System/SoundManager.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <imgui.h>

namespace {

float EaseOutExpo(float t) {
  return (t >= 1.0f) ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);
}

float EaseOutBack(float t, float s = 1.70158f) {
  float f = t - 1.0f;
  return f * f * ((s + 1.0f) * f + s) + 1.0f;
}
} // namespace

Sprite *RankingCutinEffect::GetCurrentCutinSprite() {
  switch (currentType_) {
  case CutinType::Score:
    return scoreCutinSprite_ ? scoreCutinSprite_.get() : nullptr;
  case CutinType::Distance:
    return distanceCutinSprite_ ? distanceCutinSprite_.get() : nullptr;
  case CutinType::Double:
    return doubleCutinSprite_ ? doubleCutinSprite_.get() : nullptr;
  default:
    return scoreCutinSprite_ ? scoreCutinSprite_.get() : nullptr;
  }
}

void RankingCutinEffect::Initialize(Texture *texture) {
  if (!texture)
    return;

  // テクスチャの読み込み（JPG形式のみで軽量化）
  scoreTextureHandle_ =
      texture->CreateTexture("Resources/Texture/RankingCutin_Score.jpg");
  if (scoreTextureHandle_ < 0) {
    scoreTextureHandle_ =
        texture->CreateTexture("Resources/Texture/RankingCutin.jpg");
  }

  distanceTextureHandle_ =
      texture->CreateTexture("Resources/Texture/RankingCutin_Distance.jpg");
  if (distanceTextureHandle_ < 0) {
    distanceTextureHandle_ = scoreTextureHandle_;
  }

  doubleTextureHandle_ =
      texture->CreateTexture("Resources/Texture/RankingCutin_Double.jpg");
  if (doubleTextureHandle_ < 0) {
    doubleTextureHandle_ = scoreTextureHandle_;
  }

  // 単色スプライト用の共通白テクスチャ
  whiteTextureHandle_ =
      texture->CreateTexture("Resources/Texture/white64x64.png");

  // メインカットインスプライトの共通データ
  cutinSpriteData_.transform.scale = {1.0f, 1.0f, 1.0f};
  cutinSpriteData_.transform.translate = {bannerCenterPos_.x,
                                          bannerCenterPos_.y, 0.0f};
  cutinSpriteData_.transform.rotate = {0.0f, 0.0f, bannerAngle_};
  cutinSpriteData_.size = bannerSize_;
  cutinSpriteData_.pivot = {0.5f, 0.5f};
  cutinSpriteData_.textureArea[0] = {0.0f, 0.0f};
  cutinSpriteData_.textureArea[1] = {1.0f, 1.0f};
  cutinSpriteData_.scaleMode = SpriteScaleMode::Fit;
  cutinSpriteData_.anchor = SpriteAnchor::None;

  auto createSprite = [&](int texHandle) {
    auto sp = std::make_unique<Sprite>();
    sp->Initialize(cutinSpriteData_, texHandle);
    sp->GetMaterial()->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
    sp->SettingWvp();
    return sp;
  };

  scoreCutinSprite_ = createSprite(scoreTextureHandle_);
  distanceCutinSprite_ = createSprite(distanceTextureHandle_);
  doubleCutinSprite_ = createSprite(doubleTextureHandle_);

  // 上下スラッシュ帯（白テクスチャを色付け）
  auto createSlantBar = [&](int texHandle, const Vector2 &size,
                            const Vector4 &color) {
    SpriteData data;
    data.transform.scale = {1.0f, 1.0f, 1.0f};
    data.transform.translate = {bannerCenterPos_.x, bannerCenterPos_.y, 0.0f};
    data.transform.rotate = {0.0f, 0.0f, bannerAngle_};
    data.size = size;
    data.pivot = {0.5f, 0.5f};
    data.textureArea[0] = {0.0f, 0.0f};
    data.textureArea[1] = {1.0f, 1.0f};
    data.scaleMode = SpriteScaleMode::None;
    data.anchor = SpriteAnchor::None;

    auto sp = std::make_unique<Sprite>();
    sp->Initialize(data, texHandle);
    sp->GetMaterial()->SetColor(color);
    sp->SettingWvp();
    return sp;
  };

  topBarSprite_ = createSlantBar(whiteTextureHandle_, {1500.0f, 26.0f},
                                 {1.0f, 0.72f, 0.12f, 0.95f});
  bottomBarSprite_ = createSlantBar(whiteTextureHandle_, {1500.0f, 26.0f},
                                    {0.10f, 0.12f, 0.20f, 0.95f});
  accentLineSprite_ = createSlantBar(whiteTextureHandle_, {1500.0f, 6.0f},
                                     {0.95f, 0.22f, 0.25f, 1.0f});
  eyeGlintSprite_ = createSlantBar(whiteTextureHandle_, {36.0f, 36.0f},
                                   {1.0f, 1.0f, 0.85f, 1.0f});
}

void RankingCutinEffect::Trigger(CutinType type) {
  state_ = State::FlashAndSlash;
  currentType_ = type;
  timer_ = 0.0f;
  screenShake_ = 12.0f;
  isSoundPlayedImpact_ = false;
  isSoundPlayedHold_ = false;
  isSoundPlayedExit_ = false;

  // 上下帯のテーマカラー設定
  if (topBarSprite_ && bottomBarSprite_ && accentLineSprite_) {
    if (currentType_ == CutinType::Score) {
      // アーケードゴールド & パイロットレッド
      topBarSprite_->GetMaterial()->SetColor({1.0f, 0.72f, 0.12f, 0.98f});
      bottomBarSprite_->GetMaterial()->SetColor({0.10f, 0.12f, 0.20f, 0.95f});
      accentLineSprite_->GetMaterial()->SetColor({0.95f, 0.22f, 0.25f, 1.0f});
    } else if (currentType_ == CutinType::Distance) {
      // パステルスカイブルー & ミント
      topBarSprite_->GetMaterial()->SetColor({0.15f, 0.72f, 0.98f, 0.95f});
      bottomBarSprite_->GetMaterial()->SetColor({0.06f, 0.14f, 0.24f, 0.95f});
      accentLineSprite_->GetMaterial()->SetColor({0.25f, 0.98f, 0.70f, 1.0f});
    } else {
      // 究極のゴールド & ネオンピンク
      topBarSprite_->GetMaterial()->SetColor({1.0f, 0.82f, 0.15f, 0.98f});
      bottomBarSprite_->GetMaterial()->SetColor({0.18f, 0.05f, 0.25f, 0.95f});
      accentLineSprite_->GetMaterial()->SetColor({1.0f, 0.32f, 0.75f, 1.0f});
    }
  }

  SpawnShards();

  // 開幕インパクト音（決定音 + 祝福音）
  auto sound = SoundManager::GetInstance();
  if (sound) {
    sound->PlaySE(SoundManager::SE::BossReflect, 0.9f);
    sound->PlaySE(SoundManager::SE::ClearBomb, 0.75f);
  }
}

void RankingCutinEffect::Reset() {
  state_ = State::Inactive;
  timer_ = 0.0f;
  screenShake_ = 0.0f;
  shards_.clear();
}

void RankingCutinEffect::RequestSkip() {
  // 最低0.4秒以上経過していればユーザー操作（決定・ジャンプ等）でリザルトへスキップ可能
  if (IsActive() && timer_ >= 0.40f) {
    state_ = State::Finished;
  }
}

void RankingCutinEffect::SpawnShards() {
  shards_.clear();
  shards_.reserve(32);

  // かわいい紙吹雪・キラキラスパークルパーティクル
  for (int i = 0; i < 28; ++i) {
    ShardParticle s;
    s.pos = {640.0f + static_cast<float>((std::rand() % 1000) - 500),
             360.0f + static_cast<float>((std::rand() % 400) - 200)};
    float speed = 80.0f + static_cast<float>(std::rand() % 260);
    float angle = -0.5f + static_cast<float>(std::rand() % 100) * 0.01f;
    s.velocity = {-speed * std::cos(angle), -speed * std::sin(angle) + 40.0f};
    s.rotation = static_cast<float>(std::rand() % 314) * 0.02f;
    s.rotSpeed = (std::rand() % 2 == 0 ? 1.0f : -1.0f) *
                 (1.5f + static_cast<float>(std::rand() % 4));
    float sz = 10.0f + static_cast<float>(std::rand() % 20);
    s.size = {sz, sz * (0.5f + static_cast<float>(std::rand() % 10) * 0.08f)};
    s.life = 0.0f;
    s.maxLife = 1.3f + static_cast<float>(std::rand() % 10) * 0.08f;
    s.floatPhase = static_cast<float>(std::rand() % 314) * 0.02f;

    int colType = std::rand() % 4;
    if (currentType_ == CutinType::Score) {
      if (colType == 0)
        s.color = {1.0f, 0.78f, 0.15f, 0.98f}; // ゴールドコイン
      else if (colType == 1)
        s.color = {0.95f, 0.22f, 0.25f, 0.95f}; // ルビーレッド（マフラー色）
      else if (colType == 2)
        s.color = {0.20f, 0.85f, 0.45f, 0.95f}; // エメラルドグリーン
      else
        s.color = {1.0f, 1.0f, 1.0f, 0.95f}; // ダイヤモンドホワイト
    } else if (currentType_ == CutinType::Distance) {
      if (colType == 0)
        s.color = {0.20f, 0.78f, 1.0f, 0.95f}; // スカイブルー
      else if (colType == 1)
        s.color = {0.30f, 0.98f, 0.75f, 0.95f}; // ミント
      else if (colType == 2)
        s.color = {1.0f, 0.92f, 0.35f, 0.95f}; // スターイエロー
      else
        s.color = {1.0f, 1.0f, 1.0f, 0.90f}; // ピュアホワイト
    } else {
      if (colType == 0)
        s.color = {1.0f, 0.85f, 0.20f, 0.98f}; // ゴールド
      else if (colType == 1)
        s.color = {0.95f, 0.25f, 0.45f, 0.95f}; // クラウンレッド
      else if (colType == 2)
        s.color = {0.25f, 0.85f, 1.0f, 0.95f}; // シアン
      else
        s.color = {1.0f, 1.0f, 1.0f, 0.95f}; // ホワイト
    }
    shards_.push_back(s);
  }
}

void RankingCutinEffect::UpdateShards(float dt) {
  for (auto &s : shards_) {
    s.life += dt;
    s.floatPhase += dt * 4.0f;
    // ふわふわ舞う紙吹雪の挙動
    s.pos.x += (s.velocity.x + std::sin(s.floatPhase) * 25.0f) * dt;
    s.pos.y += (s.velocity.y + 40.0f * (s.life / s.maxLife)) * dt;
    s.rotation += s.rotSpeed * dt;
  }
}

void RankingCutinEffect::Update(float dt) {
  if (!IsActive())
    return;

  timer_ += dt;
  screenShake_ = (std::max)(0.0f, screenShake_ - dt * 26.0f);
  UpdateShards(dt);

  // タイムライン制御
  if (timer_ < kSlideInDuration) {
    state_ = State::FlashAndSlash;
  } else if (timer_ < kExitStartTimer) {
    state_ = State::BannerHold;
    if (!isSoundPlayedHold_) {
      isSoundPlayedHold_ = true;
      auto sound = SoundManager::GetInstance();
      if (sound) {
        sound->PlaySE(SoundManager::SE::BossHit, 0.85f);
      }
    }
  } else if (timer_ < totalDuration_) {
    state_ = State::SlashExit;
    if (!isSoundPlayedExit_) {
      isSoundPlayedExit_ = true;
      auto sound = SoundManager::GetInstance();
      if (sound) {
        sound->PlaySE(SoundManager::SE::BarrierBreak, 0.9f);
        sound->PlaySE(SoundManager::SE::Start, 0.75f);
      }
    }
  } else {
    state_ = State::Finished;
  }

  // アクティブなカットインスプライトの更新
  Sprite *activeSprite = GetCurrentCutinSprite();
  if (!activeSprite)
    return;

  Vector2 curPos = bannerCenterPos_;
  Vector2 curScale = {1.0f, 1.0f};
  float curAngle = bannerAngle_;

  if (state_ == State::FlashAndSlash) {
    // 画面右外からポップにスライドイン（EaseOutBack風）
    float t = std::clamp(timer_ / kSlideInDuration, 0.0f, 1.0f);
    float ease = EaseOutExpo(t);
    curPos.x = 1850.0f + (bannerCenterPos_.x - 1850.0f) * ease;
    float s = 1.30f - 0.30f * ease;
    curScale = {s, s};
  } else if (state_ == State::BannerHold) {
    // 中央で構え、カメラが極めてゆっくりと迫る
    float t = std::clamp((timer_ - kSlideInDuration) /
                             (kExitStartTimer - kSlideInDuration),
                         0.0f, 1.0f);
    curPos.x = bannerCenterPos_.x - 15.0f * t;
    float s = 1.0f + 0.04f * t;
    curScale = {s, s};
  } else if (state_ == State::SlashExit) {
    // 左方向へ高速で抜ける
    float t = std::clamp((timer_ - kExitStartTimer) /
                             (totalDuration_ - kExitStartTimer),
                         0.0f, 1.0f);
    float ease = EaseInQuad(t);
    curPos.x = (bannerCenterPos_.x - 15.0f) - 1600.0f * ease;
    curScale = {1.04f + 0.1f * t, 1.04f};
  }

  // 画面揺れの加算
  if (screenShake_ > 0.001f) {
    float offsetX = ((std::rand() % 100) / 50.0f - 1.0f) * screenShake_;
    float offsetY = ((std::rand() % 100) / 50.0f - 1.0f) * screenShake_;
    curPos.x += offsetX;
    curPos.y += offsetY;
  }

  Transform ct = cutinSpriteData_.transform;
  ct.translate = {curPos.x, curPos.y, 0.0f};
  ct.scale = {curScale.x, curScale.y, 1.0f};
  ct.rotate = {0.0f, 0.0f, curAngle};
  activeSprite->SetTransform(ct);
  activeSprite->SettingWvp();

  // 上下スラッシュボーダーの同期配置
  if (topBarSprite_ && bottomBarSprite_ && accentLineSprite_) {
    // 上帯
    Transform tt = ct;
    tt.translate.y -= (bannerSize_.y * 0.5f * curScale.y + 10.0f);
    topBarSprite_->SetTransform(tt);
    topBarSprite_->SettingWvp();

    // 下帯
    Transform bt = ct;
    bt.translate.y += (bannerSize_.y * 0.5f * curScale.y + 10.0f);
    bottomBarSprite_->SetTransform(bt);
    bottomBarSprite_->SettingWvp();

    // アクセントライン
    Transform at = ct;
    at.translate.y += (bannerSize_.y * 0.5f * curScale.y);
    accentLineSprite_->SetTransform(at);
    accentLineSprite_->SettingWvp();
  }

  // 瞳・ゴーグルの星型スパークル
  if (eyeGlintSprite_) {
    if (timer_ >= kEyeGlintStart &&
        timer_ <= (kEyeGlintStart + kEyeGlintDuration)) {
      float gt = (timer_ - kEyeGlintStart) / kEyeGlintDuration;
      float glintScale = (gt < 0.35f) ? (gt / 0.35f) * 1.5f
                                      : (1.0f - (gt - 0.35f) / 0.65f) * 1.5f;
      float rot = gt * 3.14159f;

      Vector2 glintPos = {curPos.x + 35.0f, curPos.y - 75.0f};
      Transform eg;
      eg.scale = {glintScale, glintScale, 1.0f};
      eg.translate = {glintPos.x, glintPos.y, 0.0f};
      eg.rotate = {0.0f, 0.0f, rot + 0.785f};
      eyeGlintSprite_->SetTransform(eg);
      eyeGlintSprite_->SettingWvp();
    }
  }
}

void RankingCutinEffect::Render(Draw &draw) {
  if (!IsActive())
    return;

  const Vector2 fullScreenSize(1280.0f, 720.0f);

  // 1. 背景の劇的ディミング
  float bgDimAlpha = 0.88f;
  if (timer_ < 0.15f) {
    bgDimAlpha = (timer_ / 0.15f) * 0.88f;
  }
  Vector4 dimColor = (currentType_ == CutinType::Score)
                         ? Vector4(0.06f, 0.02f, 0.04f, bgDimAlpha)
                     : (currentType_ == CutinType::Distance)
                         ? Vector4(0.02f, 0.04f, 0.07f, bgDimAlpha)
                         : Vector4(0.05f, 0.02f, 0.06f, bgDimAlpha);
  draw.DrawFillRect(Vector2(0.0f, 0.0f), fullScreenSize, dimColor);

  // 2. スピードライン／集中線の背景装飾
  if (state_ == State::FlashAndSlash || state_ == State::BannerHold) {
    for (int i = 0; i < 7; ++i) {
      float y = 60.0f + i * 95.0f + std::sin(timer_ * 12.0f + i) * 6.0f;
      float h = (i % 2 == 0) ? 6.0f : 14.0f;
      Vector4 col;
      if (currentType_ == CutinType::Score) {
        col = (i % 3 == 0) ? Vector4(1.0f, 0.40f, 0.65f, 0.35f)
                           : Vector4(0.0f, 0.0f, 0.0f, 0.45f);
      } else if (currentType_ == CutinType::Distance) {
        col = (i % 3 == 0) ? Vector4(0.15f, 0.75f, 1.0f, 0.35f)
                           : Vector4(0.0f, 0.0f, 0.0f, 0.45f);
      } else {
        col = (i % 3 == 0) ? Vector4(1.0f, 0.82f, 0.20f, 0.40f)
                           : Vector4(0.0f, 0.0f, 0.0f, 0.45f);
      }
      draw.DrawFillRect(Vector2(0.0f, y), Vector2(1280.0f, h), col);
    }
  }

  // 3. 上下斜めバーの描画
  if (topBarSprite_) {
    draw.DrawSprite(topBarSprite_.get());
  }
  if (bottomBarSprite_) {
    draw.DrawSprite(bottomBarSprite_.get());
  }
  if (accentLineSprite_) {
    draw.DrawSprite(accentLineSprite_.get());
  }

  // 4. メインのペンギンカットイン画像描画
  Sprite *activeSprite = GetCurrentCutinSprite();
  if (activeSprite) {
    draw.DrawSprite(activeSprite);
  }

  // 5. 紙吹雪・スターパーティクルの描画
  for (const auto &s : shards_) {
    if (s.life < s.maxLife) {
      float alpha = 1.0f - (s.life / s.maxLife);
      Vector4 c = s.color;
      c.w *= alpha;
      draw.DrawFillRect(s.pos, s.size, c);
    }
  }

  // 6. 瞳・ゴーグルの輝き（スパークル）描画
  if (eyeGlintSprite_ && timer_ >= kEyeGlintStart &&
      timer_ <= (kEyeGlintStart + kEyeGlintDuration)) {
    draw.DrawSprite(eyeGlintSprite_.get());
  }

  // 7. スラントタイポグラフィ（斜めバナー＆文字演出）
  if (state_ == State::BannerHold || state_ == State::FlashAndSlash) {
    Vector2 tagPos(50.0f, 62.0f);
    Vector2 tagSize(430.0f, 64.0f);
    Vector2 botPos(760.0f, 595.0f);
    Vector2 botSize(470.0f, 60.0f);

    // 影
    draw.DrawFillRect(Vector2(tagPos.x + 6.0f, tagPos.y + 6.0f), tagSize,
                      Vector4(0.0f, 0.0f, 0.0f, 0.85f));
    draw.DrawFillRect(Vector2(botPos.x + 5.0f, botPos.y + 5.0f), botSize,
                      Vector4(0.0f, 0.0f, 0.0f, 0.85f));

    if (currentType_ == CutinType::Score) {
      // --- スコア用バナー演出 ---
      draw.DrawFillRect(tagPos, tagSize, Vector4(1.0f, 0.72f, 0.12f, 0.98f));
      draw.DrawFillRect(Vector2(tagPos.x, tagPos.y + tagSize.y - 5.0f),
                        Vector2(tagSize.x, 5.0f),
                        Vector4(0.95f, 0.22f, 0.25f, 1.0f));

      draw.DrawMSDFString("★ HIGH SCORE No. 1 !! ★",
                          Vector2(tagPos.x + 20.0f, tagPos.y + 14.0f), 34.0f,
                          Vector4(1.0f, 1.0f, 1.0f, 1.0f), true,
                          Vector4(0.12f, 0.14f, 0.22f, 1.0f), 0.18f, 0.14f);

      draw.DrawFillRect(botPos, botSize, Vector4(0.10f, 0.12f, 0.20f, 0.98f));
      draw.DrawFillRect(botPos, Vector2(6.0f, botSize.y),
                        Vector4(1.0f, 0.72f, 0.12f, 1.0f));

      draw.DrawMSDFString("★ TOP SCORE ACHIEVED ★",
                          Vector2(botPos.x + 25.0f, botPos.y + 13.0f), 32.0f,
                          Vector4(1.0f, 1.0f, 1.0f, 1.0f), true,
                          Vector4(0.95f, 0.22f, 0.25f, 1.0f), 0.16f, 0.12f);

      draw.DrawMSDFString("BRAVE PILOT // ARCADE HIGH SCORE",
                          Vector2(65.0f, 134.0f), 20.0f,
                          Vector4(1.0f, 0.88f, 0.3f, 0.95f), true,
                          Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.15f, 0.10f);

    } else if (currentType_ == CutinType::Distance) {
      // --- 走行距離用バナー演出 ---
      draw.DrawFillRect(tagPos, tagSize, Vector4(0.15f, 0.70f, 0.98f, 0.98f));
      draw.DrawFillRect(Vector2(tagPos.x, tagPos.y + tagSize.y - 5.0f),
                        Vector2(tagSize.x, 5.0f),
                        Vector4(0.25f, 0.98f, 0.70f, 1.0f));

      draw.DrawMSDFString("★ MAX DISTANCE No. 1 !! ★",
                          Vector2(tagPos.x + 12.0f, tagPos.y + 14.0f), 32.0f,
                          Vector4(1.0f, 1.0f, 1.0f, 1.0f), true,
                          Vector4(0.05f, 0.20f, 0.35f, 1.0f), 0.18f, 0.14f);

      draw.DrawFillRect(botPos, botSize, Vector4(0.06f, 0.14f, 0.22f, 0.98f));
      draw.DrawFillRect(botPos, Vector2(6.0f, botSize.y),
                        Vector4(0.20f, 0.90f, 0.70f, 1.0f));

      draw.DrawMSDFString("★ TOP RUNNER ACHIEVED ★",
                          Vector2(botPos.x + 25.0f, botPos.y + 13.0f), 32.0f,
                          Vector4(1.0f, 1.0f, 1.0f, 1.0f), true,
                          Vector4(0.15f, 0.70f, 0.98f, 1.0f), 0.16f, 0.12f);

      draw.DrawMSDFString("TURBO DASH // RECORD BREAKER",
                          Vector2(65.0f, 134.0f), 20.0f,
                          Vector4(0.40f, 0.95f, 0.85f, 0.95f), true,
                          Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.15f, 0.10f);

    } else {
      // --- ダブル1位（究極記録）用バナー演出 ---
      draw.DrawFillRect(tagPos, tagSize, Vector4(1.0f, 0.80f, 0.15f, 0.98f));
      draw.DrawFillRect(Vector2(tagPos.x, tagPos.y + tagSize.y - 5.0f),
                        Vector2(tagSize.x, 5.0f),
                        Vector4(1.0f, 0.30f, 0.75f, 1.0f));

      draw.DrawMSDFString("★ GRAND CHAMPION No. 1 !! ★",
                          Vector2(tagPos.x + 8.0f, tagPos.y + 14.0f), 30.0f,
                          Vector4(1.0f, 1.0f, 1.0f, 1.0f), true,
                          Vector4(0.35f, 0.10f, 0.25f, 1.0f), 0.18f, 0.14f);

      draw.DrawFillRect(botPos, botSize, Vector4(0.18f, 0.05f, 0.25f, 0.98f));
      draw.DrawFillRect(botPos, Vector2(6.0f, botSize.y),
                        Vector4(1.0f, 0.80f, 0.15f, 1.0f));

      draw.DrawMSDFString("★ DUAL RECORD ACHIEVED ★",
                          Vector2(botPos.x + 25.0f, botPos.y + 13.0f), 32.0f,
                          Vector4(1.0f, 0.95f, 0.35f, 1.0f), true,
                          Vector4(0.95f, 0.25f, 0.70f, 1.0f), 0.16f, 0.12f);

      draw.DrawMSDFString("GRAND VICTORY // LEGENDARY RUN",
                          Vector2(65.0f, 134.0f), 20.0f,
                          Vector4(1.0f, 0.85f, 0.3f, 0.95f), true,
                          Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.15f, 0.10f);
    }

    // スキップガイド
    if (timer_ >= 0.45f) {
      draw.DrawMSDFString("[SPACE / A-BUTTON] SKIP", Vector2(530.0f, 680.0f),
                          17.0f, Vector4(0.85f, 0.85f, 0.85f, 0.75f), true,
                          Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.12f);
    }
  }

  // 8. 開幕フラッシュ
  if (timer_ < kFlashDuration) {
    float flashAlpha = 1.0f - (timer_ / kFlashDuration);
    draw.DrawFillRect(Vector2(0.0f, 0.0f), fullScreenSize,
                      Vector4(1.0f, 0.98f, 0.90f, flashAlpha));
  }

  // 9. 切断エフェクト
  if (state_ == State::SlashExit) {
    float exitT =
        (timer_ - kExitStartTimer) / (totalDuration_ - kExitStartTimer);
    float slashH = 12.0f + 60.0f * exitT;
    float slashAlpha = (1.0f - exitT);
    draw.DrawFillRect(Vector2(0.0f, 360.0f - slashH * 0.5f),
                      Vector2(1280.0f, slashH),
                      Vector4(1.0f, 1.0f, 1.0f, slashAlpha));
  }
}

void RankingCutinEffect::ImGui() {
#ifdef _USE_IMGUI
  if (ImGui::TreeNode("ペンギン1位カットイン演出設定")) {
    if (ImGui::Button("スコア1位カットイン再生", ImVec2(180, 32))) {
      Trigger(CutinType::Score);
    }
    ImGui::SameLine();
    if (ImGui::Button("走行距離1位カットイン再生", ImVec2(180, 32))) {
      Trigger(CutinType::Distance);
    }
    ImGui::SameLine();
    if (ImGui::Button("ダブル1位カットイン再生", ImVec2(180, 32))) {
      Trigger(CutinType::Double);
    }
    ImGui::SameLine();
    if (ImGui::Button("リセット", ImVec2(80, 32))) {
      Reset();
    }

    const char *typeStr =
        (currentType_ == CutinType::Score)      ? "スコア1位 (Score)"
        : (currentType_ == CutinType::Distance) ? "走行距離1位 (Distance)"
                                                : "ダブル1位 (Double Champion)";
    ImGui::Text("現在の演出タイプ: %s", typeStr);

    ImGui::Text("状態: %s (Timer: %.2f / %.2f)",
                (state_ == State::Inactive)        ? "Inactive"
                : (state_ == State::FlashAndSlash) ? "FlashAndSlash"
                : (state_ == State::BannerHold)    ? "BannerHold"
                : (state_ == State::SlashExit)     ? "SlashExit"
                                                   : "Finished",
                timer_, totalDuration_);

    ImGui::DragFloat2("帯サイズ", &bannerSize_.x, 2.0f, 200.0f, 1920.0f);
    ImGui::DragFloat2("中心座標", &bannerCenterPos_.x, 2.0f, 0.0f, 1280.0f);
    ImGui::DragFloat("傾き(rad)", &bannerAngle_, 0.005f, -0.3f, 0.3f);
    ImGui::DragFloat("総演出時間(秒)", &totalDuration_, 0.05f, 0.5f, 5.0f);

    ImGui::TreePop();
  }
#endif
}


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
// イージング関数
float EaseOutExpo(float t) {
  return (t >= 1.0f) ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);
}

float EaseOutBack(float t, float s = 1.70158f) {
  float f = t - 1.0f;
  return f * f * ((s + 1.0f) * f + s) + 1.0f;
}
} // namespace

void RankingCutinEffect::Initialize(Texture *texture) {
  if (!texture)
    return;

  // ペンギンカットイン画像テクスチャの読み込み
  cutinTextureHandle_ = texture->CreateTexture("Resources/Texture/RankingCutin.png");
  if (cutinTextureHandle_ < 0) {
    cutinTextureHandle_ = texture->CreateTexture("Resources/Texture/RankingCutin.jpg");
  }

  // 単色スプライト用の共通白テクスチャ
  whiteTextureHandle_ = texture->CreateTexture("Resources/Texture/white64x64.png");

  // メインカットインスプライトの初期化
  cutinSpriteData_.transform.scale = {1.0f, 1.0f, 1.0f};
  cutinSpriteData_.transform.translate = {bannerCenterPos_.x, bannerCenterPos_.y, 0.0f};
  cutinSpriteData_.transform.rotate = {0.0f, 0.0f, bannerAngle_};
  cutinSpriteData_.size = bannerSize_;
  cutinSpriteData_.pivot = {0.5f, 0.5f};
  cutinSpriteData_.textureArea[0] = {0.0f, 0.0f};
  cutinSpriteData_.textureArea[1] = {1.0f, 1.0f};
  cutinSpriteData_.scaleMode = SpriteScaleMode::Fit;
  cutinSpriteData_.anchor = SpriteAnchor::None;

  cutinSprite_ = std::make_unique<Sprite>();
  cutinSprite_->Initialize(cutinSpriteData_, cutinTextureHandle_);
  cutinSprite_->GetMaterial()->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
  cutinSprite_->SettingWvp();

  // 上下スラッシュ帯（黒・赤斜めボーダー）
  auto createSlantBar = [&](int texHandle, const Vector2 &size, const Vector4 &color) {
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

  // 上下の黒赤ボーダーバー（帯の上下をシャープに引き締める）
  topBarSprite_ = createSlantBar(whiteTextureHandle_, {1500.0f, 28.0f}, {0.88f, 0.06f, 0.12f, 0.95f});
  bottomBarSprite_ = createSlantBar(whiteTextureHandle_, {1500.0f, 28.0f}, {0.05f, 0.05f, 0.07f, 0.95f});
  accentLineSprite_ = createSlantBar(whiteTextureHandle_, {1500.0f, 6.0f}, {1.0f, 0.92f, 0.15f, 1.0f});
  eyeGlintSprite_ = createSlantBar(whiteTextureHandle_, {36.0f, 36.0f}, {1.0f, 1.0f, 0.8f, 1.0f});
}

void RankingCutinEffect::Trigger() {
  state_ = State::FlashAndSlash;
  timer_ = 0.0f;
  screenShake_ = 14.0f;
  isSoundPlayedImpact_ = false;
  isSoundPlayedHold_ = false;
  isSoundPlayedExit_ = false;

  SpawnShards();

  // 開幕インパクト音（反射/カウンター音 + 爆発破壊音）
  auto sound = SoundManager::GetInstance();
  if (sound) {
    sound->PlaySE(SoundManager::SE::BossReflect, 1.0f);
    sound->PlaySE(SoundManager::SE::ClearBomb, 0.85f);
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
  shards_.reserve(24);

  for (int i = 0; i < 20; ++i) {
    ShardParticle s;
    s.pos = {
        640.0f + static_cast<float>((std::rand() % 900) - 450),
        360.0f + static_cast<float>((std::rand() % 360) - 180)};
    float speed = 120.0f + static_cast<float>(std::rand() % 300);
    float angle = -0.4f + static_cast<float>(std::rand() % 100) * 0.008f;
    s.velocity = {-speed * std::cos(angle), -speed * std::sin(angle)};
    s.rotation = static_cast<float>(std::rand() % 314) * 0.02f;
    s.rotSpeed = (std::rand() % 2 == 0 ? 1.0f : -1.0f) * (2.0f + static_cast<float>(std::rand() % 4));
    float sz = 12.0f + static_cast<float>(std::rand() % 24);
    s.size = {sz, sz * (0.4f + static_cast<float>(std::rand() % 10) * 0.06f)};
    s.life = 0.0f;
    s.maxLife = 1.2f + static_cast<float>(std::rand() % 10) * 0.08f;

    int colType = std::rand() % 3;
    if (colType == 0) {
      s.color = {0.92f, 0.08f, 0.14f, 0.95f};
    } else if (colType == 1) {
      s.color = {1.0f, 0.92f, 0.20f, 0.95f};
    } else {
      s.color = {0.05f, 0.05f, 0.08f, 0.90f};
    }
    shards_.push_back(s);
  }
}

void RankingCutinEffect::UpdateShards(float dt) {
  for (auto &s : shards_) {
    s.life += dt;
    s.pos.x += s.velocity.x * dt;
    s.pos.y += s.velocity.y * dt;
    s.rotation += s.rotSpeed * dt;
  }
}

void RankingCutinEffect::Update(float dt) {
  if (!IsActive())
    return;

  timer_ += dt;
  screenShake_ = (std::max)(0.0f, screenShake_ - dt * 28.0f);
  UpdateShards(dt);

  // タイムライン制御
  if (timer_ < kSlideInDuration) {
    state_ = State::FlashAndSlash;
  } else if (timer_ < kExitStartTimer) {
    state_ = State::BannerHold;
    if (!isSoundPlayedHold_) {
      isSoundPlayedHold_ = true;
      // 帯が激突・静止した瞬間の重低音
      auto sound = SoundManager::GetInstance();
      if (sound) {
        sound->PlaySE(SoundManager::SE::BossHit, 0.9f);
      }
    }
  } else if (timer_ < totalDuration_) {
    state_ = State::SlashExit;
    if (!isSoundPlayedExit_) {
      isSoundPlayedExit_ = true;
      // 画面を切り裂いてリザルトへ移行する破砕音
      auto sound = SoundManager::GetInstance();
      if (sound) {
        sound->PlaySE(SoundManager::SE::BarrierBreak, 1.0f);
        sound->PlaySE(SoundManager::SE::Start, 0.8f);
      }
    }
  } else {
    state_ = State::Finished;
  }

  // スプライトの動的トランスフォーム更新
  if (!cutinSprite_)
    return;

  Vector2 curPos = bannerCenterPos_;
  Vector2 curScale = {1.0f, 1.0f};
  float curAngle = bannerAngle_;

  if (state_ == State::FlashAndSlash) {
    // 画面右外から爆速でスライドイン（EaseOutExpo）
    float t = std::clamp(timer_ / kSlideInDuration, 0.0f, 1.0f);
    float ease = EaseOutExpo(t);
    curPos.x = 1850.0f + (bannerCenterPos_.x - 1850.0f) * ease;
    float s = 1.35f - 0.35f * ease;
    curScale = {s, s};
  } else if (state_ == State::BannerHold) {
    // 中央で構え、カメラが極めてゆっくりと迫る
    float t = std::clamp((timer_ - kSlideInDuration) / (kExitStartTimer - kSlideInDuration), 0.0f, 1.0f);
    curPos.x = bannerCenterPos_.x - 15.0f * t;
    float s = 1.0f + 0.04f * t;
    curScale = {s, s};
  } else if (state_ == State::SlashExit) {
    // 切り裂かれて左方向へ超高速で抜ける
    float t = std::clamp((timer_ - kExitStartTimer) / (totalDuration_ - kExitStartTimer), 0.0f, 1.0f);
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
  cutinSprite_->SetTransform(ct);
  cutinSprite_->SettingWvp();

  // 上下スラッシュボーダーの同期配置
  if (topBarSprite_ && bottomBarSprite_ && accentLineSprite_) {
    // 上帯
    Transform tt = ct;
    tt.translate.y -= (bannerSize_.y * 0.5f * curScale.y + 12.0f);
    topBarSprite_->SetTransform(tt);
    topBarSprite_->SettingWvp();

    // 下帯
    Transform bt = ct;
    bt.translate.y += (bannerSize_.y * 0.5f * curScale.y + 12.0f);
    bottomBarSprite_->SetTransform(bt);
    bottomBarSprite_->SettingWvp();

    // アクセントイエローライン
    Transform at = ct;
    at.translate.y += (bannerSize_.y * 0.5f * curScale.y);
    accentLineSprite_->SetTransform(at);
    accentLineSprite_->SettingWvp();
  }

  // ゴーグル／瞳のキラーン（星型スパークル）
  if (eyeGlintSprite_) {
    if (timer_ >= kEyeGlintStart && timer_ <= (kEyeGlintStart + kEyeGlintDuration)) {
      float gt = (timer_ - kEyeGlintStart) / kEyeGlintDuration;
      float glintScale = (gt < 0.35f) ? (gt / 0.35f) * 1.5f : (1.0f - (gt - 0.35f) / 0.65f) * 1.5f;
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
  draw.DrawFillRect(Vector2(0.0f, 0.0f), fullScreenSize, Vector4(0.04f, 0.02f, 0.03f, bgDimAlpha));

  // 2. スピードライン／集中線の背景装飾
  if (state_ == State::FlashAndSlash || state_ == State::BannerHold) {
    for (int i = 0; i < 7; ++i) {
      float y = 60.0f + i * 95.0f + std::sin(timer_ * 12.0f + i) * 6.0f;
      float h = (i % 2 == 0) ? 6.0f : 14.0f;
      Vector4 col = (i % 3 == 0) ? Vector4(0.85f, 0.05f, 0.1f, 0.35f)
                                 : Vector4(0.0f, 0.0f, 0.0f, 0.45f);
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
  if (cutinSprite_) {
    draw.DrawSprite(cutinSprite_.get());
  }

  // 5. 破片（シャード）パーティクルの描画
  for (const auto &s : shards_) {
    if (s.life < s.maxLife) {
      float alpha = 1.0f - (s.life / s.maxLife);
      Vector4 c = s.color;
      c.w *= alpha;
      draw.DrawFillRect(s.pos, s.size, c);
    }
  }

  // 6. 瞳・ゴーグルの輝き（スパークル）描画
  if (eyeGlintSprite_ && timer_ >= kEyeGlintStart && timer_ <= (kEyeGlintStart + kEyeGlintDuration)) {
    draw.DrawSprite(eyeGlintSprite_.get());
  }

  // 7. 極太スラントタイポグラフィ（斜めバナー＆文字）
  if (state_ == State::BannerHold || state_ == State::FlashAndSlash) {
    // --- 左上バナー: RANKING No. 1 ---
    Vector2 tagPos(50.0f, 62.0f);
    Vector2 tagSize(430.0f, 64.0f);
    // 影
    draw.DrawFillRect(Vector2(tagPos.x + 6.0f, tagPos.y + 6.0f), tagSize, Vector4(0.0f, 0.0f, 0.0f, 0.85f));
    // 本体
    draw.DrawFillRect(tagPos, tagSize, Vector4(0.92f, 0.06f, 0.12f, 0.98f));
    // 金色フチ
    draw.DrawFillRect(Vector2(tagPos.x, tagPos.y + tagSize.y - 5.0f), Vector2(tagSize.x, 5.0f),
                      Vector4(1.0f, 0.92f, 0.20f, 1.0f));

    draw.DrawMSDFString("★ RANKING No. 1 !! ★", Vector2(tagPos.x + 22.0f, tagPos.y + 14.0f),
                       36.0f, Vector4(1.0f, 0.95f, 0.3f, 1.0f), true,
                       Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.18f, 0.14f);

    // --- 右下バナー: NEW RECORD ACHIEVED! ---
    Vector2 botPos(760.0f, 595.0f);
    Vector2 botSize(470.0f, 60.0f);
    draw.DrawFillRect(Vector2(botPos.x + 5.0f, botPos.y + 5.0f), botSize, Vector4(0.0f, 0.0f, 0.0f, 0.85f));
    draw.DrawFillRect(botPos, botSize, Vector4(0.06f, 0.06f, 0.08f, 0.98f));
    draw.DrawFillRect(botPos, Vector2(6.0f, botSize.y), Vector4(0.92f, 0.06f, 0.12f, 1.0f));

    draw.DrawMSDFString("★ TOP RECORD ACHIEVED ★", Vector2(botPos.x + 25.0f, botPos.y + 13.0f),
                       32.0f, Vector4(1.0f, 1.0f, 1.0f, 1.0f), true,
                       Vector4(0.92f, 0.06f, 0.12f, 1.0f), 0.16f, 0.12f);

    // --- コミック調サブタイトル ---
    draw.DrawMSDFString("ALL-OUT DASH // CRITICAL FINISH", Vector2(65.0f, 134.0f),
                       20.0f, Vector4(1.0f, 0.9f, 0.2f, 0.9f), true,
                       Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.15f, 0.10f);

    // スキップガイド
    if (timer_ >= 0.45f) {
      draw.DrawMSDFString("[SPACE / A-BUTTON] SKIP", Vector2(530.0f, 680.0f), 17.0f,
                         Vector4(0.8f, 0.8f, 0.8f, 0.7f), true, Vector4(0.0f, 0.0f, 0.0f, 1.0f), 0.12f);
    }
  }

  // 8. 開幕フラッシュ
  if (timer_ < kFlashDuration) {
    float flashAlpha = 1.0f - (timer_ / kFlashDuration);
    draw.DrawFillRect(Vector2(0.0f, 0.0f), fullScreenSize, Vector4(1.0f, 0.98f, 0.85f, flashAlpha));
  }

  // 9. 切断エフェクト
  if (state_ == State::SlashExit) {
    float exitT = (timer_ - kExitStartTimer) / (totalDuration_ - kExitStartTimer);
    float slashH = 12.0f + 60.0f * exitT;
    float slashAlpha = (1.0f - exitT);
    draw.DrawFillRect(Vector2(0.0f, 360.0f - slashH * 0.5f), Vector2(1280.0f, slashH),
                      Vector4(1.0f, 1.0f, 1.0f, slashAlpha));
  }
}

void RankingCutinEffect::ImGui() {
#ifdef _USE_IMGUI
  if (ImGui::TreeNode("ペンギン1位カットイン演出設定")) {
    if (ImGui::Button("カットイン演出テスト再生", ImVec2(220, 36))) {
      Trigger();
    }
    ImGui::SameLine();
    if (ImGui::Button("リセット", ImVec2(100, 36))) {
      Reset();
    }

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

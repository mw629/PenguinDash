#include "Fade.h"
#include <Graphics/Render/Draw.h>
#include <algorithm>
#include <cmath>

// コンストラクタ
Fade::Fade() {
    Initialize();
}

// フェードの初期化
void Fade::Initialize() {
    status_ = Status::None;
    lastFinishedStatus_ = Status::None;
    color_ = kDefaultFadeColor;
    currentAlpha_ = 0.0f;
    timer_ = 0.0f;
    duration_ = kDefaultDuration;
    isFinished_ = false;
}

// フェードアウト開始 (デフォルト設定)
void Fade::StartFadeOut() {
    StartFadeOut(kDefaultDuration, kDefaultFadeColor);
}

// 時間を指定してフェードアウト開始
void Fade::StartFadeOut(float duration) {
    StartFadeOut(duration, kDefaultFadeColor);
}

// 時間とカラーを指定してフェードアウト開始
void Fade::StartFadeOut(float duration, const Vector4& color) {
    status_ = Status::FadeOut;
    lastFinishedStatus_ = Status::None;
    color_ = color;
    duration_ = (duration <= 0.0f) ? 0.0001f : duration;
    timer_ = 0.0f;
    currentAlpha_ = 0.0f;
    isFinished_ = false;
}

// フェードイン開始 (デフォルト設定)
void Fade::StartFadeIn() {
    StartFadeIn(kDefaultDuration, kDefaultFadeColor);
}

// 時間を指定してフェードイン開始
void Fade::StartFadeIn(float duration) {
    StartFadeIn(duration, kDefaultFadeColor);
}

// 時間とカラーを指定してフェードイン開始
void Fade::StartFadeIn(float duration, const Vector4& color) {
    status_ = Status::FadeIn;
    lastFinishedStatus_ = Status::None;
    color_ = color;
    duration_ = (duration <= 0.0f) ? 0.0001f : duration;
    timer_ = 0.0f;
    currentAlpha_ = 1.0f;
    isFinished_ = false;
}

// 毎フレームの更新処理
void Fade::Update() {
    Update(kDefaultDeltaTime);
}

// デルタ時間を指定して更新処理
void Fade::Update(float deltaTime) {
    if (status_ == Status::None) {
        return;
    }

    timer_ += deltaTime;
    float progress = std::clamp(timer_ / duration_, 0.0f, 1.0f);
    // スムーズステップによる滑らかなイージング (3x^2 - 2x^3)
    float ease = progress * progress * (3.0f - 2.0f * progress);

    if (status_ == Status::FadeOut) {
        currentAlpha_ = ease;
        if (progress >= 1.0f) {
            currentAlpha_ = 1.0f;
            lastFinishedStatus_ = Status::FadeOut;
            status_ = Status::None;
            isFinished_ = true;
        }
    } else if (status_ == Status::FadeIn) {
        currentAlpha_ = 1.0f - ease;
        if (progress >= 1.0f) {
            currentAlpha_ = 0.0f;
            lastFinishedStatus_ = Status::FadeIn;
            status_ = Status::None;
            isFinished_ = true;
        }
    }
}

#include <Core/WindowConfig.h>

// 全画面フェードオーバーレイの描画処理
void Fade::Draw(class Draw& draw) {
    if (currentAlpha_ > 0.001f) {
        Vector4 drawColor = color_;
        drawColor.w = currentAlpha_;
        draw.DrawFillRect(Vector2(0.0f, 0.0f),
            Vector2(static_cast<float>(WindowConfig::kDefaultClientWidth),
                    static_cast<float>(WindowConfig::kDefaultClientHeight)),
            drawColor);
    }
}

// フェード状態のリセット
void Fade::Reset() {
    Initialize();
}

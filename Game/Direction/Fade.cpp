#include "Fade.h"
#include <Graphics/Render/Draw.h>
#include <algorithm>
#include <cmath>

Fade::Fade() {
    Initialize();
}

void Fade::Initialize() {
    status_ = Status::None;
    lastFinishedStatus_ = Status::None;
    color_ = kDefaultFadeColor;
    currentAlpha_ = 0.0f;
    timer_ = 0.0f;
    duration_ = kDefaultDuration;
    isFinished_ = false;
}

void Fade::StartFadeOut() {
    StartFadeOut(kDefaultDuration, kDefaultFadeColor);
}

void Fade::StartFadeOut(float duration) {
    StartFadeOut(duration, kDefaultFadeColor);
}

void Fade::StartFadeOut(float duration, const Vector4& color) {
    status_ = Status::FadeOut;
    lastFinishedStatus_ = Status::None;
    color_ = color;
    duration_ = (duration <= 0.0f) ? 0.0001f : duration;
    timer_ = 0.0f;
    currentAlpha_ = 0.0f;
    isFinished_ = false;
}

void Fade::StartFadeIn() {
    StartFadeIn(kDefaultDuration, kDefaultFadeColor);
}

void Fade::StartFadeIn(float duration) {
    StartFadeIn(duration, kDefaultFadeColor);
}

void Fade::StartFadeIn(float duration, const Vector4& color) {
    status_ = Status::FadeIn;
    lastFinishedStatus_ = Status::None;
    color_ = color;
    duration_ = (duration <= 0.0f) ? 0.0001f : duration;
    timer_ = 0.0f;
    currentAlpha_ = 1.0f;
    isFinished_ = false;
}

void Fade::Update() {
    Update(kDefaultDeltaTime);
}

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

void Fade::Reset() {
    Initialize();
}

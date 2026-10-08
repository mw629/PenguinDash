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
    color_ = {0.0f, 0.0f, 0.0f, 1.0f};
    currentAlpha_ = 0.0f;
    timer_ = 0.0f;
    duration_ = 0.5f;
    isFinished_ = false;
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

void Fade::StartFadeIn(float duration, const Vector4& color) {
    status_ = Status::FadeIn;
    lastFinishedStatus_ = Status::None;
    color_ = color;
    duration_ = (duration <= 0.0f) ? 0.0001f : duration;
    timer_ = 0.0f;
    currentAlpha_ = 1.0f;
    isFinished_ = false;
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

void Fade::Draw(class Draw& draw) {
    if (currentAlpha_ > 0.001f) {
        Vector4 drawColor = color_;
        drawColor.w = currentAlpha_;
        draw.DrawFillRect(Vector2(0.0f, 0.0f), Vector2(1280.0f, 720.0f), drawColor);
    }
}

void Fade::Reset() {
    Initialize();
}

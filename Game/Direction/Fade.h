#pragma once
#include <VariableTypes.h>

class Draw;

class Fade {
public:
    enum class Status {
        None,    // フェード停止中（透明）
        FadeIn,  // 暗転から明転（アルファ 1 -> 0）
        FadeOut, // 明転から暗転（アルファ 0 -> 1）
    };

    Fade();
    ~Fade() = default;

    // 初期化
    void Initialize();

    // フェードアウト開始 (画面が徐々に指定色で暗転していく)
    void StartFadeOut(float duration = 0.5f, const Vector4& color = {0.0f, 0.0f, 0.0f, 1.0f});

    // フェードイン開始 (画面の暗転が徐々に明転していく)
    void StartFadeIn(float duration = 0.5f, const Vector4& color = {0.0f, 0.0f, 0.0f, 1.0f});

    // 更新処理
    void Update(float deltaTime = 1.0f / 60.0f);

    // 描画処理
    void Draw(class Draw& draw);

    // 状態取得
    Status GetStatus() const { return status_; }
    bool IsFading() const { return status_ != Status::None; }
    bool IsFinished() const { return isFinished_; }
    bool IsFadeOutFinished() const { return isFinished_ && lastFinishedStatus_ == Status::FadeOut; }
    bool IsFadeInFinished() const { return isFinished_ && lastFinishedStatus_ == Status::FadeIn; }
    float GetAlpha() const { return currentAlpha_; }

    // リセット（フェード強制終了）
    void Reset();

private:
    Status status_ = Status::None;
    Status lastFinishedStatus_ = Status::None;
    Vector4 color_ = {0.0f, 0.0f, 0.0f, 1.0f};
    float currentAlpha_ = 0.0f;
    float timer_ = 0.0f;
    float duration_ = 0.5f;
    bool isFinished_ = false;
};

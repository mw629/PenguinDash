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

    /// <summary>
    /// コンストラクタ
    /// </summary>
    Fade();

    /// <summary>
    /// デストラクタ
    /// </summary>
    ~Fade() = default;

    /// <summary>
    /// フェードの初期化
    /// </summary>
    void Initialize();

    /// <summary>
    /// フェードアウト開始 (画面が徐々に指定色で暗転していく)
    /// </summary>
    void StartFadeOut();

    /// <summary>
    /// 時間を指定してフェードアウト開始
    /// </summary>
    /// <param name="duration">フェード所要時間 (秒)</param>
    void StartFadeOut(float duration);

    /// <summary>
    /// 時間とカラーを指定してフェードアウト開始
    /// </summary>
    /// <param name="duration">フェード所要時間 (秒)</param>
    /// <param name="color">フェード色</param>
    void StartFadeOut(float duration, const Vector4& color);

    /// <summary>
    /// フェードイン開始 (画面の暗転が徐々に明転していく)
    /// </summary>
    void StartFadeIn();

    /// <summary>
    /// 時間を指定してフェードイン開始
    /// </summary>
    /// <param name="duration">フェード所要時間 (秒)</param>
    void StartFadeIn(float duration);

    /// <summary>
    /// 時間とカラーを指定してフェードイン開始
    /// </summary>
    /// <param name="duration">フェード所要時間 (秒)</param>
    /// <param name="color">フェード色</param>
    void StartFadeIn(float duration, const Vector4& color);

    /// <summary>
    /// 毎フレームの更新処理
    /// </summary>
    void Update();

    /// <summary>
    /// デルタ時間を指定して更新処理
    /// </summary>
    /// <param name="deltaTime">フレーム経過時間</param>
    void Update(float deltaTime);

    /// <summary>
    /// 全画面フェードオーバーレイの描画処理
    /// </summary>
    /// <param name="draw">描画マネージャ</param>
    void Draw(class Draw& draw);

    /// <summary>
    /// 現在のフェード状態を取得
    /// </summary>
    /// <returns>Status列挙値</returns>
    Status GetStatus() const { return status_; }

    /// <summary>
    /// フェード中かどうかを取得
    /// </summary>
    /// <returns>フェード中の場合true</returns>
    bool IsFading() const { return status_ != Status::None; }

    /// <summary>
    /// フェードが完了したかどうかを取得
    /// </summary>
    /// <returns>完了した場合true</returns>
    bool IsFinished() const { return isFinished_; }

    /// <summary>
    /// フェードアウトが完了した瞬間かどうかを取得
    /// </summary>
    /// <returns>完了した場合true</returns>
    bool IsFadeOutFinished() const { return isFinished_ && lastFinishedStatus_ == Status::FadeOut; }

    /// <summary>
    /// フェードインが完了した瞬間かどうかを取得
    /// </summary>
    /// <returns>完了した場合true</returns>
    bool IsFadeInFinished() const { return isFinished_ && lastFinishedStatus_ == Status::FadeIn; }

    /// <summary>
    /// 現在のフェードアルファ値を取得 (0.0:透明 〜 1.0:不透明)
    /// </summary>
    /// <returns>アルファ値</returns>
    float GetAlpha() const { return currentAlpha_; }

    /// <summary>
    /// フェード状態のリセット（強制停止）
    /// </summary>
    void Reset();

private:
    static constexpr float kDefaultDeltaTime = 1.0f / 60.0f;
    static constexpr float kDefaultDuration = 0.5f;
    static constexpr Vector4 kDefaultFadeColor = Color::Black;

    Status status_ = Status::None;
    Status lastFinishedStatus_ = Status::None;
    Vector4 color_ = kDefaultFadeColor;
    float currentAlpha_ = 0.0f;
    float timer_ = 0.0f;
    float duration_ = kDefaultDuration;
    bool isFinished_ = false;
};

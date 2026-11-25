#pragma once
#include <VariableTypes.h>
#include <Math/Calculation.h>
#include <memory>
#include <vector>
#include <functional>
#include <Graphics/Render/PostEffect.h>
#include <Texture.h>

class Draw;

class FreezeTransition {
public:
    enum class Phase {
        None,       // 停止中
        Freezing,   // 画面全体が凍結侵食してくる (Freeze Shader)
        Cracking,   // 凍結完了、ピキピキッと亀裂が走る (Shatter Shader Crack phase, 中間処理実行)
        Shattering, // パリーン！と氷が割れて無数のボロノイ破片が四散 (Shatter Shader Voronoi Fracture)
        Finished    // 遷移完了
    };

    /// <summary>
    /// シングルトンインスタンスを取得
    /// </summary>
    /// <returns>FreezeTransitionインスタンスへのポインタ</returns>
    static FreezeTransition* GetInstance();

    /// <summary>
    /// コンストラクタ
    /// </summary>
    FreezeTransition();

    /// <summary>
    /// デストラクタ
    /// </summary>
    ~FreezeTransition() = default;

    /// <summary>
    /// 初期化 (PostEffect・シェーダーパラメータ設定)
    /// </summary>
    void Initialize();

    /// <summary>
    /// 凍結・破砕画面遷移演出を開始
    /// </summary>
    /// <param name="freezeDuration">凍結にかかる時間 (秒)</param>
    /// <param name="shatterDuration">破片が飛び散る時間 (秒)</param>
    /// <param name="onMidpoint">凍結完了・割れる瞬間に呼ばれるコールバック</param>
    void Start(float freezeDuration, float shatterDuration, std::function<void()> onMidpoint = nullptr);

    /// <summary>
    /// デフォルト時間で凍結・破砕画面遷移演出を開始
    /// </summary>
    /// <param name="onMidpoint">凍結完了・割れる瞬間に呼ばれるコールバック</param>
    void Start(std::function<void()> onMidpoint = nullptr) { Start(kDefaultFreezeDuration, kDefaultShatterDuration, onMidpoint); }

    /// <summary>
    /// デルタ時間を指定して毎フレームの更新処理
    /// </summary>
    /// <param name="deltaTime">フレーム経過時間</param>
    void Update(float deltaTime);

    /// <summary>
    /// 毎フレームの更新処理 (デフォルト60fps想定)
    /// </summary>
    void Update() { Update(kDefaultDeltaTime); }

    /// <summary>
    /// ポストエフェクトおよび破片パーティクルの描画処理
    /// </summary>
    /// <param name="draw">描画マネージャ</param>
    void Draw(class Draw& draw);

    /// <summary>
    /// 現在の遷移フェーズを取得
    /// </summary>
    /// <returns>Phase列挙値</returns>
    Phase GetPhase() const { return phase_; }

    /// <summary>
    /// 演出が実行中かどうかを取得
    /// </summary>
    /// <returns>実行中の場合true</returns>
    bool IsActive() const { return phase_ != Phase::None && phase_ != Phase::Finished; }

    /// <summary>
    /// 演出が完全に終了したかどうかを取得
    /// </summary>
    /// <returns>終了した場合true</returns>
    bool IsFinished() const { return phase_ == Phase::Finished; }

    /// <summary>
    /// 凍結侵食中かどうかを取得
    /// </summary>
    /// <returns>凍結中の場合true</returns>
    bool IsFreezing() const { return phase_ == Phase::Freezing; }

    /// <summary>
    /// 氷が破砕した後の状態かどうかを取得
    /// </summary>
    /// <returns>破砕後または終了時の場合true</returns>
    bool IsShattered() const { return phase_ == Phase::Shattering || phase_ == Phase::Finished; }

    /// <summary>
    /// 演出の強制リセット
    /// </summary>
    void Reset();

private:
    static constexpr float kDefaultDeltaTime = 1.0f / 60.0f;
    static constexpr float kDefaultFreezeDuration = 0.85f;
    static constexpr float kDefaultCrackDuration = 0.50f;
    static constexpr float kDefaultShatterDuration = 0.95f;
    static constexpr Vector2 kDefaultScreenCenter = { 640.0f, 360.0f };
    static constexpr float kFreezeSparkleRadius = 450.0f;

    struct IceSparkle {
        Vector2 pos;
        Vector2 velocity;
        float size = 3.0f;
        float alpha = 1.0f;
        float life = 0.0f;
        float maxLife = 0.5f;
        Vector4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
    };

    Phase phase_ = Phase::None;
    float timer_ = 0.0f;
    float freezeDuration_ = kDefaultFreezeDuration;
    float crackDuration_ = kDefaultCrackDuration;
    float shatterDuration_ = kDefaultShatterDuration;

    std::function<void()> onMidpointCallback_ = nullptr;
    bool midpointExecuted_ = false;

    // テクスチャ管理
    std::unique_ptr<Texture> textureManager_;
    int texFrostHandle_ = -1;
    int texCrackHandle_ = -1;
    int texSurfaceHandle_ = -1;

    // PostEffect シェーダーコントローラー
    std::unique_ptr<PostEffect> freezePostEffect_;
    std::unique_ptr<PostEffect> shatterPostEffect_;

    // キラキラ光る氷粉・ガラスダストパーティクル
    std::vector<IceSparkle> sparkles_;

    bool isInitialized_ = false;

    void EmitSparkles(const Vector2& center, int count, float minSpeed, float maxSpeed);
    void EmitGlassCascade(int count);
    void UpdateSparkles(float deltaTime);
};

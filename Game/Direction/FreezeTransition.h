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

    static FreezeTransition* GetInstance();

    FreezeTransition();
    ~FreezeTransition() = default;

    // 初期化 (PostEffect・シェーダーパラメータ設定)
    void Initialize();

    // トランジション開始
    // freezeDuration: 凍結にかかる時間 (秒)
    // shatterDuration: 破片が飛び散る時間 (秒)
    // onMidpoint: 凍結完了・割れる瞬間に呼ばれるコールバック (シーン初期化やステート切り替え等)
    void Start(float freezeDuration, float shatterDuration, std::function<void()> onMidpoint = nullptr);
    void Start(std::function<void()> onMidpoint = nullptr) { Start(kDefaultFreezeDuration, kDefaultShatterDuration, onMidpoint); }

    // 毎フレームの更新処理
    void Update(float deltaTime);
    void Update() { Update(kDefaultDeltaTime); }

    // 描画処理 (DrawUIから呼び出し: PostEffect Shaderによる描画)
    void Draw(class Draw& draw);

    // 状態取得
    Phase GetPhase() const { return phase_; }
    bool IsActive() const { return phase_ != Phase::None && phase_ != Phase::Finished; }
    bool IsFinished() const { return phase_ == Phase::Finished; }
    bool IsFreezing() const { return phase_ == Phase::Freezing; }
    bool IsShattered() const { return phase_ == Phase::Shattering || phase_ == Phase::Finished; }

    // 強制リセット
    void Reset();

private:
    static constexpr float kDefaultDeltaTime = 1.0f / 60.0f;
    static constexpr float kDefaultFreezeDuration = 0.85f;
    static constexpr float kDefaultCrackDuration = 0.14f;
    static constexpr float kDefaultShatterDuration = 0.85f;
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
    void UpdateSparkles(float deltaTime);
};

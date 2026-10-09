#pragma once
#include <VariableTypes.h>
#include <Math/Calculation.h>
#include <memory>
#include <vector>
#include <functional>
#include <Sprite.h>
#include <Texture.h>

class Draw;

class FreezeTransition {
public:
    enum class Phase {
        None,       // 停止中
        Freezing,   // 画面の周りから凍結が侵食してくる
        Cracking,   // 凍結完了、ピキピキッと亀裂が走る (中間処理実行)
        Shattering, // パリーン！と氷が割れて無数の破片が飛び散る
        Finished    // 遷移完了
    };

    static FreezeTransition* GetInstance();

    FreezeTransition();
    ~FreezeTransition() = default;

    // 初期化 (スプライト・テクスチャ・破片メッシュの事前構築)
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

    // 描画処理 (DrawUIから呼び出し)
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
    static constexpr float kDefaultFreezeDuration = 0.75f;
    static constexpr float kDefaultCrackDuration = 0.12f;
    static constexpr float kDefaultShatterDuration = 0.75f;
    static constexpr float kShardGravity = 1150.0f;
    static constexpr float kShardAirResistance = 0.985f;
    static constexpr float kSpecularExponent = 5.0f;
    static constexpr float kSpecularFlashStrength = 0.55f;
    static constexpr Vector2 kDefaultScreenCenter = { 640.0f, 360.0f };
    static constexpr float kFreezeSparkleRadius = 450.0f;

    static constexpr Vector2 kCenterPivot = { 0.5f, 0.5f };
    static constexpr Vector4 kClearWhiteColor = { 1.0f, 1.0f, 1.0f, 0.0f };
    static constexpr Vector4 kFrostColor2 = { 0.9f, 0.95f, 1.0f, 0.0f };
    static constexpr Vector4 kSurfaceColor = { 0.85f, 0.95f, 1.0f, 0.0f };

    struct Shard {
        std::unique_ptr<Sprite> sprite;
        Vector2 initialPos;
        Vector2 currentPos;
        Vector2 velocity;
        Vector3 rotation{ 0.0f, 0.0f, 0.0f };       // 3D回転 (X, Y, Z)
        Vector3 angularVelocity{ 0.0f, 0.0f, 0.0f };// 3D回転速度 (X, Y, Z)
        float scale = 1.0f;
        float alpha = 1.0f;
        Vector4 baseColor{ 0.92f, 0.97f, 1.0f, 1.0f }; // ガラス基本色
        float shineSpeed = 1.0f;
        float mass = 1.0f;
    };

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
    float shatterFlashTimer_ = 0.0f; // 破砕瞬間の閃光フラッシュタイマー

    std::function<void()> onMidpointCallback_ = nullptr;
    bool midpointExecuted_ = false;

    // テクスチャ管理
    std::unique_ptr<Texture> textureManager_;
    int texFrostHandle_ = -1;
    int texCrackHandle_ = -1;
    int texSurfaceHandle_ = -1;

    // スプライト
    std::unique_ptr<Sprite> frostSprite1_; // 画面外周から侵食する霜 (正位置)
    std::unique_ptr<Sprite> frostSprite2_; // 画面外周から侵食する霜 (180度回転・高密度化)
    std::unique_ptr<Sprite> surfaceSprite_; // 画面全体を覆う氷の板
    std::unique_ptr<Sprite> crackSprite_;   // ひび割れ (クラック)

    // ガラス破片 (シャード)
    static constexpr int kMeshCols = 14;
    static constexpr int kMeshRows = 9;
    std::vector<Shard> shards_;

    // キラキラ光る氷粉・ガラスダストパーティクル
    std::vector<IceSparkle> sparkles_;

    bool isInitialized_ = false;

    void SetupShards();
    void ResetShards();
    void EmitSparkles(const Vector2& center, int count, float minSpeed, float maxSpeed);
    void UpdateSparkles(float deltaTime);
};

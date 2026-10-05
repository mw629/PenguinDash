#pragma once
#include <VariableTypes.h>
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
    void Start(float freezeDuration = 0.75f, float shatterDuration = 0.75f, std::function<void()> onMidpoint = nullptr);

    // 毎フレームの更新処理
    void Update(float deltaTime = 1.0f / 60.0f);

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
    struct Shard {
        std::unique_ptr<Sprite> sprite;
        Vector2 initialPos;
        Vector2 currentPos;
        Vector2 velocity;
        float rotation = 0.0f;
        float angularVelocity = 0.0f;
        float scale = 1.0f;
        Vector2 size;
        float alpha = 1.0f;
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
    float freezeDuration_ = 0.75f;
    float crackDuration_ = 0.12f;
    float shatterDuration_ = 0.75f;

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

    // 破片 (シャード)
    static constexpr int kGridCols = 12;
    static constexpr int kGridRows = 8;
    static constexpr int kTotalShards = kGridCols * kGridRows;
    std::vector<Shard> shards_;

    // キラキラ光る氷粉パーティクル
    std::vector<IceSparkle> sparkles_;

    bool isInitialized_ = false;

    void SetupShards();
    void ResetShards();
    void EmitSparkles(const Vector2& center, int count, float minSpeed, float maxSpeed);
    void UpdateSparkles(float deltaTime);
};

#pragma once
#include <string>
#include <functional>
#include <memory>

/// @brief 実績の判定に必要なゲーム情報コンテキスト
struct AchievementContext {
    float distance = 0.0f;           // 走行距離 (m)
    float score = 0.0f;              // 現在のスコア
    int bonusEnemyHitCount = 0;      // ボーナス敵撃破数
    float playTime = 0.0f;           // プレイ時間 (秒)
    bool isBossBattle = false;       // ボス戦中か
    bool isBossDefeated = false;     // ボスを撃破したか
    bool isOneLaneMode = false;      // 1レーン（高速）モード中か
    int jumpCount = 0;              // ジャンプ回数
    int rollCount = 0;              // スライディング（ロール）回数
    int laneChangeCount = 0;        // レーン移動回数
};

/// @brief 実績基底クラス
/// 新しい実績をクラス継承で定義したい場合は、このクラスを継承して CheckCondition などを実装します。
class Achievement {
public:
    Achievement(const std::string& id,
                const std::string& title,
                const std::string& description,
                float scoreReward);
    virtual ~Achievement() = default;

    /// @brief 条件チェック（毎フレーム呼ばれ、条件達成で true を返す）
    virtual bool CheckCondition(const AchievementContext& ctx) = 0;

    /// @brief 進捗率 (0.0f ～ 1.0f)
    virtual float GetProgress(const AchievementContext& /*ctx*/) const {
        return isUnlocked_ ? 1.0f : 0.0f;
    }

    /// @brief 進捗テキスト (例: "50 / 100 m", "達成済み" など)
    virtual std::string GetProgressText(const AchievementContext& /*ctx*/) const {
        return isUnlocked_ ? "達成" : "未達成";
    }

    // ゲッター
    const std::string& GetId() const { return id_; }
    const std::string& GetTitle() const { return title_; }
    const std::string& GetDescription() const { return description_; }
    float GetScoreReward() const { return scoreReward_; }
    bool IsUnlocked() const { return isUnlocked_; }

    // 操作
    virtual void Unlock() { isUnlocked_ = true; }
    virtual void Reset() { isUnlocked_ = false; }

protected:
    std::string id_;            // 実績の一意識別子 (例: "dist_100")
    std::string title_;         // 実績のタイトル (例: "初めの一歩")
    std::string description_;   // 実績の説明文 (例: "走行距離100mを突破する")
    float scoreReward_ = 0.0f;  // 解除時に加算されるスコアボーナス
    bool isUnlocked_ = false;   // 解除済みフラグ
};

// --------------------------------------------------------------------------------------
// 汎用派生クラス：関数・ラムダ式で簡単に作成できる実績
// --------------------------------------------------------------------------------------
using AchievementConditionFunc = std::function<bool(const AchievementContext&)>;
using AchievementProgressFunc = std::function<float(const AchievementContext&)>;
using AchievementProgressTextFunc = std::function<std::string(const AchievementContext&)>;

class ConditionAchievement : public Achievement {
public:
    ConditionAchievement(
        const std::string& id,
        const std::string& title,
        const std::string& description,
        float scoreReward,
        AchievementConditionFunc conditionFunc,
        AchievementProgressFunc progressFunc = nullptr,
        AchievementProgressTextFunc progressTextFunc = nullptr);

    bool CheckCondition(const AchievementContext& ctx) override;
    float GetProgress(const AchievementContext& ctx) const override;
    std::string GetProgressText(const AchievementContext& ctx) const override;

private:
    AchievementConditionFunc conditionFunc_;
    AchievementProgressFunc progressFunc_;
    AchievementProgressTextFunc progressTextFunc_;
};

// --------------------------------------------------------------------------------------
// 汎用派生クラス：数値の目標達成（距離〇〇m、撃破〇〇体など）に特化した進捗型実績
// --------------------------------------------------------------------------------------
using ValueGetterFunc = std::function<float(const AchievementContext&)>;

class ProgressAchievement : public Achievement {
public:
    ProgressAchievement(
        const std::string& id,
        const std::string& title,
        const std::string& description,
        float scoreReward,
        float targetValue,
        ValueGetterFunc valueGetter,
        const std::string& unit = "");

    bool CheckCondition(const AchievementContext& ctx) override;
    float GetProgress(const AchievementContext& ctx) const override;
    std::string GetProgressText(const AchievementContext& ctx) const override;

    float GetTargetValue() const { return targetValue_; }

private:
    float targetValue_ = 0.0f;
    ValueGetterFunc valueGetter_;
    std::string unit_;
};

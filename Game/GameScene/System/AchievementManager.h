#pragma once
#include "Achievement.h"
#include <vector>
#include <memory>
#include <deque>
#include <string>

class ScoreManager;
class Draw;

/// @brief 実績解除時の画面通知情報
struct AchievementNotification {
    std::string id;
    std::string title;
    std::string description;
    float scoreReward = 0.0f;
    float timer = 0.0f;             // 経過時間 (秒)
    static constexpr float kDuration = 3.5f;     // 表示継続時間
    static constexpr float kFadeDuration = 0.35f; // フェード時間
};

/// @brief 実績マネージャークラス（基盤クラス）
/// ゲーム内の実績登録・監視判定・スコア加点・演出通知・デバッグUIを一括管理します。
/// 実績を新しく増やしたい場合は、Initialize 内の RegisterDefaultAchievements または
/// RegisterProgress / RegisterCondition / AddAchievement で自由に追加できます。
class AchievementManager {
public:
    AchievementManager();
    ~AchievementManager() = default;

    /// @brief 初期化（初期実績セットを登録）
    void Initialize();

    /// @brief 実績の追加（独自派生クラスを直接渡す場合）
    Achievement* AddAchievement(std::unique_ptr<Achievement> achievement);

    /// @brief 手軽な実績追加ヘルパー：目標値達成型（距離、撃破数、スコアなど）
    /// @param id 一意なID (例: "dist_100")
    /// @param title 実績名 (例: "初めの一歩")
    /// @param description 説明文 (例: "走行距離100mを突破する")
    /// @param scoreReward 解除時に加点されるスコア (例: 500.0f)
    /// @param targetValue 達成に必要な目標値 (例: 100.0f)
    /// @param valueGetter 現在の判定値を取得する関数 (例: [](const auto& c){ return c.distance; })
    /// @param unit 単位表示 (例: "m", "体", "pt")
    Achievement* RegisterProgress(
        const std::string& id,
        const std::string& title,
        const std::string& description,
        float scoreReward,
        float targetValue,
        ValueGetterFunc valueGetter,
        const std::string& unit = "");

    /// @brief 手軽な実績追加ヘルパー：自由条件判定型（ラムダ式による条件チェック）
    /// @param id 一意なID (例: "boss_clear")
    /// @param title 実績名 (例: "氷原の支配者")
    /// @param description 説明文 (例: "ボスを撃破する")
    /// @param scoreReward 解除時に加点されるスコア (例: 3000.0f)
    /// @param conditionFunc 条件判定関数 (true を返すと解除)
    /// @param progressFunc 任意の進捗率計算関数 (0.0f〜1.0f)
    /// @param progressTextFunc 任意の進捗文字列取得関数
    Achievement* RegisterCondition(
        const std::string& id,
        const std::string& title,
        const std::string& description,
        float scoreReward,
        AchievementConditionFunc conditionFunc,
        AchievementProgressFunc progressFunc = nullptr,
        AchievementProgressTextFunc progressTextFunc = nullptr);

    /// @brief 毎フレームの更新（未達成実績の条件チェック、スコア加点、通知タイマー更新）
    void Update(float deltaTime, const AchievementContext& ctx, ScoreManager* scoreManager);

    /// @brief ゲーム開始時またはリトライ時のリセット
    void Reset();

    /// @brief 手動で実績を解除（イベント通知ドリブンやデバッグ用途）
    bool Unlock(const std::string& id, ScoreManager* scoreManager = nullptr);

    /// @brief 実績IDから検索
    Achievement* FindAchievement(const std::string& id) const;

    // ゲッター
    const std::vector<std::unique_ptr<Achievement>>& GetAchievements() const { return achievements_; }
    int GetUnlockedCount() const;
    int GetTotalCount() const { return static_cast<int>(achievements_.size()); }
    float GetTotalScoreEarned() const { return totalScoreEarned_; }

    /// @brief 実績解除のポップアップ通知を描画
    void DrawUI(Draw& draw);

    /// @brief ImGuiによる実績一覧・進捗表示・手動テスト用UI
    void ImGui(const AchievementContext& ctx, ScoreManager* scoreManager = nullptr);

private:
    /// @brief 実績を定義・追加するメソッド（ここにどんどん実績を追加していきます）
    void RegisterDefaultAchievements();

    /// @brief 実績解除時の共通トリガー処理
    void TriggerUnlock(Achievement* ach, ScoreManager* scoreManager);

private:
    std::vector<std::unique_ptr<Achievement>> achievements_;
    std::deque<AchievementNotification> notifications_;
    float totalScoreEarned_ = 0.0f;
};

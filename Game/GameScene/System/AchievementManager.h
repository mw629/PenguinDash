#pragma once
#include "Achievement.h"
#include <vector>
#include <memory>
#include <deque>
#include <string>

class ScoreManager;
class Draw;

/// <summary>
/// 実績解除時の画面通知情報
/// </summary>
struct AchievementNotification {
    std::string id;
    std::string title;
    std::string description;
    float scoreReward = 0.0f;
    float timer = 0.0f;             // 経過時間 (秒)
    static constexpr float kDuration = 3.5f;     // 表示継続時間
    static constexpr float kFadeDuration = 0.35f; // フェード時間
};

/// <summary>
/// 実績マネージャークラス（基盤クラス）
/// ゲーム内の実績登録・監視判定・スコア加点・演出通知・デバッグUIを一括管理します。
/// 実績を新しく増やしたい場合は、Initialize 内の RegisterDefaultAchievements または
/// RegisterProgress / RegisterCondition / AddAchievement で自由に追加できます。
/// </summary>
class AchievementManager {
public:
    AchievementManager();
    ~AchievementManager() = default;

    /// <summary>
    /// 初期化（初期実績セットを登録）
    /// </summary>
    void Initialize();

    /// <summary>
    /// 実績の追加（独自派生クラスを直接渡す場合）
    /// </summary>
    Achievement* AddAchievement(std::unique_ptr<Achievement> achievement);

    /// <summary>
    /// 手軽な実績追加ヘルパー：目標値達成型（距離、撃破数、スコアなど）
    /// </summary>
    /// <param name="id">一意なID (例: "dist_100")</param>
    /// <param name="title">実績名 (例: "初めの一歩")</param>
    /// <param name="description">説明文 (例: "走行距離100mを突破する")</param>
    /// <param name="scoreReward">解除時に加点されるスコア (例: 500.0f)</param>
    /// <param name="targetValue">達成に必要な目標値 (例: 100.0f)</param>
    /// <param name="valueGetter">現在の判定値を取得する関数 (例: [](const auto& c){ return c.distance; })</param>
    /// <param name="unit">単位表示 (例: "m", "体", "pt")</param>
    Achievement* RegisterProgress(
        const std::string& id,
        const std::string& title,
        const std::string& description,
        float scoreReward,
        float targetValue,
        ValueGetterFunc valueGetter,
        const std::string& unit = "");

    /// <summary>
    /// 手軽な実績追加ヘルパー：自由条件判定型（ラムダ式による条件チェック）
    /// </summary>
    /// <param name="id">一意なID (例: "boss_clear")</param>
    /// <param name="title">実績名 (例: "氷原の支配者")</param>
    /// <param name="description">説明文 (例: "ボスを撃破する")</param>
    /// <param name="scoreReward">解除時に加点されるスコア (例: 3000.0f)</param>
    /// <param name="conditionFunc">条件判定関数 (true を返すと解除)</param>
    /// <param name="progressFunc">任意の進捗率計算関数 (0.0f〜1.0f)</param>
    /// <param name="progressTextFunc">任意の進捗文字列取得関数</param>
    Achievement* RegisterCondition(
        const std::string& id,
        const std::string& title,
        const std::string& description,
        float scoreReward,
        AchievementConditionFunc conditionFunc,
        AchievementProgressFunc progressFunc = nullptr,
        AchievementProgressTextFunc progressTextFunc = nullptr);

    /// <summary>
    /// 毎フレームの更新（未達成実績の条件チェック、スコア加点、通知タイマー更新）
    /// </summary>
    void Update(float deltaTime, const AchievementContext& ctx, ScoreManager* scoreManager);

    /// <summary>
    /// ゲーム開始時またはリトライ時のリセット
    /// </summary>
    void Reset();

    /// <summary>
    /// 手動で実績を解除（イベント通知ドリブンやデバッグ用途）
    /// </summary>
    bool Unlock(const std::string& id, ScoreManager* scoreManager = nullptr);

    /// <summary>
    /// 実績IDから検索
    /// </summary>
    Achievement* FindAchievement(const std::string& id) const;

    // ゲッター
    const std::vector<std::unique_ptr<Achievement>>& GetAchievements() const { return achievements_; }
    int GetUnlockedCount() const;
    int GetTotalCount() const { return static_cast<int>(achievements_.size()); }
    float GetTotalScoreEarned() const { return totalScoreEarned_; }

    /// <summary>
    /// 実績解除のポップアップ通知を描画
    /// </summary>
    void DrawUI(Draw& draw);

    /// <summary>
    /// ImGuiによる実績一覧・進捗表示・手動テスト用UI
    /// </summary>
    void ImGui(const AchievementContext& ctx, ScoreManager* scoreManager = nullptr);

private:
    /// <summary>
    /// 実績を定義・追加するメソッド（ここにどんどん実績を追加していきます）
    /// </summary>
    void RegisterDefaultAchievements();

    /// <summary>
    /// 実績解除時の共通トリガー処理
    /// </summary>
    void TriggerUnlock(Achievement* ach, ScoreManager* scoreManager);

private:
    std::vector<std::unique_ptr<Achievement>> achievements_;
    std::deque<AchievementNotification> notifications_;
    float totalScoreEarned_ = 0.0f;
};

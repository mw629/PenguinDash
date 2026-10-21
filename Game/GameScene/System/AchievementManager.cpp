#include "AchievementManager.h"
#include "ScoreManager.h"
#include "SoundManager.h"
#include <Draw.h>
#include <imgui.h>
#include <algorithm>
#include <cmath>

// コンストラクタ
AchievementManager::AchievementManager() = default;

// 初期化（初期実績セットを登録）
void AchievementManager::Initialize() {
    achievements_.clear();
    notifications_.clear();
    totalScoreEarned_ = 0.0f;

    RegisterDefaultAchievements();
}

// 実績の追加（独自派生クラスを直接渡す場合）
Achievement* AchievementManager::AddAchievement(std::unique_ptr<Achievement> achievement) {
    if (!achievement) return nullptr;
    Achievement* ptr = achievement.get();
    achievements_.push_back(std::move(achievement));
    return ptr;
}

// 手軽な実績追加ヘルパー：目標値達成型（距離、撃破数、スコアなど）
Achievement* AchievementManager::RegisterProgress(
    const std::string& id,
    const std::string& title,
    const std::string& description,
    float scoreReward,
    float targetValue,
    ValueGetterFunc valueGetter,
    const std::string& unit) {
    auto ach = std::make_unique<ProgressAchievement>(
        id, title, description, scoreReward, targetValue, std::move(valueGetter), unit);
    return AddAchievement(std::move(ach));
}

// 手軽な実績追加ヘルパー：自由条件判定型（ラムダ式による条件チェック）
Achievement* AchievementManager::RegisterCondition(
    const std::string& id,
    const std::string& title,
    const std::string& description,
    float scoreReward,
    AchievementConditionFunc conditionFunc,
    AchievementProgressFunc progressFunc,
    AchievementProgressTextFunc progressTextFunc) {
    auto ach = std::make_unique<ConditionAchievement>(
        id, title, description, scoreReward, std::move(conditionFunc),
        std::move(progressFunc), std::move(progressTextFunc));
    return AddAchievement(std::move(ach));
}

// 実績を定義・追加するメソッド（ここにどんどん実績を追加していきます）
void AchievementManager::RegisterDefaultAchievements() {
    // ==========================================================================================
    // 【実績の追加ガイド】
    // ここに実績をどんどん追加していくことができます！
    // 
    // ■ 目標値型（距離、スコア、撃破数など）:
    //   RegisterProgress("ID", "タイトル", "説明文", 加点スコア, 目標値,
    //                    [](const AchievementContext& ctx) { return ctx.xxx; }, "単位");
    //
    // ■ 条件判定型（フラグ、複合条件など）:
    //   RegisterCondition("ID", "タイトル", "説明文", 加点スコア,
    //                     [](const AchievementContext& ctx) { return ctx.xxx; });
    //
    // ■ 独自クラス型（複雑な状態を持つ実績など）:
    //   AddAchievement(std::make_unique<MyCustomAchievement>(...));
    // ==========================================================================================

    // --- 1. 走行距離系実績 ---
    RegisterProgress("dist_50", "ペンギン散歩", "走行距離50mを突破する", 200.0f, 50.0f,
                     [](const AchievementContext& ctx) { return ctx.distance; }, "m");

    RegisterProgress("dist_150", "俊足のランナー", "走行距離150mを突破する", 500.0f, 150.0f,
                     [](const AchievementContext& ctx) { return ctx.distance; }, "m");

    RegisterProgress("dist_300", "氷原の覇者", "走行距離300mを突破する", 1000.0f, 300.0f,
                     [](const AchievementContext& ctx) { return ctx.distance; }, "m");

    RegisterProgress("dist_500", "南極点到達！", "走行距離500mを突破する", 2000.0f, 500.0f,
                     [](const AchievementContext& ctx) { return ctx.distance; }, "m");

    // --- 2. ボーナス敵撃破系実績 ---
    RegisterProgress("bonus_1", "初撃破！", "ボーナス敵を1体吹っ飛ばす", 300.0f, 1.0f,
                     [](const AchievementContext& ctx) { return static_cast<float>(ctx.bonusEnemyHitCount); }, "体");

    RegisterProgress("bonus_3", "ボーナスコレクター", "ボーナス敵を累計3体吹っ飛ばす", 800.0f, 3.0f,
                     [](const AchievementContext& ctx) { return static_cast<float>(ctx.bonusEnemyHitCount); }, "体");

    RegisterProgress("bonus_5", "ボーナスハンター", "ボーナス敵を累計5体吹っ飛ばす", 1500.0f, 5.0f,
                     [](const AchievementContext& ctx) { return static_cast<float>(ctx.bonusEnemyHitCount); }, "体");

    // --- 3. スコア到達系実績 ---
    RegisterProgress("score_1000", "スコアスターター", "スコア1,000ptを突破する", 300.0f, 1000.0f,
                     [](const AchievementContext& ctx) { return ctx.score; }, "pt");

    RegisterProgress("score_3000", "ハイスコアラー", "スコア3,000ptを突破する", 1000.0f, 3000.0f,
                     [](const AchievementContext& ctx) { return ctx.score; }, "pt");

    RegisterProgress("score_5000", "スコアマスター", "スコア5,000ptを突破する", 2500.0f, 5000.0f,
                     [](const AchievementContext& ctx) { return ctx.score; }, "pt");

    // --- 4. アクション・特殊モード系実績 ---
    RegisterProgress("jump_10", "ジャンピングスター", "ジャンプを累計10回行う", 300.0f, 10.0f,
                     [](const AchievementContext& ctx) { return static_cast<float>(ctx.jumpCount); }, "回");

    RegisterProgress("roll_10", "スライディング職人", "スライディングを累計10回行う", 300.0f, 10.0f,
                     [](const AchievementContext& ctx) { return static_cast<float>(ctx.rollCount); }, "回");

    RegisterCondition("speed_zone", "トップスピード", "高速1レーンモードに突入する", 500.0f,
                      [](const AchievementContext& ctx) { return ctx.isOneLaneMode; });

    // --- 5. ボス戦系実績 ---
    RegisterCondition("boss_encounter", "氷雪の王と対峙", "ボス戦に突入する", 500.0f,
                      [](const AchievementContext& ctx) { return ctx.isBossBattle; });

    RegisterCondition("boss_defeat", "アイスキング討伐！", "ボスを撃破する", 3000.0f,
                      [](const AchievementContext& ctx) { return ctx.isBossDefeated; });
}

// 毎フレームの更新（未達成実績の条件チェック、スコア加点、通知タイマー更新）
void AchievementManager::Update(float deltaTime, const AchievementContext& ctx, ScoreManager* scoreManager) {
    // 1. 未達成の実績を判定
    for (auto& ach : achievements_) {
        if (!ach->IsUnlocked()) {
            if (ach->CheckCondition(ctx)) {
                TriggerUnlock(ach.get(), scoreManager);
            }
        }
    }

    // 2. 通知キューのタイマー更新
    for (auto it = notifications_.begin(); it != notifications_.end(); ) {
        it->timer += deltaTime;
        if (it->timer >= AchievementNotification::kDuration) {
            it = notifications_.erase(it);
        } else {
            ++it;
        }
    }
}

// 実績解除時の共通トリガー処理
void AchievementManager::TriggerUnlock(Achievement* ach, ScoreManager* scoreManager) {
    if (!ach || ach->IsUnlocked()) return;

    ach->Unlock();
    totalScoreEarned_ += ach->GetScoreReward();

    // スコアマネージャーへ実績ボーナス加点
    if (scoreManager && ach->GetScoreReward() > 0.0f) {
        scoreManager->AddAchievementScore(ach->GetScoreReward());
    }

    // 解除SEの再生
    SoundManager::GetInstance()->PlaySE(SoundManager::SE::ItemGet);

    // 通知ポップアップをキューに追加
    AchievementNotification notif;
    notif.id = ach->GetId();
    notif.title = ach->GetTitle();
    notif.description = ach->GetDescription();
    notif.scoreReward = ach->GetScoreReward();
    notif.timer = 0.0f;
    notifications_.push_back(notif);
}

// 手動で実績を解除（イベント通知ドリブンやデバッグ用途）
bool AchievementManager::Unlock(const std::string& id, ScoreManager* scoreManager) {
    Achievement* ach = FindAchievement(id);
    if (ach && !ach->IsUnlocked()) {
        TriggerUnlock(ach, scoreManager);
        return true;
    }
    return false;
}

// 実績IDから検索
Achievement* AchievementManager::FindAchievement(const std::string& id) const {
    for (const auto& ach : achievements_) {
        if (ach->GetId() == id) {
            return ach.get();
        }
    }
    return nullptr;
}

// UnlockedCount を取得
int AchievementManager::GetUnlockedCount() const {
    int count = 0;
    for (const auto& ach : achievements_) {
        if (ach->IsUnlocked()) {
            count++;
        }
    }
    return count;
}

// ゲーム開始時またはリトライ時のリセット
void AchievementManager::Reset() {
    for (auto& ach : achievements_) {
        ach->Reset();
    }
    notifications_.clear();
    totalScoreEarned_ = 0.0f;
}

// 実績解除のポップアップ通知を描画
void AchievementManager::DrawUI(Draw& draw) {
    if (notifications_.empty()) return;

    // 最新の通知を画面上部にアニメーション付きで表示
    const auto& notif = notifications_.front();

    // フェードイン / フェードアウトの計算
    float alpha = 1.0f;
    float offsetY = 0.0f;
    if (notif.timer < AchievementNotification::kFadeDuration) {
        float t = notif.timer / AchievementNotification::kFadeDuration;
        alpha = t;
        offsetY = (1.0f - t) * -20.0f; // 上からスライドイン
    } else if (notif.timer > AchievementNotification::kDuration - AchievementNotification::kFadeDuration) {
        float t = (AchievementNotification::kDuration - notif.timer) / AchievementNotification::kFadeDuration;
        alpha = t;
        offsetY = (1.0f - t) * -20.0f;
    }

    // パネル配置（画面中央上部）
    constexpr float kPanelWidth = 440.0f;
    constexpr float kPanelHeight = 65.0f;
    constexpr float kBaseX = (1280.0f - kPanelWidth) * 0.5f;
    constexpr float kBaseY = 25.0f;

    Vector2 panelPos = { kBaseX, kBaseY + offsetY };
    Vector2 panelSize = { kPanelWidth, kPanelHeight };

    // 背景（濃紺・半透明）
    draw.DrawFillRect(panelPos, panelSize, Vector4(0.04f, 0.08f, 0.16f, 0.90f * alpha));

    // 金色のアクセントボーダー（上部）
    draw.DrawFillRect(panelPos, Vector2(kPanelWidth, 3.0f), Vector4(1.0f, 0.85f, 0.25f, alpha));

    // 金色の外枠（下・左・右）
    draw.DrawFillRect(Vector2(panelPos.x, panelPos.y + kPanelHeight - 1.5f), Vector2(kPanelWidth, 1.5f), Vector4(0.6f, 0.5f, 0.15f, 0.6f * alpha));
    draw.DrawFillRect(panelPos, Vector2(1.5f, kPanelHeight), Vector4(0.6f, 0.5f, 0.15f, 0.6f * alpha));
    draw.DrawFillRect(Vector2(panelPos.x + kPanelWidth - 1.5f, panelPos.y), Vector2(1.5f, kPanelHeight), Vector4(0.6f, 0.5f, 0.15f, 0.6f * alpha));

    // テキスト1行目: ★ 実績解除！ [タイトル] (+500 pt) ★
    char titleBuf[128];
    if (notif.scoreReward > 0.0f) {
        snprintf(titleBuf, sizeof(titleBuf), "★ 実績解除！ [%s]  +%.0f pt", notif.title.c_str(), notif.scoreReward);
    } else {
        snprintf(titleBuf, sizeof(titleBuf), "★ 実績解除！ [%s]", notif.title.c_str());
    }

    draw.DrawMSDFString(
        titleBuf,
        Vector2(panelPos.x + 16.0f, panelPos.y + 12.0f),
        19.0f,
        Vector4(1.0f, 0.88f, 0.25f, alpha),
        true,
        Vector4(0.2f, 0.12f, 0.0f, 0.8f * alpha),
        0.08f);

    // テキスト2行目: 説明文
    draw.DrawMSDFString(
        notif.description.c_str(),
        Vector2(panelPos.x + 16.0f, panelPos.y + 38.0f),
        15.0f,
        Vector4(0.92f, 0.94f, 0.98f, 0.95f * alpha),
        true,
        Vector4(0.0f, 0.05f, 0.15f, 0.7f * alpha),
        0.08f);
}

// ImGuiによる実績一覧・進捗表示・手動テスト用UI
void AchievementManager::ImGui(const AchievementContext& ctx, ScoreManager* scoreManager) {
#ifdef _USE_IMGUI
    if (ImGui::CollapsingHeader("実績システム (Achievement System)", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("解除状況: %d / %d  (獲得ボーナス: +%.0f pt)",
                    GetUnlockedCount(), GetTotalCount(), totalScoreEarned_);

        if (ImGui::Button("全実績を解除 (デバッグ)")) {
            for (auto& ach : achievements_) {
                if (!ach->IsUnlocked()) {
                    TriggerUnlock(ach.get(), scoreManager);
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("全実績をリセット")) {
            Reset();
        }

        ImGui::Separator();

        for (size_t i = 0; i < achievements_.size(); ++i) {
            auto& ach = achievements_[i];
            ImGui::PushID(static_cast<int>(i));

            bool unlocked = ach->IsUnlocked();
            float progress = ach->GetProgress(ctx);
            std::string progText = ach->GetProgressText(ctx);

            if (unlocked) {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.3f, 1.0f), "[解除済]");
            } else {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "[未解除]");
            }
            ImGui::SameLine();
            ImGui::Text("%s (+%.0f pt) - %s", ach->GetTitle().c_str(), ach->GetScoreReward(), ach->GetDescription().c_str());

            // 進捗バー
            char overlayBuf[64];
            snprintf(overlayBuf, sizeof(overlayBuf), "%s", progText.c_str());
            ImGui::ProgressBar(progress, ImVec2(300.0f, 0.0f), overlayBuf);

            ImGui::SameLine();
            if (!unlocked) {
                if (ImGui::Button("解除")) {
                    TriggerUnlock(ach.get(), scoreManager);
                }
            } else {
                ImGui::TextDisabled("解除済み");
            }

            ImGui::PopID();
        }
    }
#endif
}

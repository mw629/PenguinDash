#include "AchievementManager.h"
#include "ScoreManager.h"
#include "SoundManager.h"
#include <Draw.h>
#include <algorithm>
#include <cmath>
#include <imgui.h>


AchievementManager::AchievementManager() = default;

void AchievementManager::Initialize() {
  achievements_.clear();
  notifications_.clear();
  totalScoreEarned_ = 0.0f;

  RegisterDefaultAchievements();
}

Achievement *
AchievementManager::AddAchievement(std::unique_ptr<Achievement> achievement) {
  if (!achievement)
    return nullptr;
  Achievement *ptr = achievement.get();
  achievements_.push_back(std::move(achievement));
  return ptr;
}

Achievement *AchievementManager::RegisterProgress(
    const std::string &id, const std::string &title,
    const std::string &description, float scoreReward, float targetValue,
    ValueGetterFunc valueGetter, const std::string &unit,
    AchievementRarity rarity) {
  auto ach = std::make_unique<ProgressAchievement>(
      id, title, description, scoreReward, targetValue, std::move(valueGetter),
      unit, rarity);
  return AddAchievement(std::move(ach));
}

Achievement *AchievementManager::RegisterCondition(
    const std::string &id, const std::string &title,
    const std::string &description, float scoreReward,
    AchievementConditionFunc conditionFunc, AchievementRarity rarity,
    AchievementProgressFunc progressFunc,
    AchievementProgressTextFunc progressTextFunc) {
  auto ach = std::make_unique<ConditionAchievement>(
      id, title, description, scoreReward, std::move(conditionFunc), rarity,
      std::move(progressFunc), std::move(progressTextFunc));
  return AddAchievement(std::move(ach));
}

void AchievementManager::RegisterDefaultAchievements() {
  // ==========================================================================================
  // 【実績の追加ガイド】
  // ここに実績をどんどん追加していくことができます！
  //
  // ■ 目標値型（距離、スコア、撃破数など）:
  //   RegisterProgress("ID", "タイトル", "説明文", 加点スコア, 目標値,
  //                    [](const AchievementContext& ctx) { return ctx.xxx; },
  //                    "単位", AchievementRarity::Common / Rare / SuperRare / Secret);
  //
  // ■ 条件判定型（フラグ、複合条件など）:
  //   RegisterCondition("ID", "タイトル", "説明文", 加点スコア,
  //                     [](const AchievementContext& ctx) { return ctx.xxx; },
  //                     AchievementRarity::Common / Rare / SuperRare / Secret);
  //
  // ※ AchievementRarity::Secret を指定した実績は、タイトル画面の実績一覧（モーダル）に
  //    表示されない隠し実績（シークレット）になります。解除時には特別な金枠で通知されます。
  //
  // ■ 独自クラス型（複雑な状態を持つ実績など）:
  //   AddAchievement(std::make_unique<MyCustomAchievement>(...));
  // ==========================================================================================

  // --- 1. 走行距離系実績 ---
  RegisterProgress(
      "dist_500", "ペンギン散歩", "走行距離500mを突破する", 200.0f, 500.0f,
      [](const AchievementContext &ctx) { return ctx.distance; }, "m",
      AchievementRarity::Common);

  RegisterProgress(
      "dist_1000", "俊足のランナー", "走行距離1000mを突破する", 500.0f, 1000.0f,
      [](const AchievementContext &ctx) { return ctx.distance; }, "m",
      AchievementRarity::Common);

  RegisterProgress(
      "dist_2000", "氷原の覇者", "走行距離2000mを突破する", 1000.0f, 2000.0f,
      [](const AchievementContext &ctx) { return ctx.distance; }, "m",
      AchievementRarity::Rare);

  RegisterProgress(
      "dist_5000", "南極点到達！", "走行距離5000mを突破する", 2000.0f, 5000.0f,
      [](const AchievementContext &ctx) { return ctx.distance; }, "m",
      AchievementRarity::SuperRare);

  // --- 2. ボーナス敵撃破系実績 ---
  RegisterProgress(
      "bonus_1", "初撃破！", "ボーナス敵を1体吹っ飛ばす", 300.0f, 1.0f,
      [](const AchievementContext &ctx) {
        return static_cast<float>(ctx.bonusEnemyHitCount);
      },
      "体", AchievementRarity::Common);

  RegisterProgress(
      "bonus_3", "ボーナスコレクター", "ボーナス敵を累計3体吹っ飛ばす", 800.0f,
      3.0f,
      [](const AchievementContext &ctx) {
        return static_cast<float>(ctx.bonusEnemyHitCount);
      },
      "体", AchievementRarity::Rare);

  RegisterProgress(
      "bonus_5", "ボーナスハンター", "ボーナス敵を累計5体吹っ飛ばす", 1500.0f,
      5.0f,
      [](const AchievementContext &ctx) {
        return static_cast<float>(ctx.bonusEnemyHitCount);
      },
      "体", AchievementRarity::SuperRare);

  RegisterProgress(
      "bonus_10", "雪だるまバスター", "ボーナス敵を累計10体吹っ飛ばす", 2500.0f,
      10.0f,
      [](const AchievementContext &ctx) {
        return static_cast<float>(ctx.bonusEnemyHitCount);
      },
      "体", AchievementRarity::SuperRare);

  // --- 3. スコア到達系実績 ---
  RegisterProgress(
      "score_1000", "スコアスターター", "スコア1,000ptを突破する", 300.0f,
      1000.0f, [](const AchievementContext &ctx) { return ctx.score; }, "pt",
      AchievementRarity::Common);

  RegisterProgress(
      "score_3000", "ハイスコアラー", "スコア3,000ptを突破する", 1000.0f,
      3000.0f, [](const AchievementContext &ctx) { return ctx.score; }, "pt",
      AchievementRarity::Rare);

  RegisterProgress(
      "score_5000", "スコアマスター", "スコア5,000ptを突破する", 2500.0f,
      5000.0f, [](const AchievementContext &ctx) { return ctx.score; }, "pt",
      AchievementRarity::SuperRare);

  RegisterProgress(
      "score_10000", "レジェンドペンギン", "スコア10,000ptを突破する", 3000.0f,
      10000.0f, [](const AchievementContext &ctx) { return ctx.score; }, "pt",
      AchievementRarity::SuperRare);

  // --- 4. アクション・特殊モード系実績 ---
  RegisterProgress(
      "jump_10", "ジャンピングスター", "ジャンプを累計10回行う", 300.0f, 10.0f,
      [](const AchievementContext &ctx) {
        return static_cast<float>(ctx.jumpCount);
      },
      "回", AchievementRarity::Common);

  RegisterProgress(
      "jump_30", "空飛ぶペンギン", "ジャンプを累計30回行う", 800.0f, 30.0f,
      [](const AchievementContext &ctx) {
        return static_cast<float>(ctx.jumpCount);
      },
      "回", AchievementRarity::Rare);

  RegisterProgress(
      "roll_10", "スライディング職人", "スライディングを累計10回行う", 300.0f,
      10.0f,
      [](const AchievementContext &ctx) {
        return static_cast<float>(ctx.rollCount);
      },
      "回", AchievementRarity::Common);

  RegisterProgress(
      "roll_30", "氷上の曲芸師", "スライディングを累計30回行う", 800.0f, 30.0f,
      [](const AchievementContext &ctx) {
        return static_cast<float>(ctx.rollCount);
      },
      "回", AchievementRarity::Rare);

  RegisterProgress(
      "lane_15", "華麗なるステップ", "レーン移動を累計15回行う", 300.0f, 15.0f,
      [](const AchievementContext &ctx) {
        return static_cast<float>(ctx.laneChangeCount);
      },
      "回", AchievementRarity::Common);

  RegisterProgress(
      "lane_40", "氷上のダンサー", "レーン移動を累計40回行う", 800.0f, 40.0f,
      [](const AchievementContext &ctx) {
        return static_cast<float>(ctx.laneChangeCount);
      },
      "回", AchievementRarity::Rare);

  RegisterCondition(
      "speed_zone", "トップスピード", "1レーンモードに突入する", 500.0f,
      [](const AchievementContext &ctx) { return ctx.isOneLaneMode; },
      AchievementRarity::Rare);

  RegisterProgress(
      "speed_zone_clear", "疾風の突破者", "1レーンモードを完走する",
      1000.0f, 1.0f,
      [](const AchievementContext &ctx) {
        return static_cast<float>(ctx.oneLaneClearCount);
      },
      "回", AchievementRarity::Rare);

  /*RegisterCondition(
      "first_person", "臨場感マックス", "一人称視点に切り替えて走る", 300.0f,
      [](const AchievementContext &ctx) { return ctx.isFirstPerson; },
      AchievementRarity::Common);*/

  RegisterProgress(
      "unmoved_3s", "不動の精神", "3秒間動かずに留まる", 500.0f, 3.0f,
      [](const AchievementContext &ctx) { return ctx.unMovedTime; }, "秒",
      AchievementRarity::Rare);

  RegisterProgress(
      "unmoved_5s", "石像の境地", "5秒間動かずに留まる", 1000.0f, 5.0f,
      [](const AchievementContext &ctx) { return ctx.unMovedTime; }, "秒",
      AchievementRarity::SuperRare);

  // --- 5. アイテム・サバイバル系実績 ---
  RegisterProgress(
      "barrier_block_1", "九死に一生", "バリアで障害物の激突を1回防ぐ", 400.0f,
      1.0f,
      [](const AchievementContext &ctx) {
        return static_cast<float>(ctx.barrierBlockCount);
      },
      "回", AchievementRarity::Common);

  RegisterProgress(
      "barrier_block_3", "鉄壁のペンギン", "バリアで障害物の激突を累計3回防ぐ",
      1200.0f, 3.0f,
      [](const AchievementContext &ctx) {
        return static_cast<float>(ctx.barrierBlockCount);
      },
      "回", AchievementRarity::Rare);

  RegisterProgress(
      "clear_bomb_1", "一掃ボマー", "ボムアイテムで障害物を一掃する", 400.0f,
      1.0f,
      [](const AchievementContext &ctx) {
        return static_cast<float>(ctx.clearBombCount);
      },
      "回", AchievementRarity::Common);

  RegisterProgress(
      "item_collector_5", "アイテムコレクター", "アイテムを累計5個取得する",
      800.0f, 5.0f,
      [](const AchievementContext &ctx) {
        return static_cast<float>(ctx.itemGetCount);
      },
      "個", AchievementRarity::Rare);

  RegisterProgress(
      "survive_60s", "氷原のサバイバー", "1回のプレイで60秒間生き残る", 500.0f,
      60.0f, [](const AchievementContext &ctx) { return ctx.playTime; }, "秒",
      AchievementRarity::Common);

  RegisterProgress(
      "survive_120s", "不屈の鉄人", "1回のプレイで120秒間生き残る", 1200.0f,
      120.0f, [](const AchievementContext &ctx) { return ctx.playTime; }, "秒",
      AchievementRarity::Rare);

  // --- 6. ボス戦系実績 ---
  RegisterCondition(
      "boss_encounter", "氷雪の王と対峙", "ボス戦に突入する", 500.0f,
      [](const AchievementContext &ctx) { return ctx.isBossBattle; },
      AchievementRarity::Rare);

  RegisterProgress(
      "boss_reflect_1", "カウンターストライク", "ボス戦で魚を1回跳ね返す",
      400.0f, 1.0f,
      [](const AchievementContext &ctx) {
        return static_cast<float>(ctx.bossReflectCount);
      },
      "回", AchievementRarity::Common);

  RegisterProgress(
      "boss_reflect_5", "反射の達人", "ボス戦で魚を累計5回跳ね返す", 1200.0f,
      5.0f,
      [](const AchievementContext &ctx) {
        return static_cast<float>(ctx.bossReflectCount);
      },
      "回", AchievementRarity::Rare);

  RegisterCondition(
      "boss_defeat", "アイスキング討伐！", "ボスを撃破する", 3000.0f,
      [](const AchievementContext &ctx) { return ctx.isBossDefeated; },
      AchievementRarity::SuperRare);

  // --- 7. シークレット実績（実績一覧には表示されない隠し実績） ---
  RegisterCondition(
      "secret_zenjin_mitou", "前人未踏", "走行距離ランキングで1位に到達する", 0.0f,
      [](const AchievementContext &ctx) { return ctx.isDistanceFirstPlace; },
      AchievementRarity::Secret);
}

void AchievementManager::Update(float deltaTime, const AchievementContext &ctx,
                                ScoreManager *scoreManager) {
  // 1. 未達成の実績を判定
  for (auto &ach : achievements_) {
    if (!ach->IsUnlocked()) {
      if (ach->CheckCondition(ctx)) {
        TriggerUnlock(ach.get(), scoreManager);
      }
    }
  }

  // 2. 通知キューのタイマー更新（現在表示中の先頭通知のみタイマーを進め、完了後に順次ポップ）
  if (!notifications_.empty()) {
    auto &currentNotif = notifications_.front();
    currentNotif.timer += deltaTime;
    if (currentNotif.timer >= AchievementNotification::kDuration) {
      notifications_.pop_front();
    }
  }
}

void AchievementManager::TriggerUnlock(Achievement *ach,
                                       ScoreManager *scoreManager) {
  if (!ach || ach->IsUnlocked())
    return;

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
  notif.rarity = ach->GetRarity();
  notif.timer = 0.0f;
  notifications_.push_back(notif);
}

bool AchievementManager::Unlock(const std::string &id,
                                ScoreManager *scoreManager) {
  Achievement *ach = FindAchievement(id);
  if (ach && !ach->IsUnlocked()) {
    TriggerUnlock(ach, scoreManager);
    return true;
  }
  return false;
}

Achievement *AchievementManager::FindAchievement(const std::string &id) const {
  for (const auto &ach : achievements_) {
    if (ach->GetId() == id) {
      return ach.get();
    }
  }
  return nullptr;
}

std::vector<Achievement *> AchievementManager::GetVisibleAchievements() const {
  std::vector<Achievement *> visible;
  visible.reserve(achievements_.size());
  for (const auto &ach : achievements_) {
    if (ach && ach->IsVisibleInList()) {
      visible.push_back(ach.get());
    }
  }
  return visible;
}

int AchievementManager::GetVisibleTotalCount() const {
  int count = 0;
  for (const auto &ach : achievements_) {
    if (ach && ach->IsVisibleInList()) {
      count++;
    }
  }
  return count;
}

int AchievementManager::GetUnlockedCount() const {
  int count = 0;
  for (const auto &ach : achievements_) {
    if (ach->IsUnlocked()) {
      count++;
    }
  }
  return count;
}

void AchievementManager::Reset() {
  for (auto &ach : achievements_) {
    ach->Reset();
  }
  notifications_.clear();
  totalScoreEarned_ = 0.0f;
}

void AchievementManager::DrawUI(Draw &draw) {
  if (notifications_.empty())
    return;

  // 最新の通知を画面右上（1レーン表示・ボスHPより下）にアニメーション付きで表示
  const auto &notif = notifications_.front();

  // フェードイン / フェードアウトの計算
  float alpha = 1.0f;
  float offsetY = 0.0f;
  if (notif.timer < AchievementNotification::kFadeDuration) {
    float t = notif.timer / AchievementNotification::kFadeDuration;
    alpha = t;
    offsetY = (1.0f - t) * -15.0f; // 上からスライドイン
  } else if (notif.timer > AchievementNotification::kDuration -
                               AchievementNotification::kFadeDuration) {
    float t = (AchievementNotification::kDuration - notif.timer) /
              AchievementNotification::kFadeDuration;
    alpha = t;
    offsetY = (1.0f - t) * -15.0f; // 上へスライドアウト
  }

  // パネル配置（画面右上：1レーン表示[下端61px]・ボスHPバー[下端106px]・反撃チャンス警告[下端156px]より下）
  constexpr float kPanelWidth = 440.0f;
  constexpr float kPanelHeight = 65.0f;
  constexpr float kBaseX = 1280.0f - kPanelWidth - 20.0f; // 右端から20pxマージン (x = 820.0f)
  constexpr float kBaseY = 165.0f;                         // 1レーン・ボスHP関連HUDより下の位置 (y = 165.0f)

  Vector2 panelPos = {kBaseX, kBaseY + offsetY};
  Vector2 panelSize = {kPanelWidth, kPanelHeight};

  // 背景（濃紺・半透明）
  draw.DrawFillRect(panelPos, panelSize,
                    Vector4(0.04f, 0.08f, 0.16f, 0.90f * alpha));

  // レア度に応じた枠の色（コモン: 白 / レア: 緑 / スーパーレア: 紫 / シークレット: 金）
  Vector4 borderMainColor = Vector4(1.0f, 1.0f, 1.0f, alpha);
  Vector4 borderSubColor = Vector4(0.90f, 0.92f, 0.95f, 0.85f * alpha);
  if (notif.rarity == AchievementRarity::Rare) {
    borderMainColor = Vector4(0.20f, 0.95f, 0.35f, alpha);
    borderSubColor = Vector4(0.18f, 0.85f, 0.30f, 0.85f * alpha);
  } else if (notif.rarity == AchievementRarity::SuperRare) {
    borderMainColor = Vector4(0.75f, 0.30f, 1.0f, alpha);
    borderSubColor = Vector4(0.68f, 0.25f, 0.95f, 0.85f * alpha);
  } else if (notif.rarity == AchievementRarity::Secret) {
    borderMainColor = Vector4(1.0f, 0.84f, 0.0f, alpha);         // ゴールド
    borderSubColor = Vector4(1.0f, 0.65f, 0.15f, 0.85f * alpha); // オレンジゴールド
  }

  // アクセントボーダー（上部）
  draw.DrawFillRect(panelPos, Vector2(kPanelWidth, 3.0f), borderMainColor);

  // 外枠（下・左・右）
  draw.DrawFillRect(Vector2(panelPos.x, panelPos.y + kPanelHeight - 2.0f),
                    Vector2(kPanelWidth, 2.0f), borderSubColor);
  draw.DrawFillRect(panelPos, Vector2(2.0f, kPanelHeight), borderSubColor);
  draw.DrawFillRect(Vector2(panelPos.x + kPanelWidth - 2.0f, panelPos.y),
                    Vector2(2.0f, kPanelHeight), borderSubColor);

  // テキスト1行目: ★ 実績解除！ [タイトル] (+500 pt) ★（白文字、シークレットは特別な冠表記）
  char titleBuf[128];
  const char *prefix = (notif.rarity == AchievementRarity::Secret) ? "★ シークレット実績解除！" : "★ 実績解除！";
  if (notif.scoreReward > 0.0f) {
    snprintf(titleBuf, sizeof(titleBuf), "%s [%s]  +%.0f pt",
             prefix, notif.title.c_str(), notif.scoreReward);
  } else {
    snprintf(titleBuf, sizeof(titleBuf), "%s [%s]",
             prefix, notif.title.c_str());
  }

  draw.DrawMSDFString(titleBuf, Vector2(panelPos.x + 16.0f, panelPos.y + 12.0f),
                      19.0f, Vector4(1.0f, 1.0f, 1.0f, alpha), true,
                      Vector4(0.0f, 0.05f, 0.15f, 0.85f * alpha), 0.08f);

  // テキスト2行目: 説明文（白文字）
  draw.DrawMSDFString(notif.description.c_str(),
                      Vector2(panelPos.x + 16.0f, panelPos.y + 38.0f), 15.0f,
                      Vector4(1.0f, 1.0f, 1.0f, 0.95f * alpha), true,
                      Vector4(0.0f, 0.05f, 0.15f, 0.75f * alpha), 0.08f);
}

void AchievementManager::ImGui(const AchievementContext &ctx,
                               ScoreManager *scoreManager) {
#ifdef _USE_IMGUI
  if (ImGui::CollapsingHeader("実績システム (Achievement System)",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Text("解除状況: %d / %d  (獲得ボーナス: +%.0f pt)",
                GetUnlockedCount(), GetTotalCount(), totalScoreEarned_);

    if (ImGui::Button("全実績を解除 (デバッグ)")) {
      for (auto &ach : achievements_) {
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

    // リアルタイムトラッキング情報
    if (ImGui::TreeNode("判定値モニター (動かない時間・各種進行度)")) {
      ImGui::TextColored(ImVec4(0.3f, 0.9f, 1.0f, 1.0f),
                         "★ 動かない時間 (未操作): %.2f 秒", ctx.unMovedTime);
      ImGui::Text("走行距離: %.1f m | スコア: %.0f pt | 敵撃破: %d 体",
                  ctx.distance, ctx.score, ctx.bonusEnemyHitCount);
      ImGui::Text("アクション累計: ジャンプ %d 回 / スライディング %d 回 / レーン移動 %d 回",
                  ctx.jumpCount, ctx.rollCount, ctx.laneChangeCount);
      ImGui::Text("プレイ時間: %.1f 秒 | 1レーン: %s (完走 %d 回) | ボス戦: %s",
                  ctx.playTime, ctx.isOneLaneMode ? "ON" : "OFF",
                  ctx.oneLaneClearCount, ctx.isBossBattle ? "ON" : "OFF");
      ImGui::Text("アイテム取得: %d 個 | バリア防御: %d 回 | ボム一掃: %d 回 | 魚反射: %d 回",
                  ctx.itemGetCount, ctx.barrierBlockCount, ctx.clearBombCount,
                  ctx.bossReflectCount);
      ImGui::Text("視点: %s | 走行距離1位: %s",
                  ctx.isFirstPerson ? "一人称" : "三人称",
                  ctx.isDistanceFirstPlace ? "★ 1位到達！" : "NO");
      ImGui::TreePop();
    }

    ImGui::Separator();

    for (size_t i = 0; i < achievements_.size(); ++i) {
      auto &ach = achievements_[i];
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

      ImVec4 rarityImColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
      if (ach->GetRarity() == AchievementRarity::Rare) {
        rarityImColor = ImVec4(0.2f, 0.95f, 0.35f, 1.0f);
      } else if (ach->GetRarity() == AchievementRarity::SuperRare) {
        rarityImColor = ImVec4(0.8f, 0.4f, 1.0f, 1.0f);
      } else if (ach->GetRarity() == AchievementRarity::Secret) {
        rarityImColor = ImVec4(1.0f, 0.85f, 0.2f, 1.0f);
      }
      ImGui::TextColored(rarityImColor, "[%s]", ach->GetRarityName());
      ImGui::SameLine();
      ImGui::Text("%s (+%.0f pt) - %s", ach->GetTitle().c_str(),
                  ach->GetScoreReward(), ach->GetDescription().c_str());

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

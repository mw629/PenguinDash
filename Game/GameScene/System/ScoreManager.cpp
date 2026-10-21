#include "ScoreManager.h"
#include "Network/Firebase/FirebaseRankingManager.h"
#include <imgui.h>

void ScoreManager::Reset() {
  currentDistance_ = 0.0f;
  currentScore_ = 0.0f;
  bonusEnemyHitCount_ = 0;
  achievementBonusScore_ = 0.0f;
  isOnlineScoreSent_ = false;
  lastDistanceRank_ = -1;
  lastScoreRank_ = -1;
}

void ScoreManager::Update(float scrollSpeed, float timeScale) {
  currentDistance_ += scrollSpeed * timeScale;
  currentScore_ = currentDistance_ +
                  (bonusEnemyHitCount_ * kBonusScoreMultiplier * scrollSpeed) +
                  achievementBonusScore_;
}

void ScoreManager::AddBonusEnemyHit() {
  bonusEnemyHitCount_++;
}

void ScoreManager::AddAchievementScore(float score) {
  achievementBonusScore_ += score;
}

void ScoreManager::RecordCurrentScores() {
  UpdateRanking();
  UpdateScoreRanking();
}

void ScoreManager::SetPlayerName(const std::string& name) {
  playerName_ = name.empty() ? "Player" : name;
  snprintf(inputPlayerName_, sizeof(inputPlayerName_), "%s", playerName_.c_str());
}

void ScoreManager::SendCurrentScoreToOnlineRanking(const std::string& playerName) {
  SetPlayerName(playerName);
  FirebaseRankingManager::GetInstance()->SendScoreAsync(
      playerName_,
      currentDistance_,
      currentScore_,
      [](bool success, const std::string& /*msg*/) {
        if (success) {
          FirebaseRankingManager::GetInstance()->FetchBothRankingsAsync(10);
        }
      });
}

void ScoreManager::SendOnlineScoreAndFetchRanking() {
  if (isOnlineScoreSent_) {
    return;
  }
  isOnlineScoreSent_ = true;

  // inputPlayerName_ の編集内容を反映
  if (inputPlayerName_[0] != '\0') {
    playerName_ = inputPlayerName_;
  }

  // まず現在のキャッシュを取得開始（または即座に表示）
  auto rankingMgr = FirebaseRankingManager::GetInstance();
  if (rankingMgr->IsConfigured()) {
    // スコアを送信し、送信完了時に最新のランキングを再取得 (スコア・距離両方)
    rankingMgr->SendScoreAsync(
        playerName_,
        currentDistance_,
        currentScore_,
        [](bool success, const std::string& /*msg*/) {
          if (success) {
            FirebaseRankingManager::GetInstance()->FetchBothRankingsAsync(10);
          }
        });

    // 送信待機中にも既存ランキングをすぐに画面に表示できるよう事前取得 (両方)
    rankingMgr->FetchBothRankingsAsync(10);
  }
}

void ScoreManager::UpdateRanking() {
  lastDistanceRank_ = kRankingCount + 1; // デフォルトは圏外
  for (int i = 0; i < kRankingCount; i++) {
    if (currentDistance_ > topRankings_[i]) {
      for (int j = kRankingCount - 1; j > i; j--) {
        topRankings_[j] = topRankings_[j - 1];
      }
      topRankings_[i] = currentDistance_;
      lastDistanceRank_ = i + 1;
      break;
    }
  }
}

void ScoreManager::UpdateScoreRanking() {
  lastScoreRank_ = kRankingCount + 1; // デフォルトは圏外
  for (int i = 0; i < kRankingCount; i++) {
    if (currentScore_ > topScoreRankings_[i]) {
      for (int j = kRankingCount - 1; j > i; j--) {
        topScoreRankings_[j] = topScoreRankings_[j - 1];
      }
      topScoreRankings_[i] = currentScore_;
      lastScoreRank_ = i + 1;
      break;
    }
  }
}

bool ScoreManager::IsScoreFirstPlace() const {
  if (currentScore_ <= 1.0f) {
    return false;
  }
  auto rankingMgr = FirebaseRankingManager::GetInstance();
  if (rankingMgr && rankingMgr->IsConfigured()) {
    const auto& scoreRanks = rankingMgr->GetCachedScoreRankings();
    if (!scoreRanks.empty()) {
      return (currentScore_ >= scoreRanks[0].score);
    }
  }
  return (lastScoreRank_ == 1);
}

bool ScoreManager::IsDistanceFirstPlace() const {
  if (currentDistance_ <= 1.0f) {
    return false;
  }
  auto rankingMgr = FirebaseRankingManager::GetInstance();
  if (rankingMgr && rankingMgr->IsConfigured()) {
    const auto& distRanks = rankingMgr->GetCachedDistanceRankings();
    if (!distRanks.empty()) {
      return (currentDistance_ >= distRanks[0].distance);
    }
  }
  return (lastDistanceRank_ == 1);
}

bool ScoreManager::IsFirstPlace() const {
  return IsScoreFirstPlace() || IsDistanceFirstPlace();
}

void ScoreManager::ImGui() {
#ifdef _USE_IMGUI
  ImGui::Text("現在の走行距離: %.2f m", currentDistance_);
  if (ImGui::TreeNode("走行距離 TOP 3 ランキング (ローカル)")) {
    for (int i = 0; i < kRankingCount; i++) {
      if (topRankings_[i] > 0.0f) {
        ImGui::Text("%d 位: %.2f m", i + 1, topRankings_[i]);
      } else {
        ImGui::Text("%d 位: ---", i + 1);
      }
    }
    ImGui::TreePop();
  }

  ImGui::Text("現在のスコア: %.0f (実績ボーナス: +%.0f pt)", currentScore_, achievementBonusScore_);
  if (ImGui::TreeNode("スコア TOP 3 ランキング (ローカル)")) {
    for (int i = 0; i < kRankingCount; i++) {
      if (topScoreRankings_[i] > 0.0f) {
        ImGui::Text("%d 位: %.0f", i + 1, topScoreRankings_[i]);
      } else {
        ImGui::Text("%d 位: ---", i + 1);
      }
    }
    ImGui::TreePop();
  }

  ImGui::Separator();
  if (ImGui::TreeNode("Firebase オンラインランキング")) {
    auto rankingMgr = FirebaseRankingManager::GetInstance();
    ImGui::Text("ステータス: %s", rankingMgr->GetLastStatusMessage().c_str());

    ImGui::InputText("プレイヤー名", inputPlayerName_, sizeof(inputPlayerName_));

    if (rankingMgr->IsSending()) {
      ImGui::BeginDisabled();
      ImGui::Button("送信中...");
      ImGui::EndDisabled();
    } else {
      if (ImGui::Button("現在のスコアをFirebaseへ送信")) {
        SendCurrentScoreToOnlineRanking(inputPlayerName_);
      }
    }

    ImGui::SameLine();
    if (rankingMgr->IsFetching()) {
      ImGui::BeginDisabled();
      ImGui::Button("取得中...");
      ImGui::EndDisabled();
    } else {
      if (ImGui::Button("ランキングを取得 (距離・スコア)")) {
        rankingMgr->FetchBothRankingsAsync();
      }
    }

    ImGui::TreePop();
  }
#endif
}


#include "ScoreManager.h"
#include "Network/Firebase/FirebaseRankingManager.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <nlohmann/json.hpp>

ScoreManager::ScoreManager() {
  LoadLocalScores();
}

void ScoreManager::Initialize() {
  LoadLocalScores();
}

void ScoreManager::Reset() {
  currentDistance_ = 0.0f;
  currentScore_ = 0.0f;
  bonusEnemyHitCount_ = 0;
  achievementBonusScore_ = 0.0f;
  isOnlineScoreSent_ = false;
  lastDistanceRank_ = -1;
  lastScoreRank_ = -1;
}

bool ScoreManager::SaveLocalScores(const std::string &filePath) const {
  try {
    std::filesystem::path path(filePath);
    if (path.has_parent_path() && !std::filesystem::exists(path.parent_path())) {
      std::filesystem::create_directories(path.parent_path());
    }

    nlohmann::json root;
    root["playerName"] = playerName_;

    nlohmann::json scoreArray = nlohmann::json::array();
    for (int i = 0; i < kRankingCount; i++) {
      scoreArray.push_back(topScoreRankings_[i]);
    }
    root["topScoreRankings"] = scoreArray;

    nlohmann::json distArray = nlohmann::json::array();
    for (int i = 0; i < kRankingCount; i++) {
      distArray.push_back(topRankings_[i]);
    }
    root["topDistanceRankings"] = distArray;

    root["bestScore"] = topScoreRankings_[0];
    root["bestDistance"] = topRankings_[0];

    std::ofstream ofs(path);
    if (!ofs.is_open()) {
      return false;
    }
    ofs << root.dump(4);
    ofs.close();
    return true;
  } catch (const std::exception &) {
    return false;
  }
}

bool ScoreManager::LoadLocalScores(const std::string &filePath) {
  try {
    std::filesystem::path path(filePath);
    if (!std::filesystem::exists(path)) {
      return false;
    }

    std::ifstream ifs(path);
    if (!ifs.is_open()) {
      return false;
    }

    nlohmann::json root;
    ifs >> root;
    ifs.close();

    if (root.contains("playerName") && root["playerName"].is_string()) {
      std::string loadedName = root["playerName"].get<std::string>();
      playerName_ = loadedName.empty() ? "Player" : loadedName;
      snprintf(inputPlayerName_, sizeof(inputPlayerName_), "%s",
               playerName_.c_str());
    }

    if (root.contains("topScoreRankings") &&
        root["topScoreRankings"].is_array()) {
      const auto &arr = root["topScoreRankings"];
      for (int i = 0; i < kRankingCount; i++) {
        if (static_cast<size_t>(i) < arr.size() && arr[i].is_number()) {
          topScoreRankings_[i] =
              (std::max)(0.0f, arr[i].get<float>());
        } else {
          topScoreRankings_[i] = 0.0f;
        }
      }
    }

    if (root.contains("topDistanceRankings") &&
        root["topDistanceRankings"].is_array()) {
      const auto &arr = root["topDistanceRankings"];
      for (int i = 0; i < kRankingCount; i++) {
        if (static_cast<size_t>(i) < arr.size() && arr[i].is_number()) {
          topRankings_[i] =
              (std::max)(0.0f, arr[i].get<float>());
        } else {
          topRankings_[i] = 0.0f;
        }
      }
    }

    return true;
  } catch (const std::exception &) {
    return false;
  }
}

void ScoreManager::ClearLocalScores() {
  for (int i = 0; i < kRankingCount; i++) {
    topRankings_[i] = 0.0f;
    topScoreRankings_[i] = 0.0f;
  }
  lastDistanceRank_ = -1;
  lastScoreRank_ = -1;
  SaveLocalScores();
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
  SaveLocalScores();
}

void ScoreManager::SetPlayerName(const std::string& name) {
  playerName_ = name.empty() ? "Player" : name;
  snprintf(inputPlayerName_, sizeof(inputPlayerName_), "%s", playerName_.c_str());
  SaveLocalScores();
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
  if (lastDistanceRank_ == 1) {
    return true;
  }
  if (topRankings_[0] > 0.0f) {
    return (currentDistance_ >= topRankings_[0]);
  }
  return (currentDistance_ >= 500.0f);
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

  if (ImGui::Button("ローカルスコア保存")) {
    SaveLocalScores();
  }
  ImGui::SameLine();
  if (ImGui::Button("ローカルスコア読込")) {
    LoadLocalScores();
  }
  ImGui::SameLine();
  if (ImGui::Button("ローカル記録リセット")) {
    ClearLocalScores();
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


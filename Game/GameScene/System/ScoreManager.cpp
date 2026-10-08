#include "ScoreManager.h"
#include <imgui.h>

void ScoreManager::Reset() {
  currentDistance_ = 0.0f;
  currentScore_ = 0.0f;
  bonusEnemyHitCount_ = 0;
}

void ScoreManager::Update(float scrollSpeed, float timeScale) {
  currentDistance_ += scrollSpeed * timeScale;
  currentScore_ = currentDistance_ + (bonusEnemyHitCount_ * 200.0f * scrollSpeed);
}

void ScoreManager::AddBonusEnemyHit() {
  bonusEnemyHitCount_++;
}

void ScoreManager::RecordCurrentScores() {
  UpdateRanking();
  UpdateScoreRanking();
}

void ScoreManager::UpdateRanking() {
  for (int i = 0; i < 3; i++) {
    if (currentDistance_ > topRankings_[i]) {
      for (int j = 2; j > i; j--) {
        topRankings_[j] = topRankings_[j - 1];
      }
      topRankings_[i] = currentDistance_;
      break;
    }
  }
}

void ScoreManager::UpdateScoreRanking() {
  for (int i = 0; i < 3; i++) {
    if (currentScore_ > topScoreRankings_[i]) {
      for (int j = 2; j > i; j--) {
        topScoreRankings_[j] = topScoreRankings_[j - 1];
      }
      topScoreRankings_[i] = currentScore_;
      break;
    }
  }
}

void ScoreManager::ImGui() {
#ifdef _USE_IMGUI
  ImGui::Text("Current Distance: %.2f m", currentDistance_);
  if (ImGui::TreeNode("Distance Top 3 Ranking")) {
    for (int i = 0; i < 3; i++) {
      if (topRankings_[i] > 0.0f) {
        ImGui::Text("Rank %d: %.2f m", i + 1, topRankings_[i]);
      } else {
        ImGui::Text("Rank %d: ---", i + 1);
      }
    }
    ImGui::TreePop();
  }

  ImGui::Text("Current Score: %.0f", currentScore_);
  if (ImGui::TreeNode("Score Top 3 Ranking")) {
    for (int i = 0; i < 3; i++) {
      if (topScoreRankings_[i] > 0.0f) {
        ImGui::Text("Rank %d: %.0f", i + 1, topScoreRankings_[i]);
      } else {
        ImGui::Text("Rank %d: ---", i + 1);
      }
    }
    ImGui::TreePop();
  }
#endif
}

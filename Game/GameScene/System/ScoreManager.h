#pragma once

class ScoreManager {
private:
  static constexpr int kRankingCount = 3;
  static constexpr float kBonusScoreMultiplier = 200.0f;

  float currentDistance_ = 0.0f;
  float currentScore_ = 0.0f;
  int bonusEnemyHitCount_ = 0;

  float topRankings_[kRankingCount] = {0.0f, 0.0f, 0.0f};
  float topScoreRankings_[kRankingCount] = {0.0f, 0.0f, 0.0f};

public:
  ScoreManager() = default;
  ~ScoreManager() = default;

  void Reset();
  void Update(float scrollSpeed, float timeScale);
  void AddBonusEnemyHit();
  void RecordCurrentScores();

  float GetCurrentDistance() const { return currentDistance_; }
  float GetCurrentScore() const { return currentScore_; }
  int GetBonusEnemyHitCount() const { return bonusEnemyHitCount_; }

  const float *GetTopRankings() const { return topRankings_; }
  const float *GetTopScoreRankings() const { return topScoreRankings_; }

  bool IsNewDistanceRecord() const {
    return topRankings_[0] > 0.0f && currentDistance_ >= topRankings_[0];
  }
  bool IsNewScoreRecord() const {
    return topScoreRankings_[0] > 0.0f && currentScore_ >= topScoreRankings_[0];
  }

  void ImGui();

private:
  void UpdateRanking();
  void UpdateScoreRanking();
};

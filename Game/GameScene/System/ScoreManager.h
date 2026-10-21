#pragma once
#include <string>

class ScoreManager {
private:
  static constexpr int kRankingCount = 3;
  static constexpr float kBonusScoreMultiplier = 200.0f;

  float currentDistance_ = 0.0f;
  float currentScore_ = 0.0f;
  int bonusEnemyHitCount_ = 0;
  float achievementBonusScore_ = 0.0f;

  float topRankings_[kRankingCount] = {0.0f, 0.0f, 0.0f};
  float topScoreRankings_[kRankingCount] = {0.0f, 0.0f, 0.0f};

public:
  ScoreManager() = default;
  ~ScoreManager() = default;

  /// <summary>
  /// スコア、走行距離、ボーナス加算などの状態をリセット
  /// </summary>
  void Reset();

  /// <summary>
  /// 走行距離とスコアの毎フレーム加算更新
  /// </summary>
  /// <param name="scrollSpeed">現在のスクロール速度</param>
  /// <param name="timeScale">時間スケール</param>
  void Update(float scrollSpeed, float timeScale);

  /// <summary>
  /// ボーナスエネミー撃破時のスコア加算
  /// </summary>
  void AddBonusEnemyHit();

  /// <summary>
  /// 実績解除に伴うボーナススコア加算
  /// </summary>
  /// <param name="score">加算するスコア</param>
  void AddAchievementScore(float score);

  /// <summary>
  /// ゲームオーバー時のスコア・距離をローカルランキングへ記録
  /// </summary>
  void RecordCurrentScores();

  float GetCurrentDistance() const { return currentDistance_; }
  float GetCurrentScore() const { return currentScore_; }
  int GetBonusEnemyHitCount() const { return bonusEnemyHitCount_; }
  float GetAchievementBonusScore() const { return achievementBonusScore_; }

  const float *GetTopRankings() const { return topRankings_; }
  const float *GetTopScoreRankings() const { return topScoreRankings_; }

  bool IsNewDistanceRecord() const {
    return topRankings_[0] > 0.0f && currentDistance_ >= topRankings_[0];
  }
  bool IsNewScoreRecord() const {
    return topScoreRankings_[0] > 0.0f && currentScore_ >= topScoreRankings_[0];
  }

  int GetLastDistanceRank() const { return lastDistanceRank_; }
  int GetLastScoreRank() const { return lastScoreRank_; }

  /// <summary>
  /// 今回のプレイがスコアランキング1位であるか
  /// </summary>
  bool IsScoreFirstPlace() const;

  /// <summary>
  /// 今回のプレイが走行距離ランキング1位であるか
  /// </summary>
  bool IsDistanceFirstPlace() const;

  /// <summary>
  /// 今回のプレイがランキング1位（スコアまたは走行距離）であるか
  /// </summary>
  bool IsFirstPlace() const;

  /// <summary>
  /// 現在のプレイヤー名・走行距離・スコアを Firebase オンラインランキングへ送信
  /// </summary>
  void SendCurrentScoreToOnlineRanking(const std::string& playerName);

  /// <summary>
  /// ゲームオーバー時の自動送信＆ランキング取得（1ゲームにつき1回のみ実行）
  /// </summary>
  void SendOnlineScoreAndFetchRanking();

  const std::string& GetPlayerName() const { return playerName_; }
  void SetPlayerName(const std::string& name);

  bool IsOnlineScoreSent() const { return isOnlineScoreSent_; }

  void ImGui();

private:
  void UpdateRanking();
  void UpdateScoreRanking();

  int lastDistanceRank_ = -1;
  int lastScoreRank_ = -1;

  std::string playerName_ = "Player";
  bool isOnlineScoreSent_ = false;
  char inputPlayerName_[32] = "Player";
};


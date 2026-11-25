#pragma once
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <queue>
#include "EnvLoader.h"

/// <summary>
/// ランキングエントリ構造体
/// </summary>
struct RankingEntry {
  int rank = 0;              ///< 順位（1〜）
  std::string playerName;    ///< プレイヤーネーム
  float distance = 0.0f;     ///< 走行距離
  float score = 0.0f;        ///< スコア
  std::string timestamp;     ///< 登録日時 (ISO8601等)
  std::string documentId;    ///< Firestore ドキュメントID
};

/// <summary>
/// スコア送信コールバック型 (success, message)
/// </summary>
using SendScoreCallback = std::function<void(bool success, const std::string& message)>;

/// <summary>
/// ランキング取得コールバック型 (success, rankings, message)
/// </summary>
using FetchRankingCallback = std::function<void(bool success, const std::vector<RankingEntry>& rankings, const std::string& message)>;

/// <summary>
/// ソート対象フィールド
/// </summary>
enum class RankingSortField {
  Score,
  Distance
};

/// <summary>
/// Firebase (Cloud Firestore REST API) を使用したオンラインランキング管理クラス
/// </summary>
class FirebaseRankingManager {
public:
  static FirebaseRankingManager* GetInstance();

  /// <summary>
  /// 初期化（My.env を読み込み接続設定を構築）
  /// </summary>
  /// <param name="envPath">My.env のファイルパス（デフォルト: "My.env"）</param>
  bool Initialize(const std::string& envPath = "My.env");

  /// <summary>
  /// 毎フレーム呼び出す更新処理（非同期コールバックをメインスレッドで安全に実行）
  /// </summary>
  void Update();

  /// <summary>
  /// スコアを Firebase に非同期送信
  /// </summary>
  /// <param name="playerName">プレイヤーネーム</param>
  /// <param name="distance">走行距離</param>
  /// <param name="score">スコア</param>
  /// <param name="callback">完了時のコールバック（メインスレッドで呼ばれます）</param>
  void SendScoreAsync(
      const std::string& playerName,
      float distance,
      float score,
      SendScoreCallback callback = nullptr);

  /// <summary>
  /// オンラインランキングを Firebase から非同期取得（スコア順）
  /// </summary>
  /// <param name="limit">取得件数（0以下の場合は My.env の設定値を使用）</param>
  /// <param name="callback">完了時のコールバック（メインスレッドで呼ばれます）</param>
  void FetchRankingAsync(
      int limit = 0,
      FetchRankingCallback callback = nullptr);

  /// <summary>
  /// 指定したソートフィールドでオンラインランキングを非同期取得
  /// </summary>
  void FetchRankingAsync(
      RankingSortField sortField,
      int limit = 0,
      FetchRankingCallback callback = nullptr);

  /// <summary>
  /// 走行距離・スコアの両方のランキングを非同期一括取得
  /// </summary>
  void FetchBothRankingsAsync(
      int limit = 0,
      std::function<void(bool success)> callback = nullptr);

  /// <summary>
  /// キャッシュされているスコアランキングエントリ一覧を取得（互換用）
  /// </summary>
  const std::vector<RankingEntry>& GetCachedRankings() const { return cachedScoreRankings_; }

  /// <summary>
  /// キャッシュされているスコアランキングエントリ一覧を取得
  /// </summary>
  const std::vector<RankingEntry>& GetCachedScoreRankings() const { return cachedScoreRankings_; }

  /// <summary>
  /// キャッシュされている走行距離ランキングエントリ一覧を取得
  /// </summary>
  const std::vector<RankingEntry>& GetCachedDistanceRankings() const { return cachedDistanceRankings_; }

  /// <summary>
  /// 直前に送信されたドキュメントのIDを取得
  /// </summary>
  const std::string& GetLastSubmittedDocId() const { return lastSubmittedDocId_; }

  /// <summary>
  /// 直前に送信された走行距離を取得
  /// </summary>
  float GetLastSubmittedDistance() const { return lastSubmittedDistance_; }

  /// <summary>
  /// 直前に送信されたスコアを取得
  /// </summary>
  float GetLastSubmittedScore() const { return lastSubmittedScore_; }

  /// <summary>
  /// Firebase のキー情報が設定されているか
  /// </summary>
  bool IsConfigured() const;

  /// <summary>
  /// 現在送信中か
  /// </summary>
  bool IsSending() const { return isSending_; }

  /// <summary>
  /// 現在ランキング取得中か
  /// </summary>
  bool IsFetching() const { return isFetching_; }

  /// <summary>
  /// 直近のエラーまたはステータスメッセージ
  /// </summary>
  const std::string& GetLastStatusMessage() const { return lastStatusMessage_; }

  /// <summary>
  /// 直前の送信が成功したか
  /// </summary>
  bool WasLastSendSuccess() const { return lastSendSuccess_; }

  /// <summary>
  /// EnvLoader の取得
  /// </summary>
  const EnvLoader& GetEnvLoader() const { return envLoader_; }

  /// <summary>
  /// ImGui デバッグUI（設定確認・テスト送信・ランキング表示）
  /// </summary>
  void ImGui();

private:
  FirebaseRankingManager() = default;
  ~FirebaseRankingManager() = default;
  FirebaseRankingManager(const FirebaseRankingManager&) = delete;
  FirebaseRankingManager& operator=(const FirebaseRankingManager&) = delete;

  std::vector<RankingEntry> QueryFirestoreSync(
      const std::string& projectId,
      const std::string& apiKey,
      const std::string& collection,
      const std::string& sortField,
      int fetchLimit,
      bool& outSuccess,
      std::string& outMsg);

  std::string GenerateCurrentIsoTimestamp() const;
  std::string MaskKey(const std::string& key) const;

  EnvLoader envLoader_;
  std::string projectId_;
  std::string apiKey_;
  std::string collectionName_ = "scores";
  int rankingLimit_ = 10;
  int nicknameMaxLength_ = 12;

  std::vector<RankingEntry> cachedScoreRankings_;
  std::vector<RankingEntry> cachedDistanceRankings_;
  std::string lastSubmittedDocId_;
  float lastSubmittedDistance_ = 0.0f;
  float lastSubmittedScore_ = 0.0f;

  bool isSending_ = false;
  bool isFetching_ = false;
  bool lastSendSuccess_ = false;
  std::string lastStatusMessage_ = "Uninitialized";

  std::mutex queueMutex_;
  std::queue<std::function<void()>> mainThreadQueue_;

  // ImGui テスト用バッファ
  char testPlayerName_[64] = "Player1";
  float testDistance_ = 125.5f;
  float testScore_ = 1500.0f;
};

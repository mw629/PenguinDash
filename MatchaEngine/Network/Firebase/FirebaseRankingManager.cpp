#include "FirebaseRankingManager.h"
#include "HttpClient.h"
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <imgui.h>

using json = nlohmann::json;

FirebaseRankingManager* FirebaseRankingManager::GetInstance() {
  static FirebaseRankingManager instance;
  return &instance;
}

bool FirebaseRankingManager::Initialize(const std::string& envPath) {
  bool loaded = envLoader_.Load(envPath);

  projectId_ = envLoader_.Get("FIREBASE_PROJECT_ID");
  apiKey_ = envLoader_.Get("FIREBASE_API_KEY");
  rankingLimit_ = envLoader_.GetInt("RANKING_LIMIT", 10);
  if (rankingLimit_ <= 0) {
    rankingLimit_ = 10;
  }
  nicknameMaxLength_ = envLoader_.GetInt("NICKNAME_MAX_LENGTH", 12);
  if (nicknameMaxLength_ <= 0) {
    nicknameMaxLength_ = 12;
  }

  if (IsConfigured()) {
    lastStatusMessage_ = "初期化完了。Firebase (プロジェクト: " + projectId_ + ") への接続準備完了";
  } else {
    lastStatusMessage_ = "初期化完了。ただし " + envPath + " に FIREBASE_PROJECT_ID または FIREBASE_API_KEY が設定されていません";
  }

  return loaded;
}

bool FirebaseRankingManager::IsConfigured() const {
  return !projectId_.empty() && !apiKey_.empty();
}

std::string FirebaseRankingManager::GenerateCurrentIsoTimestamp() const {
  auto now = std::chrono::system_clock::now();
  auto in_time_t = std::chrono::system_clock::to_time_t(now);
  std::stringstream ss;
  tm gmt = {};
  gmtime_s(&gmt, &in_time_t);
  ss << std::put_time(&gmt, "%Y-%m-%dT%H:%M:%SZ");
  return ss.str();
}

std::string FirebaseRankingManager::MaskKey(const std::string& key) const {
  if (key.size() <= 8) {
    return std::string(key.size(), '*');
  }
  return key.substr(0, 4) + "..." + key.substr(key.size() - 4);
}

void FirebaseRankingManager::Update() {
  std::queue<std::function<void()>> toExecute;
  {
    std::lock_guard<std::mutex> lock(queueMutex_);
    toExecute.swap(mainThreadQueue_);
  }

  while (!toExecute.empty()) {
    auto& func = toExecute.front();
    if (func) {
      func();
    }
    toExecute.pop();
  }
}

void FirebaseRankingManager::SendScoreAsync(
    const std::string& playerName,
    float distance,
    float score,
    SendScoreCallback callback) {
  if (!IsConfigured()) {
    lastStatusMessage_ = "スコア送信不可: My.env に FIREBASE_PROJECT_ID または FIREBASE_API_KEY が設定されていません";
    lastSendSuccess_ = false;
    if (callback) {
      callback(false, lastStatusMessage_);
    }
    return;
  }

  if (isSending_) {
    if (callback) {
      callback(false, "別のスコア送信処理がすでに実行中です。");
    }
    return;
  }

  isSending_ = true;
  lastStatusMessage_ = "スコア送信中...";

  // プレイヤー名のサニタイズ
  std::string sanitizedName = playerName;
  if (sanitizedName.empty()) {
    sanitizedName = "Player";
  }
  if (static_cast<int>(sanitizedName.size()) > nicknameMaxLength_) {
    sanitizedName = sanitizedName.substr(0, nicknameMaxLength_);
  }

  std::string timestamp = GenerateCurrentIsoTimestamp();
  std::string projectId = projectId_;
  std::string apiKey = apiKey_;
  std::string collection = collectionName_;

  std::thread([this, sanitizedName, distance, score, timestamp, projectId, apiKey, collection, callback]() {
    // Firestore REST API (ドキュメント追加)
    // POST https://firestore.googleapis.com/v1/projects/{projectId}/databases/(default)/documents/{collection}?key={apiKey}
    std::string url = "https://firestore.googleapis.com/v1/projects/" + projectId +
                      "/databases/(default)/documents/" + collection +
                      "?key=" + apiKey;

    json docJson;
    docJson["fields"]["playerName"]["stringValue"] = sanitizedName;
    docJson["fields"]["distance"]["doubleValue"] = distance;
    docJson["fields"]["score"]["doubleValue"] = score;
    docJson["fields"]["createdAt"]["timestampValue"] = timestamp;

    std::string requestBody = docJson.dump();
    HttpResponse response = HttpClient::PostJson(url, requestBody);

    bool success = response.success;
    std::string msg;
    if (success) {
      msg = "スコアの送信に成功しました！";
    } else {
      msg = "スコア送信失敗: " + response.errorMessage;
      if (!response.body.empty()) {
        try {
          auto errJson = json::parse(response.body);
          if (errJson.contains("error") && errJson["error"].contains("message")) {
            msg += " (" + errJson["error"]["message"].get<std::string>() + ")";
          }
        } catch (...) {
          // JSONパース不可時はそのまま
        }
      }
    }

    {
      std::lock_guard<std::mutex> lock(this->queueMutex_);
      this->mainThreadQueue_.push([this, success, msg, callback]() {
        this->isSending_ = false;
        this->lastSendSuccess_ = success;
        this->lastStatusMessage_ = msg;
        if (callback) {
          callback(success, msg);
        }
      });
    }
  }).detach();
}

void FirebaseRankingManager::FetchRankingAsync(
    int limit,
    FetchRankingCallback callback) {
  if (!IsConfigured()) {
    lastStatusMessage_ = "ランキング取得不可: My.env に FIREBASE_PROJECT_ID または FIREBASE_API_KEY が設定されていません";
    if (callback) {
      callback(false, cachedRankings_, lastStatusMessage_);
    }
    return;
  }

  if (isFetching_) {
    if (callback) {
      callback(false, cachedRankings_, "別のランキング取得処理がすでに実行中です。");
    }
    return;
  }

  int fetchLimit = (limit > 0) ? limit : rankingLimit_;
  isFetching_ = true;
  lastStatusMessage_ = "ランキング取得中...";

  std::string projectId = projectId_;
  std::string apiKey = apiKey_;
  std::string collection = collectionName_;

  std::thread([this, fetchLimit, projectId, apiKey, collection, callback]() {
    // Firestore runQuery REST API
    // POST https://firestore.googleapis.com/v1/projects/{projectId}/databases/(default)/documents:runQuery?key={apiKey}
    std::string url = "https://firestore.googleapis.com/v1/projects/" + projectId +
                      "/databases/(default)/documents:runQuery?key=" + apiKey;

    json queryJson;
    queryJson["structuredQuery"]["from"] = json::array({ {{"collectionId", collection}} });
    queryJson["structuredQuery"]["orderBy"] = json::array({
        {
            {"field", {{"fieldPath", "score"}}},
            {"direction", "DESCENDING"}
        }
    });
    queryJson["structuredQuery"]["limit"] = fetchLimit;

    std::string requestBody = queryJson.dump();
    HttpResponse response = HttpClient::PostJson(url, requestBody);

    std::vector<RankingEntry> results;
    bool success = response.success;
    std::string msg;

    if (success) {
      try {
        auto resArray = json::parse(response.body);
        int currentRank = 1;

        for (const auto& item : resArray) {
          if (!item.contains("document")) {
            continue;
          }
          const auto& doc = item["document"];
          if (!doc.contains("fields")) {
            continue;
          }

          const auto& fields = doc["fields"];
          RankingEntry entry;
          entry.rank = currentRank++;

          // ドキュメントID
          if (doc.contains("name")) {
            std::string namePath = doc["name"].get<std::string>();
            size_t slashPos = namePath.find_last_of('/');
            if (slashPos != std::string::npos) {
              entry.documentId = namePath.substr(slashPos + 1);
            }
          }

          // プレイヤー名
          if (fields.contains("playerName") && fields["playerName"].contains("stringValue")) {
            entry.playerName = fields["playerName"]["stringValue"].get<std::string>();
          } else {
            entry.playerName = "Unknown";
          }

          // 走行距離 (doubleValue または integerValue)
          if (fields.contains("distance")) {
            if (fields["distance"].contains("doubleValue")) {
              entry.distance = static_cast<float>(fields["distance"]["doubleValue"].get<double>());
            } else if (fields["distance"].contains("integerValue")) {
              entry.distance = static_cast<float>(std::stod(fields["distance"]["integerValue"].get<std::string>()));
            }
          }

          // スコア (doubleValue または integerValue)
          if (fields.contains("score")) {
            if (fields["score"].contains("doubleValue")) {
              entry.score = static_cast<float>(fields["score"]["doubleValue"].get<double>());
            } else if (fields["score"].contains("integerValue")) {
              entry.score = static_cast<float>(std::stod(fields["score"]["integerValue"].get<std::string>()));
            }
          }

          // タイムスタンプ
          if (fields.contains("createdAt") && fields["createdAt"].contains("timestampValue")) {
            entry.timestamp = fields["createdAt"]["timestampValue"].get<std::string>();
          } else if (doc.contains("createTime")) {
            entry.timestamp = doc["createTime"].get<std::string>();
          }

          results.push_back(entry);
        }

        msg = "ランキング取得完了 (" + std::to_string(results.size()) + " 件取得)";
      } catch (const std::exception& e) {
        success = false;
        msg = "JSON解析エラー: " + std::string(e.what());
      }
    } else {
      msg = "ランキング取得失敗: " + response.errorMessage;
      if (!response.body.empty()) {
        try {
          auto errJson = json::parse(response.body);
          if (errJson.contains("error") && errJson["error"].contains("message")) {
            msg += " (" + errJson["error"]["message"].get<std::string>() + ")";
          }
        } catch (...) {
        }
      }
    }

    {
      std::lock_guard<std::mutex> lock(this->queueMutex_);
      this->mainThreadQueue_.push([this, success, results, msg, callback]() {
        this->isFetching_ = false;
        if (success) {
          this->cachedRankings_ = results;
        }
        this->lastStatusMessage_ = msg;
        if (callback) {
          callback(success, this->cachedRankings_, msg);
        }
      });
    }
  }).detach();
}

void FirebaseRankingManager::ImGui() {
#ifdef _USE_IMGUI
  if (ImGui::Begin("オンラインランキング (Firebase)")) {
    ImGui::Text("=== 接続設定・ステータス ===");
    ImGui::Text("設定ファイルパス: %s", envLoader_.GetLoadedPath().c_str());
    ImGui::Text("接続準備: %s", IsConfigured() ? "OK (設定完了)" : "未設定 (My.envを確認してください)");
    ImGui::Text("プロジェクトID: %s", projectId_.empty() ? "(未入力)" : projectId_.c_str());
    ImGui::Text("APIキー: %s", apiKey_.empty() ? "(未入力)" : MaskKey(apiKey_).c_str());
    ImGui::Text("コレクション名: %s", collectionName_.c_str());
    ImGui::Text("プレイヤー名最大文字数: %d 文字", nicknameMaxLength_);
    ImGui::Text("取得件数リミット: %d 件", rankingLimit_);

    if (ImGui::Button("My.env を再読み込み")) {
      Initialize("My.env");
    }

    ImGui::Separator();
    ImGui::Text("最新ステータス: %s", lastStatusMessage_.c_str());

    ImGui::Separator();
    ImGui::Text("=== スコア送信テスト ===");
    ImGui::InputText("プレイヤー名", testPlayerName_, IM_ARRAYSIZE(testPlayerName_));
    ImGui::InputFloat("走行距離 (m)", &testDistance_, 1.0f, 10.0f, "%.2f");
    ImGui::InputFloat("スコア", &testScore_, 10.0f, 100.0f, "%.0f");

    if (isSending_) {
      ImGui::BeginDisabled();
      ImGui::Button("送信中...");
      ImGui::EndDisabled();
    } else {
      if (ImGui::Button("スコアをテスト送信")) {
        SendScoreAsync(testPlayerName_, testDistance_, testScore_);
      }
    }

    ImGui::Separator();
    ImGui::Text("=== オンラインランキング一覧 ===");
    if (isFetching_) {
      ImGui::BeginDisabled();
      ImGui::Button("取得中...");
      ImGui::EndDisabled();
    } else {
      if (ImGui::Button("ランキングを最新に更新")) {
        FetchRankingAsync();
      }
    }

    if (ImGui::BeginTable("RankingTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
      ImGui::TableSetupColumn("順位", ImGuiTableColumnFlags_WidthFixed, 50.0f);
      ImGui::TableSetupColumn("プレイヤー名", ImGuiTableColumnFlags_WidthStretch);
      ImGui::TableSetupColumn("走行距離", ImGuiTableColumnFlags_WidthFixed, 90.0f);
      ImGui::TableSetupColumn("スコア", ImGuiTableColumnFlags_WidthFixed, 90.0f);
      ImGui::TableSetupColumn("登録日時", ImGuiTableColumnFlags_WidthFixed, 160.0f);
      ImGui::TableHeadersRow();

      if (cachedRankings_.empty()) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("-");
        ImGui::TableSetColumnIndex(1);
        ImGui::TextDisabled("データがまだ取得されていないか、登録がありません");
        ImGui::TableSetColumnIndex(2);
        ImGui::TextDisabled("-");
        ImGui::TableSetColumnIndex(3);
        ImGui::TextDisabled("-");
        ImGui::TableSetColumnIndex(4);
        ImGui::TextDisabled("-");
      } else {
        for (const auto& entry : cachedRankings_) {
          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(0);
          ImGui::Text("%d 位", entry.rank);
          ImGui::TableSetColumnIndex(1);
          ImGui::TextUnformatted(entry.playerName.c_str());
          ImGui::TableSetColumnIndex(2);
          ImGui::Text("%.2f m", entry.distance);
          ImGui::TableSetColumnIndex(3);
          ImGui::Text("%.0f", entry.score);
          ImGui::TableSetColumnIndex(4);
          ImGui::TextUnformatted(entry.timestamp.c_str());
        }
      }
      ImGui::EndTable();
    }
  }
  ImGui::End();
#endif
}

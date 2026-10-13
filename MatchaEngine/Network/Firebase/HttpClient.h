#pragma once
#include <string>
#include <vector>

/// @brief HTTP レスポンス構造体
struct HttpResponse {
  int statusCode = 0;
  std::string body;
  std::string errorMessage;
  bool success = false;
};

/// @brief Windows標準の WinHTTP を利用した HTTPS/HTTP クライアント
class HttpClient {
public:
  HttpClient() = default;
  ~HttpClient() = default;

  /// @brief 同期 HTTP リクエストを実行
  /// @param method "GET", "POST", "PUT", "DELETE" 等
  /// @param url 完全な URL ("https://...")
  /// @param body リクエストボディ（POST/PUT 時）
  /// @param headers 追加ヘッダー配列（例: "Content-Type: application/json"）
  /// @param timeoutMs タイムアウト時間（ミリ秒、デフォルト 10000ms）
  /// @return HttpResponse
  static HttpResponse Request(
      const std::string& method,
      const std::string& url,
      const std::string& body = "",
      const std::vector<std::string>& headers = {},
      int timeoutMs = 10000);

  /// @brief GET リクエスト（簡易版）
  static HttpResponse Get(const std::string& url, int timeoutMs = 10000) {
    return Request("GET", url, "", {}, timeoutMs);
  }

  /// @brief JSON POST リクエスト（簡易版）
  static HttpResponse PostJson(const std::string& url, const std::string& jsonBody, int timeoutMs = 10000) {
    return Request("POST", url, jsonBody, {"Content-Type: application/json; charset=utf-8"}, timeoutMs);
  }

private:
  static std::wstring Utf8ToWide(const std::string& utf8);
  static std::string WideToUtf8(const std::wstring& wide);
};

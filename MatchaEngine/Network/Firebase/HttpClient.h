#pragma once
#include <string>
#include <vector>

/// <summary>
/// HTTP レスポンス構造体
/// </summary>
struct HttpResponse {
  int statusCode = 0;
  std::string body;
  std::string errorMessage;
  bool success = false;
};

/// <summary>
/// Windows標準の WinHTTP を利用した HTTPS/HTTP クライアント
/// </summary>
class HttpClient {
public:
  HttpClient() = default;
  ~HttpClient() = default;

  /// <summary>
  /// 同期 HTTP リクエストを実行
  /// </summary>
  /// <param name="method">"GET", "POST", "PUT", "DELETE" 等</param>
  /// <param name="url">完全な URL ("https://...")</param>
  /// <param name="body">リクエストボディ（POST/PUT 時）</param>
  /// <param name="headers">追加ヘッダー配列（例: "Content-Type: application/json"）</param>
  /// <param name="timeoutMs">タイムアウト時間（ミリ秒、デフォルト 10000ms）</param>
  /// <returns>HttpResponse</returns>
  static HttpResponse Request(
      const std::string& method,
      const std::string& url,
      const std::string& body = "",
      const std::vector<std::string>& headers = {},
      int timeoutMs = 10000);

  /// <summary>
  /// GET リクエスト（簡易版）
  /// </summary>
  static HttpResponse Get(const std::string& url, int timeoutMs = 10000) {
    return Request("GET", url, "", {}, timeoutMs);
  }

  /// <summary>
  /// JSON POST リクエスト（簡易版）
  /// </summary>
  static HttpResponse PostJson(const std::string& url, const std::string& jsonBody, int timeoutMs = 10000) {
    return Request("POST", url, jsonBody, {"Content-Type: application/json; charset=utf-8"}, timeoutMs);
  }

private:
  static std::wstring Utf8ToWide(const std::string& utf8);
  static std::string WideToUtf8(const std::wstring& wide);
};

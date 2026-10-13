#include "HttpClient.h"
#include <windows.h>
#include <winhttp.h>
#include <vector>

#pragma comment(lib, "winhttp.lib")

std::wstring HttpClient::Utf8ToWide(const std::string& utf8) {
  if (utf8.empty()) return L"";
  int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), nullptr, 0);
  if (sizeNeeded <= 0) return L"";
  std::wstring wide(sizeNeeded, 0);
  MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), &wide[0], sizeNeeded);
  return wide;
}

std::string HttpClient::WideToUtf8(const std::wstring& wide) {
  if (wide.empty()) return "";
  int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
  if (sizeNeeded <= 0) return "";
  std::string utf8(sizeNeeded, 0);
  WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()), &utf8[0], sizeNeeded, nullptr, nullptr);
  return utf8;
}

HttpResponse HttpClient::Request(
    const std::string& method,
    const std::string& url,
    const std::string& body,
    const std::vector<std::string>& headers,
    int timeoutMs) {
  HttpResponse response;
  std::wstring wideUrl = Utf8ToWide(url);

  // URL をパース
  URL_COMPONENTS urlComp = {};
  urlComp.dwStructSize = sizeof(urlComp);
  urlComp.dwSchemeLength = static_cast<DWORD>(-1);
  urlComp.dwHostNameLength = static_cast<DWORD>(-1);
  urlComp.dwUrlPathLength = static_cast<DWORD>(-1);
  urlComp.dwExtraInfoLength = static_cast<DWORD>(-1);

  if (!WinHttpCrackUrl(wideUrl.c_str(), static_cast<DWORD>(wideUrl.length()), 0, &urlComp)) {
    response.errorMessage = "Failed to parse URL: " + url;
    return response;
  }

  std::wstring hostName(urlComp.lpszHostName, urlComp.dwHostNameLength);
  std::wstring urlPath(urlComp.lpszUrlPath, urlComp.dwUrlPathLength + urlComp.dwExtraInfoLength);
  bool isHttps = (urlComp.nScheme == INTERNET_SCHEME_HTTPS);

  // WinHTTP セッション開始
  HINTERNET hSession = WinHttpOpen(
      L"MatchaEngine-FirebaseClient/1.0",
      WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
      WINHTTP_NO_PROXY_NAME,
      WINHTTP_NO_PROXY_BYPASS,
      0);

  if (!hSession) {
    response.errorMessage = "WinHttpOpen failed (Error: " + std::to_string(GetLastError()) + ")";
    return response;
  }

  // タイムアウト設定
  WinHttpSetTimeouts(hSession, timeoutMs, timeoutMs, timeoutMs, timeoutMs);

  // ホストへ接続
  HINTERNET hConnect = WinHttpConnect(hSession, hostName.c_str(), urlComp.nPort, 0);
  if (!hConnect) {
    response.errorMessage = "WinHttpConnect failed (Error: " + std::to_string(GetLastError()) + ")";
    WinHttpCloseHandle(hSession);
    return response;
  }

  // リクエストオープン
  std::wstring wideMethod = Utf8ToWide(method);
  DWORD openFlags = isHttps ? WINHTTP_FLAG_SECURE : 0;
  HINTERNET hRequest = WinHttpOpenRequest(
      hConnect,
      wideMethod.c_str(),
      urlPath.c_str(),
      nullptr,
      WINHTTP_NO_REFERER,
      WINHTTP_DEFAULT_ACCEPT_TYPES,
      openFlags);

  if (!hRequest) {
    response.errorMessage = "WinHttpOpenRequest failed (Error: " + std::to_string(GetLastError()) + ")";
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return response;
  }

  // ヘッダー追加
  std::wstring headerStr;
  for (const auto& h : headers) {
    headerStr += Utf8ToWide(h) + L"\r\n";
  }

  // リクエスト送信
  LPVOID pOptional = (body.empty()) ? nullptr : const_cast<char*>(body.data());
  DWORD optionalLength = static_cast<DWORD>(body.size());

  BOOL bSend = WinHttpSendRequest(
      hRequest,
      headerStr.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : headerStr.c_str(),
      static_cast<DWORD>(headerStr.length()),
      pOptional,
      optionalLength,
      optionalLength,
      0);

  if (!bSend) {
    response.errorMessage = "WinHttpSendRequest failed (Error: " + std::to_string(GetLastError()) + ")";
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return response;
  }

  // レスポンス待機
  if (!WinHttpReceiveResponse(hRequest, nullptr)) {
    response.errorMessage = "WinHttpReceiveResponse failed (Error: " + std::to_string(GetLastError()) + ")";
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return response;
  }

  // ステータスコード取得
  DWORD statusCode = 0;
  DWORD statusCodeSize = sizeof(statusCode);
  if (WinHttpQueryHeaders(
          hRequest,
          WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
          WINHTTP_HEADER_NAME_BY_INDEX,
          &statusCode,
          &statusCodeSize,
          WINHTTP_NO_HEADER_INDEX)) {
    response.statusCode = static_cast<int>(statusCode);
  }

  // ボディ読み出し
  std::string responseBody;
  DWORD dwSize = 0;
  DWORD dwDownloaded = 0;
  do {
    dwSize = 0;
    if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
      break;
    }
    if (dwSize == 0) {
      break;
    }

    std::vector<char> buffer(dwSize);
    if (WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded)) {
      responseBody.append(buffer.data(), dwDownloaded);
    } else {
      break;
    }
  } while (dwSize > 0);

  response.body = responseBody;
  response.success = (response.statusCode >= 200 && response.statusCode < 300);

  if (!response.success && response.errorMessage.empty()) {
    response.errorMessage = "HTTP " + std::to_string(response.statusCode);
  }

  WinHttpCloseHandle(hRequest);
  WinHttpCloseHandle(hConnect);
  WinHttpCloseHandle(hSession);

  return response;
}

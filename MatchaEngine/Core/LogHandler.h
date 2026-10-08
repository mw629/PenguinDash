#pragma once
#include <format>
#include <string>
#include <filesystem>
#include <vector>
#include <source_location>
#include <windows.h>
#include <dxgidebug.h>
#include <dbgHelp.h>
#include <strsafe.h>

#pragma comment(lib, "dbgHelp.lib")

// 前方宣言
struct ID3D12Device;

// ログレベル
enum class LogLevel {
    Info,
    Warning,
    Error,
    Fatal
};

// Stringに変換
std::string ConvertString(const std::wstring& str);
std::wstring ConvertString(const std::string& str);

// 現在時刻を取得してログファイルを開く（互換性維持）
std::ofstream CurrentTimestamp();

// グローバルログストリームの設定・取得
void SetGlobalLogStream(std::ostream* os);
std::ostream* GetGlobalLogStream();

// 各種クラッシュ・エラーハンドラの初期化
void InitializeErrorHandlers();

// ログ出力関数
void Log(const std::string& message);
void Log(std::ostream& os, const std::string& message);
void LogMessage(LogLevel level, const std::string& message, const std::source_location& loc = std::source_location::current());

// ログレベル別関数
void LogInfo(const std::string& message, const std::source_location& loc = std::source_location::current());
void LogWarning(const std::string& message, const std::source_location& loc = std::source_location::current());
void LogError(const std::string& message, const std::source_location& loc = std::source_location::current());
void LogFatal(const std::string& message, const std::source_location& loc = std::source_location::current());

// コールスタック取得
std::string GetStackTrace(int skipFrames = 1, int maxFrames = 32);
std::string GetStackTraceFromContext(const CONTEXT* context, int maxFrames = 32);

// HRESULT フォーマット & 検証
std::string FormatHResult(HRESULT hr);
void CheckHResult(HRESULT hr, const std::string& message, ID3D12Device* device = nullptr, const std::source_location& loc = std::source_location::current());

// ログの取得とクリア用 (Editor / UI用)
const std::vector<std::string>& GetLogs();
void ClearLogs();

// CrashHandler (SEH)
LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception);

// 便利マクロ
#define LOG_INFO(msg)    LogInfo(msg, std::source_location::current())
#define LOG_WARN(msg)    LogWarning(msg, std::source_location::current())
#define LOG_ERROR(msg)   LogError(msg, std::source_location::current())
#define LOG_FATAL(msg)   LogFatal(msg, std::source_location::current())

#define ENGINE_ASSERT(expr, msg) \
    do { \
        if (!(expr)) { \
            LogError(std::string("Assertion Failed: (") + #expr + ") - " + (msg), std::source_location::current()); \
            MessageBoxA(nullptr, (std::string("Assertion Failed:\n") + #expr + "\n\nMessage:\n" + (msg) + "\n\nLocation:\n" + __FILE__ + ":" + std::to_string(__LINE__)).c_str(), "Assertion Error", MB_OK | MB_ICONERROR); \
            __debugbreak(); \
        } \
    } while(0)

#define ENGINE_ASSERT_HR(hr, msg) \
    do { \
        HRESULT _hr_val = (hr); \
        if (FAILED(_hr_val)) { \
            CheckHResult(_hr_val, msg, nullptr, std::source_location::current()); \
            __debugbreak(); \
        } \
    } while(0)
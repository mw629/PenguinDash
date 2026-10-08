#include "LogHandler.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <mutex>
#include <algorithm>
#include <comdef.h>
#include <crtdbg.h>
#include <d3d12.h>
#include <dxgi.h>

namespace {
	std::mutex g_logMutex;
	std::ostream* g_globalLogStream = nullptr;
	std::ofstream g_fallbackFileStream;
	std::string g_currentLogFilePath;
	std::vector<std::string> g_logs;
	bool g_handlersInitialized = false;

	void EnsureFallbackFileStream() {
		if (g_globalLogStream != nullptr) return;
		if (g_fallbackFileStream.is_open()) return;

		std::filesystem::create_directories("logs");
		std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
		std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> nowSeconds =
			std::chrono::time_point_cast<std::chrono::seconds>(now);
		std::chrono::zoned_time localTime{ std::chrono::current_zone(), nowSeconds };
		std::string dateString = std::format("{:%Y%m%d_%H%M%S}", localTime);
		g_currentLogFilePath = "logs/" + dateString + ".log";
		g_fallbackFileStream.open(g_currentLogFilePath, std::ios::out | std::ios::app);
	}

	std::string GetCurrentTimestampString() {
		std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
		std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> nowSeconds =
			std::chrono::time_point_cast<std::chrono::seconds>(now);
		std::chrono::zoned_time localTime{ std::chrono::current_zone(), nowSeconds };
		return std::format("{:%Y-%m-%d %H:%M:%S}", localTime);
	}

	void InitSymbols() {
		static std::once_flag s_initOnce;
		std::call_once(s_initOnce, []() {
			HANDLE process = GetCurrentProcess();
			SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
			SymInitialize(process, NULL, TRUE);
		});
	}

	std::string GetExceptionCodeString(DWORD code) {
		switch (code) {
		case EXCEPTION_ACCESS_VIOLATION:         return "EXCEPTION_ACCESS_VIOLATION (0xC0000005)";
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED (0xC000008C)";
		case EXCEPTION_BREAKPOINT:               return "EXCEPTION_BREAKPOINT (0x80000003)";
		case EXCEPTION_DATATYPE_MISALIGNMENT:    return "EXCEPTION_DATATYPE_MISALIGNMENT (0x80000002)";
		case EXCEPTION_FLT_DENORMAL_OPERAND:     return "EXCEPTION_FLT_DENORMAL_OPERAND (0xC000008D)";
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:       return "EXCEPTION_FLT_DIVIDE_BY_ZERO (0xC000008E)";
		case EXCEPTION_FLT_INEXACT_RESULT:       return "EXCEPTION_FLT_INEXACT_RESULT (0xC000008F)";
		case EXCEPTION_FLT_INVALID_OPERATION:    return "EXCEPTION_FLT_INVALID_OPERATION (0xC0000090)";
		case EXCEPTION_FLT_OVERFLOW:             return "EXCEPTION_FLT_OVERFLOW (0xC0000091)";
		case EXCEPTION_FLT_STACK_CHECK:          return "EXCEPTION_FLT_STACK_CHECK (0xC0000092)";
		case EXCEPTION_FLT_UNDERFLOW:            return "EXCEPTION_FLT_UNDERFLOW (0xC0000093)";
		case EXCEPTION_ILLEGAL_INSTRUCTION:      return "EXCEPTION_ILLEGAL_INSTRUCTION (0xC000001D)";
		case EXCEPTION_IN_PAGE_ERROR:            return "EXCEPTION_IN_PAGE_ERROR (0xC0000006)";
		case EXCEPTION_INT_DIVIDE_BY_ZERO:       return "EXCEPTION_INT_DIVIDE_BY_ZERO (0xC0000094)";
		case EXCEPTION_INT_OVERFLOW:             return "EXCEPTION_INT_OVERFLOW (0xC0000095)";
		case EXCEPTION_INVALID_DISPOSITION:      return "EXCEPTION_INVALID_DISPOSITION (0xC0000026)";
		case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "EXCEPTION_NONCONTINUABLE_EXCEPTION (0xC0000025)";
		case EXCEPTION_PRIV_INSTRUCTION:         return "EXCEPTION_PRIV_INSTRUCTION (0xC0000096)";
		case EXCEPTION_SINGLE_STEP:              return "EXCEPTION_SINGLE_STEP (0x80000004)";
		case EXCEPTION_STACK_OVERFLOW:           return "EXCEPTION_STACK_OVERFLOW (0xC00000FD)";
		case 0xE06D7363:                         return "C++ Exception (0xE06D7363)";
		default:                                 return std::format("Unknown Exception (0x{:08X})", code);
		}
	}
}

std::string ConvertString(const std::wstring& str)
{
	if (str.empty()) {
		return std::string();
	}

	auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
	if (sizeNeeded == 0) {
		return std::string();
	}
	std::string result(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
	return result;
}

std::wstring ConvertString(const std::string& str) {
	if (str.empty()) {
		return std::wstring();
	}

	auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), NULL, 0);
	if (sizeNeeded == 0) {
		return std::wstring();
	}
	std::wstring result(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);
	return result;
}

void SetGlobalLogStream(std::ostream* os) {
	std::lock_guard<std::mutex> lock(g_logMutex);
	g_globalLogStream = os;
}

std::ostream* GetGlobalLogStream() {
	return g_globalLogStream;
}

void Log(const std::string& message) {
	std::lock_guard<std::mutex> lock(g_logMutex);

	OutputDebugStringA((message + "\n").c_str());
	std::cout << message << std::endl;

	if (g_globalLogStream && g_globalLogStream->good()) {
		*g_globalLogStream << message << std::endl;
		g_globalLogStream->flush();
	}
	else {
		EnsureFallbackFileStream();
		if (g_fallbackFileStream.is_open()) {
			g_fallbackFileStream << message << std::endl;
			g_fallbackFileStream.flush();
		}
	}

	g_logs.push_back(message);
}

void Log(std::ostream& os, const std::string& message) {
	if (g_globalLogStream == &os) {
		Log(message);
	}
	else {
		os << message << std::endl;
		Log(message);
	}
}

const std::vector<std::string>& GetLogs() {
	return g_logs;
}

void ClearLogs() {
	std::lock_guard<std::mutex> lock(g_logMutex);
	g_logs.clear();
}

std::ofstream CurrentTimestamp()
{
	std::filesystem::create_directories("logs");
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>
		nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
	std::chrono::zoned_time localTime{ std::chrono::current_zone(), nowSeconds };
	std::string dateString = std::format("{:%Y%m%d_%H%M%S}", localTime);
	std::string logFilePath = "logs/" + dateString + ".log";
	g_currentLogFilePath = logFilePath;

	std::ofstream logStream(logFilePath);
	return logStream;
}

std::string GetStackTrace(int skipFrames, int maxFrames) {
	InitSymbols();
	HANDLE process = GetCurrentProcess();

	void* stack[64]{};
	WORD captured = RtlCaptureStackBackTrace(skipFrames, (std::min)(maxFrames, 64), stack, NULL);

	std::ostringstream ss;
	alignas(SYMBOL_INFO) char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)]{};
	PSYMBOL_INFO symbol = reinterpret_cast<PSYMBOL_INFO>(buffer);
	symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	symbol->MaxNameLen = MAX_SYM_NAME;

	for (WORD i = 0; i < captured; ++i) {
		DWORD64 address = reinterpret_cast<DWORD64>(stack[i]);

		DWORD64 displacement = 0;
		std::string funcName = "UnknownFunction";
		if (SymFromAddr(process, address, &displacement, symbol)) {
			funcName = symbol->Name;
		}

		IMAGEHLP_LINE64 line{};
		line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
		DWORD lineDisplacement = 0;
		std::string fileAndLine = "";
		if (SymGetLineFromAddr64(process, address, &lineDisplacement, &line)) {
			fileAndLine = std::format(" - {}:{}", line.FileName, line.LineNumber);
		}

		ss << std::format("  [{:02d}] 0x{:016X}: {}{}\n", i, address, funcName, fileAndLine);
	}
	return ss.str();
}

std::string GetStackTraceFromContext(const CONTEXT* context, int maxFrames) {
	if (!context) return GetStackTrace(1, maxFrames);

	InitSymbols();
	HANDLE process = GetCurrentProcess();
	HANDLE thread = GetCurrentThread();

	CONTEXT ctxCopy = *context;

	STACKFRAME64 frame{};
	DWORD machineType = IMAGE_FILE_MACHINE_AMD64;
	frame.AddrPC.Offset = ctxCopy.Rip;
	frame.AddrPC.Mode = AddrModeFlat;
	frame.AddrFrame.Offset = ctxCopy.Rbp;
	frame.AddrFrame.Mode = AddrModeFlat;
	frame.AddrStack.Offset = ctxCopy.Rsp;
	frame.AddrStack.Mode = AddrModeFlat;

	std::ostringstream ss;
	alignas(SYMBOL_INFO) char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)]{};
	PSYMBOL_INFO symbol = reinterpret_cast<PSYMBOL_INFO>(buffer);
	symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	symbol->MaxNameLen = MAX_SYM_NAME;

	int frameIndex = 0;
	while (StackWalk64(machineType, process, thread, &frame, &ctxCopy, NULL,
		SymFunctionTableAccess64, SymGetModuleBase64, NULL) && frameIndex < maxFrames) {
		if (frame.AddrPC.Offset == 0) break;

		DWORD64 address = frame.AddrPC.Offset;
		DWORD64 displacement = 0;
		std::string funcName = "UnknownFunction";
		if (SymFromAddr(process, address, &displacement, symbol)) {
			funcName = symbol->Name;
		}

		IMAGEHLP_LINE64 line{};
		line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
		DWORD lineDisplacement = 0;
		std::string fileAndLine = "";
		if (SymGetLineFromAddr64(process, address, &lineDisplacement, &line)) {
			fileAndLine = std::format(" - {}:{}", line.FileName, line.LineNumber);
		}

		ss << std::format("  [{:02d}] 0x{:016X}: {}{}\n", frameIndex++, address, funcName, fileAndLine);
	}
	return ss.str();
}

void LogMessage(LogLevel level, const std::string& message, const std::source_location& loc) {
	const char* levelStr = "INFO";
	switch (level) {
	case LogLevel::Info:    levelStr = "INFO"; break;
	case LogLevel::Warning: levelStr = "WARN"; break;
	case LogLevel::Error:   levelStr = "ERROR"; break;
	case LogLevel::Fatal:   levelStr = "FATAL"; break;
	}

	std::filesystem::path p(loc.file_name());
	std::string filename = p.filename().string();
	std::string timestamp = GetCurrentTimestampString();

	std::string formatted = std::format("[{}] [{}] [{}:{}] ({}): {}",
		timestamp, levelStr, filename, loc.line(), loc.function_name(), message);

	if (level == LogLevel::Error || level == LogLevel::Fatal) {
		formatted += "\nCall Stack:\n" + GetStackTrace(2, 16);
	}

	Log(formatted);
}

void LogInfo(const std::string& message, const std::source_location& loc) {
	LogMessage(LogLevel::Info, message, loc);
}

void LogWarning(const std::string& message, const std::source_location& loc) {
	LogMessage(LogLevel::Warning, message, loc);
}

void LogError(const std::string& message, const std::source_location& loc) {
	LogMessage(LogLevel::Error, message, loc);
}

void LogFatal(const std::string& message, const std::source_location& loc) {
	LogMessage(LogLevel::Fatal, message, loc);
}

std::string FormatHResult(HRESULT hr) {
	_com_error err(hr);
	LPCTSTR errMsg = err.ErrorMessage();
	std::wstring wstr(errMsg ? errMsg : L"");
	std::string desc = ConvertString(wstr);

	std::string dxgiName = "";
	switch (hr) {
	case DXGI_ERROR_DEVICE_REMOVED:        dxgiName = "DXGI_ERROR_DEVICE_REMOVED: "; break;
	case DXGI_ERROR_DEVICE_HUNG:           dxgiName = "DXGI_ERROR_DEVICE_HUNG: "; break;
	case DXGI_ERROR_DEVICE_RESET:          dxgiName = "DXGI_ERROR_DEVICE_RESET: "; break;
	case DXGI_ERROR_DRIVER_INTERNAL_ERROR: dxgiName = "DXGI_ERROR_DRIVER_INTERNAL_ERROR: "; break;
	case DXGI_ERROR_INVALID_CALL:          dxgiName = "DXGI_ERROR_INVALID_CALL: "; break;
	case E_INVALIDARG:                     dxgiName = "E_INVALIDARG: "; break;
	case E_OUTOFMEMORY:                    dxgiName = "E_OUTOFMEMORY: "; break;
	case E_FAIL:                           dxgiName = "E_FAIL: "; break;
	default: break;
	}

	return std::format("0x{:08X} ({}{})", static_cast<uint32_t>(hr), dxgiName, desc);
}

void CheckHResult(HRESULT hr, const std::string& message, ID3D12Device* device, const std::source_location& loc) {
	if (SUCCEEDED(hr)) return;

	std::string errDetail = std::format("HRESULT Failure: {} - {}", FormatHResult(hr), message);
	if (device && (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_HUNG)) {
		HRESULT reason = device->GetDeviceRemovedReason();
		errDetail += std::format("\nDevice Removed Reason: {}", FormatHResult(reason));
	}

	LogError(errDetail, loc);

	std::string boxMsg = std::format("{}\n\nLocation: {}:{}\nFunction: {}",
		errDetail, loc.file_name(), loc.line(), loc.function_name());
	MessageBoxA(nullptr, boxMsg.c_str(), "DirectX Error", MB_OK | MB_ICONERROR);
}

LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception) {
	SYSTEMTIME time;
	GetLocalTime(&time);
	wchar_t filePath[MAX_PATH] = { 0 };
	CreateDirectory(L"./Dump", nullptr);
	StringCchPrintf(filePath, MAX_PATH, L"./Dump/%04d-%02d%02d-%02d%02d.dmp", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute);
	HANDLE dumpFileHandle = CreateFile(filePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
	if (dumpFileHandle != INVALID_HANDLE_VALUE) {
		DWORD processId = GetCurrentProcessId();
		DWORD threadId = GetCurrentThreadId();
		_MINIDUMP_EXCEPTION_INFORMATION minidumpInformation{ 0 };
		minidumpInformation.ThreadId = threadId;
		minidumpInformation.ExceptionPointers = exception;
		minidumpInformation.ClientPointers = TRUE;
		MiniDumpWriteDump(GetCurrentProcess(), processId, dumpFileHandle, MiniDumpNormal, &minidumpInformation, nullptr, nullptr);
		CloseHandle(dumpFileHandle);
	}

	std::string report = "\n================================================================\n";
	report += "CRASH DETECTED! (Unhandled Exception)\n";
	report += "================================================================\n";

	if (exception && exception->ExceptionRecord) {
		DWORD code = exception->ExceptionRecord->ExceptionCode;
		report += std::format("Exception Code:   {}\n", GetExceptionCodeString(code));
		report += std::format("Fault Address:    0x{:016X}\n", reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress));

		if (code == EXCEPTION_ACCESS_VIOLATION && exception->ExceptionRecord->NumberParameters >= 2) {
			ULONG_PTR accessType = exception->ExceptionRecord->ExceptionInformation[0];
			ULONG_PTR targetAddr = exception->ExceptionRecord->ExceptionInformation[1];
			std::string op = (accessType == 0) ? "Read from" : (accessType == 1 ? "Write to" : "Execute at");
			report += std::format("Violation Detail: Attempted to {} address 0x{:016X}{}\n",
				op, targetAddr, (targetAddr < 0x10000 ? " (Null pointer dereference!)" : ""));
		}
	}

	report += std::format("Dump File:        {}\n", ConvertString(filePath));
	report += "\nCall Stack:\n";
	if (exception && exception->ContextRecord) {
		report += GetStackTraceFromContext(exception->ContextRecord, 32);
	}
	else {
		report += GetStackTrace(1, 32);
	}
	report += "================================================================\n";

	Log(report);
	MessageBoxA(nullptr, report.c_str(), "Engine Crash Report", MB_OK | MB_ICONERROR);

	return EXCEPTION_EXECUTE_HANDLER;
}

#ifdef _DEBUG
static int __cdecl CustomCrtReportHook(int reportType, wchar_t* message, int* returnValue) {
	(void)returnValue;
	if (reportType == _CRT_ASSERT) {
		std::wstring wmsg = message ? message : L"";
		std::string smsg = ConvertString(wmsg);
		std::string stack = GetStackTrace(3, 20);

		std::string logMsg = std::format("\n========================================\n"
			"[CRT ASSERTION FAILED]\n{}\n"
			"Call Stack:\n{}"
			"========================================\n",
			smsg, stack);
		Log(logMsg);
	}
	return 0;
}
#endif

static void CustomInvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved) {
	(void)pReserved;
	std::string exprStr = expression ? ConvertString(expression) : "Unknown";
	std::string funcStr = function ? ConvertString(function) : "Unknown";
	std::string fileStr = file ? ConvertString(file) : "Unknown";
	std::string stack = GetStackTrace(2, 20);

	std::string logMsg = std::format("\n========================================\n"
		"[CRT INVALID PARAMETER]\n"
		"Expression: {}\n"
		"Function:   {}\n"
		"Location:   {}:{}\n"
		"Call Stack:\n{}"
		"========================================\n",
		exprStr, funcStr, fileStr, line, stack);
	Log(logMsg);
	MessageBoxA(nullptr, logMsg.c_str(), "CRT Invalid Parameter Error", MB_OK | MB_ICONERROR);
}

static void CustomPureCallHandler() {
	std::string stack = GetStackTrace(2, 20);
	std::string logMsg = "\n========================================\n"
		"[PURE VIRTUAL FUNCTION CALL]\n"
		"A pure virtual function was called!\n"
		"Call Stack:\n" + stack +
		"========================================\n";
	Log(logMsg);
	MessageBoxA(nullptr, logMsg.c_str(), "Purecall Error", MB_OK | MB_ICONERROR);
}

static void CustomTerminateHandler() {
	std::string stack = GetStackTrace(1, 20);
	std::string logMsg = "\n========================================\n"
		"[UNHANDLED C++ EXCEPTION / TERMINATE]\n"
		"std::terminate() was invoked.\n"
		"Call Stack:\n" + stack +
		"========================================\n";
	Log(logMsg);
	MessageBoxA(nullptr, logMsg.c_str(), "Terminate Error", MB_OK | MB_ICONERROR);
	std::abort();
}

void InitializeErrorHandlers() {
	if (g_handlersInitialized) return;
	g_handlersInitialized = true;

	SetUnhandledExceptionFilter(ExportDump);

#ifdef _DEBUG
	_CrtSetReportHookW2(_CRT_RPTHOOK_INSTALL, CustomCrtReportHook);
#endif

	_set_invalid_parameter_handler(CustomInvalidParameterHandler);
	_set_purecall_handler(CustomPureCallHandler);
	std::set_terminate(CustomTerminateHandler);
}

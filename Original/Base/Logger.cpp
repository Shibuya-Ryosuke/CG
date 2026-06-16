#include "Logger.h"
#include <filesystem>
#include <chrono>
#include <format>
#include <cassert>
#include <strsafe.h>
#include <DbgHelp.h>

#pragma comment(lib, "Dbghelp.lib")

namespace RyoEngine {

	std::ofstream Logger::logStream_;
	
	std::vector<std::string> Logger::logHistory_;
	std::mutex Logger::logMutex_;

	void Logger::Initialize() {
		// クラッシュハンドラ登録
		SetUnhandledExceptionFilter(ExportDump);

		// logsディレクトリ作成
		std::filesystem::create_directory("logs");

		// 現在時刻を取得(UTC時刻)
		std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
		// ログファイルの名前にコンマ何秒はいらないので、削って秒にする
		std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>
			nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
		// 日本時間(PCの設定時間)に変換
		std::chrono::zoned_time localTime{ std::chrono::current_zone(), nowSeconds };
		// formatを使って年月日_時分秒の文字列に変換
		std::string dateString = std::format("{:%Y%m%d_%H%M%S}", localTime);
		// 時刻を使ってファイル名を決定
		std::string logFilePath = std::string("logs/") + dateString + ".log";
		// あらかじめ作っておいたログファイルにパスを教えて準備完了
		logStream_.open(logFilePath);

		assert(logStream_.is_open());

		Log("Logger Initialized\n");
	}

	void Logger::Finalize() {
		Log("Logger Finalized\n");
		if (logStream_.is_open()) {
			logStream_.close();
		}
	}

	

	std::string Logger::ConvertString(const std::wstring& str) {
		if (str.empty()) return std::string();
		// 変換後のサイズを計算
		int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
		// 確保したサイズでstringを作成
		std::string result(static_cast<size_t>(sizeNeeded), 0);
		// 変換
		WideCharToMultiByte(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), &result[0], sizeNeeded, NULL, NULL);

		return result;
	}

	std::wstring Logger::ConvertString(const std::string& str) {
		if (str.empty()) return std::wstring();
		int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), NULL, 0);
		std::wstring result(static_cast<size_t>(sizeNeeded), 0);
		MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), &result[0], sizeNeeded);
		return result;
	}

	LONG WINAPI Logger::ExportDump(EXCEPTION_POINTERS* exception) noexcept {
		// 時刻を取得して、時刻を名前に居れたファイルを作成。Dumpsディレクトリ以下に出力
		SYSTEMTIME time;
		GetLocalTime(&time);
		wchar_t filePath[MAX_PATH] = { 0 };
		CreateDirectory(L"./Dumps", nullptr);
		StringCchPrintfW(filePath, MAX_PATH, L"./Dumps/%04d-%02d%02d-%02d%02d.dmp", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute);
		HANDLE dumpFileHandle = CreateFile(filePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
		// processId(このexeのId)とクラッシュ(例外)の発生したthreadIdを取得
		DWORD processId = GetCurrentProcessId();
		DWORD threadId = GetCurrentThreadId();
		// 設定情報を入力
		MINIDUMP_EXCEPTION_INFORMATION minidumpInformation{ 0 };
		minidumpInformation.ThreadId = threadId;
		minidumpInformation.ExceptionPointers = exception;
		minidumpInformation.ClientPointers = TRUE;
		// Dumpを出力。MiniDumpNormalは最低限の情報を出力するフラグ
		MiniDumpWriteDump(GetCurrentProcess(), processId, dumpFileHandle, MiniDumpNormal, &minidumpInformation, nullptr, nullptr);
		// 他に関連付けられているSEH例外ハンドラがあれば実行。通常はプロセスを終了する
		return EXCEPTION_EXECUTE_HANDLER;
	}
	void Logger::OutputLogMessage(const std::string& message) {
		// スレッドセーフにするためのロック
		std::lock_guard<std::mutex> lock(logMutex_);

		if (logStream_.is_open()) {
			logStream_ << message << std::endl;
		}
		OutputDebugStringA((message + "\n").c_str());

		// --- 追加: ImGui用のバッファに蓄積 ---
		logHistory_.push_back(message);

		// 古いログの削除（パフォーマンス維持のため）
		if (logHistory_.size() > MAX_LOG_LINES) {
			logHistory_.erase(logHistory_.begin());
		}
	}

	void Logger::Clear() {
		std::lock_guard<std::mutex> lock(logMutex_);
		logHistory_.clear();
	}
}
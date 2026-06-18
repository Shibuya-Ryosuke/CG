#pragma once
#include <string>
#include <fstream>
#include <Windows.h>
#include <vector>
#include <format>
#include <string_view>
#include <mutex>

namespace RyoEngine {

	class Logger {
	public:
		Logger() = delete;
		~Logger() = default;

		/// <summary>
		/// 初期化(ファイル準備とクラッシュハンドラ登録)
		/// </summary>
		static void Initialize();

		/// <summary>
		/// 終了
		/// </summary>
		static void Finalize();


		/// <summary>
		/// ログ出力
		/// </summary>
		/// <typeparam name="...Args"></typeparam>
		/// <param name="format">文章</param>
		/// <param name="...args">引数</param>
		template <typename... Args>
		static void Log(std::string_view format, Args&&... args) {
			std::string message = std::vformat(format, std::make_format_args(args...));

			OutputLogMessage(message);
		}

		/// <summary>
		/// ログ出力 (成功時に使用)
		/// </summary>
		/// <typeparam name="...Args"></typeparam>
		/// <param name="format">文章</param>
		/// <param name="...args">引数</param>
		template <typename... Args>
		static void LogSuccess(std::string_view format, Args&&... args) {
			Log("[Success] " + std::string(format), std::forward<Args>(args)...);
		}

		/// <summary>
		/// ログ出力 (警告時に使用)
		/// </summary>
		/// <typeparam name="...Args"></typeparam>
		/// <param name="format">文章</param>
		/// <param name="...args">引数</param>
		template <typename... Args>
		static void LogWarning(std::string_view format, Args&&... args) {
			Log("[Warning] " + std::string(format), std::forward<Args>(args)...);
		}

		/// <summary>
		/// ログ出力 (エラー時に使用)
		/// </summary>
		/// <typeparam name="...Args"></typeparam>
		/// <param name="format">文章</param>
		/// <param name="...args">引数</param>
		template <typename... Args>
		static void LogError(std::string_view format, Args&&... args) {
			Log("[Error] " + std::string(format), std::forward<Args>(args)...);
		}

		/// <summary>
		/// std::wstringからstd::stringへ変換
		/// </summary>
		/// <param name="str">変換したいwstring型</param>
		/// <returns>string型</returns>
		static std::string ConvertString(const std::wstring& str);

		/// <summary>
		/// std::stringからstd::wstringへ変換
		/// </summary>
		/// <param name="str">変換したいstring型</param>
		/// <returns>wstring型</returns>
		static std::wstring ConvertString(const std::string& str);

		/// <summary>
		/// MiniDumpを出力
		/// </summary>
		/// <param name="exception"></param>
		/// <returns></returns>
		static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception) noexcept;


		static const std::vector<std::string>& GetLogHistory() {
			return logHistory_;
		}

		static std::mutex& GetMutex() {
			return logMutex_;
		}

		static void Clear();

	private:
		static std::ofstream logStream_;

		static std::vector<std::string> logHistory_;
		static std::mutex logMutex_;
		static const size_t MAX_LOG_LINES = 500;

		static void OutputLogMessage(const std::string& message);
	};
}
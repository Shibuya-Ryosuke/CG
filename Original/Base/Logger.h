#pragma once
#include <string>
#include <fstream>
#include <Windows.h>
#include <format>
#include <string_view>

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
		/// <param name="message">書き出される文字</param>
		template <typename... Args>
		static void Log(std::string_view format, Args&&... args) {
			std::string message = std::vformat(format, std::make_format_args(args...));

			OutputLogMessage(message);
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

	private:
		static std::ofstream logStream_;

		static void OutputLogMessage(const std::string& message);
	};
}
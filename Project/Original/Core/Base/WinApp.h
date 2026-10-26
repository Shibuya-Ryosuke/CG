#pragma once
#include <Windows.h>
#include <cstdint>

namespace RyoEngine {

	class WinApp {
	public:
		// ウィンドウプロシージャ
		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

		/// <summary>
		/// 初期化
		/// </summary>
		/// <param name="title">ウィンドウに表示させる文字</param>
		/// <param name="width">ウィンドウの横幅</param>
		/// <param name="height">ウィンドウの縦幅</param>
		static void Initialize(const wchar_t* title, int32_t width = 1280, int32_t height = 720);

		// メッセージ処理
		static bool ProcessMessage();

		// 終了
		static void Finalize();


		// ゲッター
		static HWND GetHwnd() { return GetInstance()->hwnd_; }
		static HINSTANCE GetHInstance() { return GetInstance()->wc_.hInstance; };

	private:
		// インスタンスの取得
		static WinApp* GetInstance();

		WinApp() = default;
		~WinApp() = default;
		WinApp(const WinApp&) = delete;
		WinApp& operator=(const WinApp&) = delete;

		HWND hwnd_ = nullptr;
		WNDCLASS wc_{};
	};
}
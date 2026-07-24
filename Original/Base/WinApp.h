#pragma once
#include <Windows.h>
#include <cstdint>

namespace RyoEngine {

	class WinApp {
	public:
		// ウィンドウプロシージャ
		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

		// インスタンスの取得
		static WinApp* GetInstance();

		/// <summary>
		/// 初期化
		/// </summary>
		/// <param name="title">ウィンドウに表示させる文字</param>
		/// <param name="width">ウィンドウの横幅</param>
		/// <param name="height">ウィンドウの縦幅</param>
		void Initialize(const wchar_t* title, int32_t width = 1280, int32_t height = 720);

		// メッセージ処理
		bool ProcessMessage();

		// 終了
		void Finalize();


		// ゲッター
		HWND GetHwnd() const { return hwnd_; };
		HINSTANCE GetHInstance() const { return wc_.hInstance; };

	private:
		WinApp() = default;
		~WinApp() = default;
		WinApp(const WinApp&) = delete;
		WinApp& operator=(const WinApp&) = delete;

		HWND hwnd_ = nullptr;
		WNDCLASS wc_{};
	};
}
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
		/// <param name="width">ウィンドウの横幅(Debug/Developmentで最大化を解除したときの大きさ)</param>
		/// <param name="height">ウィンドウの縦幅(Debug/Developmentで最大化を解除したときの大きさ)</param>
		/// NOTE: Debug/Developmentは枠付き・サイズ変更可能なウィンドウを最大化で起動する。
		///       Releaseは枠なしでモニター全体を覆う(ボーダーレスフルスクリーン)。
		static void Initialize(const wchar_t* title, int32_t width = 1920, int32_t height = 1080);

		// メッセージ処理
		static bool ProcessMessage();

		// 終了
		static void Finalize();


		// ゲッター
		static HWND GetHwnd() { return GetInstance()->hwnd_; }
		static HINSTANCE GetHInstance() { return GetInstance()->wc_.hInstance; };
		// 現在のクライアント領域の大きさ(ウィンドウのサイズ変更に追従する。最小化中は最後の大きさのまま)
		static int32_t GetClientWidth() { return GetInstance()->clientWidth_; }
		static int32_t GetClientHeight() { return GetInstance()->clientHeight_; }

	private:
		// インスタンスの取得
		static WinApp* GetInstance();

		WinApp() = default;
		~WinApp() = default;
		WinApp(const WinApp&) = delete;
		WinApp& operator=(const WinApp&) = delete;

		HWND hwnd_ = nullptr;
		WNDCLASS wc_{};
		int32_t clientWidth_ = 0;   // WM_SIZEで更新される
		int32_t clientHeight_ = 0;
	};
}
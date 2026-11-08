#include "WinApp.h"
#include "Logger.h"
#include <d3d12.h>
#include <wrl.h> // これが ComPtr に必要です
#pragma comment(lib, "d3d12.lib")

// ImGui使いますよー
#ifdef NRELEASE
#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

namespace RyoEngine {

	LRESULT WinApp::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
        #ifdef NRELEASE
		if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
			return true;
		}
        #endif

		// メッセージに応じてゲーム固有の処理を行う
		switch (msg) {
			// ウィンドウが破棄された
		case WM_DESTROY:
			// OSに対して、アプリの終了を伝える
			PostQuitMessage(0);
			return 0;
			// ウィンドウのサイズが変わった(最大化・ドラッグでのサイズ変更など)
		case WM_SIZE:
			// 最小化中はクライアント領域が0x0になるので、最後の大きさのままにしておく
			if (wparam != SIZE_MINIMIZED) {
				GetInstance()->clientWidth_ = static_cast<int32_t>(LOWORD(lparam));
				GetInstance()->clientHeight_ = static_cast<int32_t>(HIWORD(lparam));
			}
			break;
		}

		// 標準のメッセージ処理を行う
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}

	WinApp* WinApp::GetInstance() {
		static WinApp instance;
		return &instance;
	}

	void WinApp::Initialize(const wchar_t* title, int32_t width, int32_t height) {
		WinApp* instance = GetInstance();

		Logger::Log("WinApp : Initializing...\n");

		// ウィンドウプロシージャ
		instance->wc_.lpfnWndProc = WindowProc;
		// ウィンドウクラス名(何でもいい)
		instance->wc_.lpszClassName = L"OriginalEngine";
		// インスタンスハンドル
		instance->wc_.hInstance = GetModuleHandle(nullptr);
		// カーソル
		instance->wc_.hCursor = LoadCursor(nullptr, IDC_ARROW);
		// ウィンドウクラスを登録する
		RegisterClass(&instance->wc_);

		// ウィンドウサイズを表す構造体にクライアント領域を入れる
		RECT wrc = { 0,0,width,height };

#ifdef NRELEASE
		// エディタ用：枠付き・サイズ変更可能なウィンドウ(このあと最大化で表示する)
		const DWORD windowStyle = WS_OVERLAPPEDWINDOW;
		// クライアント領域をもとに実際のサイズにwrcを変更してもらう
		AdjustWindowRect(&wrc, windowStyle, false);
		const int windowX = CW_USEDEFAULT;   // 表示位置はWindowsに任せる
		const int windowY = CW_USEDEFAULT;
#else
		// Release：枠なしでメインモニター全体を覆う(ボーダーレスフルスクリーン)
		// スワップチェーンはゲーム解像度のまま作り、モニターとの大きさの差はDXGIに引き伸ばしてもらう
		const DWORD windowStyle = WS_POPUP;
		MONITORINFO monitorInfo{};
		monitorInfo.cbSize = sizeof(monitorInfo);
		GetMonitorInfo(MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY), &monitorInfo);
		wrc = monitorInfo.rcMonitor;
		const int windowX = wrc.left;
		const int windowY = wrc.top;
#endif

		// ウィンドウの生成
		instance->hwnd_ = CreateWindow(
			instance->wc_.lpszClassName,        // 利用するクラス名
			title,                   // タイトルバーの文字
			windowStyle,             // ウィンドウスタイル(Debug/Development:枠付き / Release:枠なし)
			windowX,                 // 表示X座標
			windowY,                 // 表示Y座標
			wrc.right - wrc.left,    // ウィンドウ横幅
			wrc.bottom - wrc.top,    // ウィンドウ縦幅
			nullptr,                 // 親ウィンドウハンドル
			nullptr,                 // メニューハンドル
			instance->wc_.hInstance,            // インスタンスハンドル
			nullptr                  // オプション
		);

        #ifdef _DEBUG
		// デバッグレイヤーの実装
		Microsoft::WRL::ComPtr<ID3D12Debug1> debugController = nullptr;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
			// デバッグレイヤーを有効化する
			debugController->EnableDebugLayer();
			// さらにGPU側でもチェックを行うようにする
			debugController->SetEnableGPUBasedValidation(TRUE);
		}
        #endif

		if (!instance->hwnd_) {
			Logger::Log("WinApp: Failed to create window.\n");
			return;
		}

		// ウィンドウを表示する(Debug/Developmentはエディタとして使いやすいよう最大化する)
#ifdef NRELEASE
		ShowWindow(instance->hwnd_, SW_MAXIMIZE);
#else
		ShowWindow(instance->hwnd_, SW_SHOW);
#endif

		// 表示後の実際のクライアント領域を記録しておく(以降はWM_SIZEで追従する)
		RECT clientRect{};
		GetClientRect(instance->hwnd_, &clientRect);
		instance->clientWidth_ = static_cast<int32_t>(clientRect.right - clientRect.left);
		instance->clientHeight_ = static_cast<int32_t>(clientRect.bottom - clientRect.top);
		Logger::LogSuccess("WinApp : Initialized\n");
	}

	bool WinApp::ProcessMessage() {
		MSG msg{};
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
			if (msg.message == WM_QUIT) {
				return false;
			}
		}
		return true;
	}

	void WinApp::Finalize() {
		WinApp* instance = GetInstance();

		Logger::Log("WinApp : Finalizing...\n");
		UnregisterClass(instance->wc_.lpszClassName, instance->wc_.hInstance);
		Logger::LogSuccess("WinApp : Finalized\n");
	}
	
}
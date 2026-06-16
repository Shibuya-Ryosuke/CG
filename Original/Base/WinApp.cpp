#include "WinApp.h"
#include "Logger.h"
#include <d3d12.h>
#include <wrl.h> // これが ComPtr に必要です
#pragma comment(lib, "d3d12.lib")

// ImGui使いますよー
#ifdef USE_IMGUI
#include "../../Original/Externals/imgui/imgui.h"
#include "../../Original/Externals/imgui/imgui_impl_dx12.h"
#include "../../Original/Externals/imgui/imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

namespace RyoEngine {

	LRESULT WinApp::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
        #ifdef USE_IMGUI
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
		}

		// 標準のメッセージ処理を行う
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}

	WinApp* WinApp::GetInstance() {
		static WinApp instance;
		return &instance;
	}

	void WinApp::Initialize(const wchar_t* title, int32_t width, int32_t height) {
		Logger::Log("WinApp : Initializing...\n");

		// ウィンドウプロシージャ
		wc_.lpfnWndProc = WindowProc;
		// ウィンドウクラス名(何でもいい)
		wc_.lpszClassName = title;
		// インスタンスハンドル
		wc_.hInstance = GetModuleHandle(nullptr);
		// カーソル
		wc_.hCursor = LoadCursor(nullptr, IDC_ARROW);
		// ウィンドウクラスを登録する
		RegisterClass(&wc_);

		// ウィンドウサイズを表す構造体にクライアント領域を入れる
		RECT wrc = { 0,0,width,height };

		// クライアント領域をもとに実際のサイズにwrcを変更してもらう
		AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

		// ウィンドウの生成
		hwnd_ = CreateWindow(
			wc_.lpszClassName,        // 利用するクラス名
			L"CG2_シブヤ",            // タイトルバーの文字(何でも良い)
			WS_OVERLAPPEDWINDOW,     // よく見るウィンドウスタイル
			CW_USEDEFAULT,           // 表示X座標(Windowsに任せる)
			CW_USEDEFAULT,           // 表示Y座標(WindowsOSに任せる)
			wrc.right - wrc.left,    // ウィンドウ横幅
			wrc.bottom - wrc.top,    // ウィンドウ縦幅
			nullptr,                 // 親ウィンドウハンドル
			nullptr,                 // メニューハンドル
			wc_.hInstance,            // インスタンスハンドル
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

		if (!hwnd_) {
			Logger::Log("WinApp: Failed to create window.\n");
			return;
		}

		// ウィンドウを表示する
		ShowWindow(hwnd_, SW_SHOW);
		Logger::Log("WinApp : Initialized\n");
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
		Logger::Log("WinApp : Finalizing...\n");
		UnregisterClass(wc_.lpszClassName, wc_.hInstance);
		Logger::Log("WinApp : Finalized\n");
	}
	
}
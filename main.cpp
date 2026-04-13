#pragma warning(push)
// 自コード以外の警告を無視
#pragma warning(disable:4668)
#pragma warning(disable:4865)
#pragma warning(disable:5039)
#include <Windows.h>
#pragma warning(pop)

#include<cstdint>
#include<string>
#include<format>

// ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
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

// 出力ウィンドウに文字を出す
void Log(const std::string& message) {
	OutputDebugStringA(message.c_str());
}

// std::wstringからstd::stringへ変換
std::string ConvertString(const std::wstring& str) {
	if (str.empty()) return std::string();
	// 変換後のサイズを計算
	int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
	// 確保したサイズでstringを作成
	std::string result(static_cast<size_t>(sizeNeeded), 0);
	// 変換
	WideCharToMultiByte(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), &result[0], sizeNeeded, NULL, NULL);

	return result;
}

// std::stringからstd::wstringへ変換
std::wstring ConvertString(const std::string& str) {
	if (str.empty()) return std::wstring();
	int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), NULL, 0);
	std::wstring result(static_cast<size_t>(sizeNeeded), 0);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), &result[0], sizeNeeded);
	return result;
}

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	WNDCLASS wc{};
	// ウィンドウプロシージャ
	wc.lpfnWndProc = WindowProc;
	// ウィンドウクラス名(何でもいい)
	wc.lpszClassName = L"CG2WindowClass";
	// インスタンスハンドル
	wc.hInstance = GetModuleHandle(nullptr);
	// カーソル
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

	// ウィンドウクラスを登録する
	RegisterClass(&wc);

	// クライアント領域のサイズ
	const int32_t kClientWidth = 1280;
	const int32_t kClientHeight = 720;

	// ウィンドウサイズを表す構造体にクライアント領域を入れる
	RECT wrc = { 0,0,kClientWidth,kClientHeight };

	// クライアント領域をもとに実際のサイズにwrcを変更してもらう
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// ウィンドウの生成
	HWND hwnd = CreateWindow(
		wc.lpszClassName,        // 利用するクラス名
		L"CG2_シブヤ",            // タイトルバーの文字(何でも良い)
		WS_OVERLAPPEDWINDOW,     // よく見るウィンドウスタイル
		CW_USEDEFAULT,           // 表示X座標(Windowsに任せる)
		CW_USEDEFAULT,           // 表示Y座標(WindowsOSに任せる)
		wrc.right - wrc.left,    // ウィンドウ横幅
		wrc.bottom - wrc.top,    // ウィンドウ縦幅
		nullptr,                 // 親ウィンドウハンドル
		nullptr,                 // メニューハンドル
		wc.hInstance,            // インスタンスハンドル
		nullptr                  // オプション
	);

	// ウィンドウを表示する
	ShowWindow(hwnd, SW_SHOW);

	// 文字列を格納
	std::string str1{ "HAPPY" };
	// 整数を文字列にする
	std::string str2{ std::to_string(100) };
	// 出力ウィンドウに表示
	Log(std::format("string1:{}, string2:{}\n", str1, str2));

	// wstringバージョン
	std::wstring wstringValue = { std::to_wstring(500) };
	Log(ConvertString(std::format(L"WSTRING{}\n", wstringValue)));
	
	MSG msg{};
	// ウィンドウの×ボタンが押されるまでループ
	while (msg.message != WM_QUIT) {
		// Windowにメッセージが来てたら最優先で処理させる
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} else {
			// ゲームの処理

		}
	}
	return 0;
}
#pragma warning(push)
// 自コード以外の警告を無視
#pragma warning(disable:4668)
#pragma warning(disable:4865)
#pragma warning(disable:5039)
#include <Windows.h>
#pragma warning(pop)

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	// 出力ウィンドウへの文字出力
	OutputDebugStringA("Hello,DirectX!\n");

	return 0;
}
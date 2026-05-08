#pragma once
#include <Windows.h>
#include "../Externals/imgui/imgui.h"
#include "../Externals/imgui/imgui_impl_dx12.h"
#include "../Externals/imgui/imgui_impl_win32.h"

namespace Engine {

	extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

}

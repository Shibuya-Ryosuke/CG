#pragma once
#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cstdint>

namespace RyoEngine {

    class ImGuiManager {
    public:
#ifdef NRELEASE
        // SrvManager::Initialize()より後に呼ぶこと(フォント用のSRVスロットをSrvManagerから確保するため)
        static void Initialize(HWND hwnd, ID3D12Device* device, int bufferCount, DXGI_FORMAT rtvFormat);
        // SrvManager::Finalize()より前に呼ぶこと
        static void Finalize();
        static void NewFrame();
        static void EndFrame(ID3D12GraphicsCommandList* commandList);
#else
        // Debug側と同じ名前にしておく(呼び出し側を#ifdefで囲まなくても済むように)
        static void Initialize(HWND, ID3D12Device*, int, DXGI_FORMAT) {}
        static void Finalize() {}
        static void NewFrame() {}
        static void EndFrame(ID3D12GraphicsCommandList*) {}
#endif

    private:
        static ImGuiManager* GetInstance();

        ImGuiManager() = default;
        ~ImGuiManager() = default;
        ImGuiManager(const ImGuiManager&) = delete;
        ImGuiManager& operator=(const ImGuiManager&) = delete;

        uint32_t fontSrvIndex_ = 0;  // フォントテクスチャ用にSrvManagerから確保したスロット

        // imgui.iniが無いとき(初回起動・iniを消したとき)、最初のフレームで初期配置を作る
        bool needDefaultLayout_ = false;
        // Game Viewを中央、各パネルを右と下にドッキングした初期配置を作る (dockspaceIdはImGuiID)
        static void BuildDefaultLayout(uint32_t dockspaceId);
    };
}
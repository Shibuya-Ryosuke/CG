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
#ifdef _DEBUG
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
    };
}
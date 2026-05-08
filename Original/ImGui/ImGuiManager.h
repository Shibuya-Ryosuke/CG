#pragma once
#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

namespace Engine {

    class ImGuiManager {
    public:
#ifdef _DEBUG
        static void Initialize(HWND hwnd, ID3D12Device* device, int bufferCount, DXGI_FORMAT rtvFormat);
        static void Finalize();
        static void Begin();
        static void End(ID3D12GraphicsCommandList* commandList);
#else
        static void Initialize(HWND, ID3D12Device*, int, DXGI_FORMAT) {}
        static void Finalize() {}
        static void Begin() {}
        static void End(ID3D12GraphicsCommandList*) {}
#endif

    private:
        static ImGuiManager* GetInstance();

        ImGuiManager() = default;
        ~ImGuiManager() = default;
        ImGuiManager(const ImGuiManager&) = delete;
        ImGuiManager& operator=(const ImGuiManager&) = delete;

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap_;
    };
}

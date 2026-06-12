#include "ImGuiManager.h"

#ifdef _DEBUG

#include"../Externals/imgui/imgui.h"
#include"../Externals/imgui/imgui_impl_dx12.h"
#include"../Externals/imgui/imgui_impl_win32.h"


namespace RyoEngine {
    ImGuiManager* ImGuiManager::GetInstance() {
        static ImGuiManager instance;
        return &instance;
    }

    void ImGuiManager::Initialize(HWND hwnd, ID3D12Device* device, int bufferCount, DXGI_FORMAT rtvFormat) {
        ImGuiManager* instance = GetInstance();

        // 1. SRVヒープの作成
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = 1;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(instance->srvHeap_.GetAddressOf()));

        // 2. ImGuiコンテキスト作成
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        // 3. 各種初期化
        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX12_Init(
            device,
            bufferCount,
            rtvFormat,
            instance->srvHeap_.Get(),
            instance->srvHeap_->GetCPUDescriptorHandleForHeapStart(),
            instance->srvHeap_->GetGPUDescriptorHandleForHeapStart()
        );

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    }

    void ImGuiManager::NewFrame() {
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport();
    }

    void ImGuiManager::EndFrame(ID3D12GraphicsCommandList* commandList) {
        ImGuiManager* instance = GetInstance();

        ImGui::Render();

        // DescriptorHeapのセット
        ID3D12DescriptorHeap* heaps[] = { instance->srvHeap_.Get() };
        commandList->SetDescriptorHeaps(_countof(heaps), heaps);

        // 描画コマンド発行
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
    }

    void ImGuiManager::Finalize() {
        ImGuiManager* instance = GetInstance();

        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        
        instance->srvHeap_.Reset();
    }
}
#endif
#include "ImGuiManager.h"
#include "../Graphics/TextureManager.h"
#include "../Base/Logger.h"

#ifdef _DEBUG

#include "ImGuiAllInclude.h"


namespace RyoEngine {
    ImGuiManager* ImGuiManager::GetInstance() {
        static ImGuiManager instance;
        return &instance;
    }

    void ImGuiManager::Initialize(HWND hwnd, ID3D12Device* device, int bufferCount, DXGI_FORMAT rtvFormat) {
        Logger::Log("ImGuiManager : Initializing...\n");
        // 2. ImGuiコンテキスト作成
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        // 3. 各種初期化
        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX12_Init(
            device,
            bufferCount,
            rtvFormat,
            TextureManager::GetInstance()->GetDescriptorHeap(),
            TextureManager::GetInstance()->GetCPUHandle(0),
            TextureManager::GetInstance()->GetGPUHandle(0)
        );

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        Logger::LogSuccess("ImGuiManager : Initialized\n");
    }

    void ImGuiManager::NewFrame() {
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport();
    }

    void ImGuiManager::EndFrame(ID3D12GraphicsCommandList* commandList) {
        ImGui::Render();

        // DescriptorHeapのセット
        ID3D12DescriptorHeap* ppHeaps[] = { TextureManager::GetInstance()->GetDescriptorHeap()};
        commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

        // 描画コマンド発行
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
    }

    void ImGuiManager::Finalize() {
        Logger::Log("ImGuiManager : Finalizing...\n");
        ImGuiManager* instance = GetInstance();

        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        
        instance->srvHeap_.Reset();
        Logger::LogSuccess("ImGuiManager : Finalized\n");
    }
}
#endif
#include "ImGuiManager.h"
#include "../Graphics/2D/TextureManager.h"
#include "../../Core/Base/Logger.h"

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

        ImFontConfig config;
        config.MergeMode = false;

        const char* ttcPath = "C:\\Windows\\Fonts\\msgothic.ttc";

        ImFont* font = io.Fonts->AddFontFromFileTTF(ttcPath, 13.0f, &config, io.Fonts->GetGlyphRangesJapanese());
        if (font == nullptr) {
            std::string errorMsg = "Cannot load the ttc file.\nPath searched for: " + std::string(ttcPath);
            Logger::LogError(errorMsg);
        }
        Logger::LogSuccess("ImGuiManager : Initialized\n");
    }

    void ImGuiManager::NewFrame() {
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport();
        ImGuizmo::BeginFrame();
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
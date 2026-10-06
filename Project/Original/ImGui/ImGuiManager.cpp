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

        ImVec4* colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.04f, 0.04f, 0.06f, 0.85f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.11f, 0.11f, 0.14f, 0.92f);
        colors[ImGuiCol_Border] = ImVec4(0.50f, 0.50f, 0.50f, 0.50f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.43f, 0.43f, 0.43f, 0.39f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.47f, 0.47f, 0.69f, 0.40f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.42f, 0.41f, 0.64f, 0.69f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.27f, 0.27f, 0.54f, 0.83f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.32f, 0.32f, 0.63f, 0.87f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.40f, 0.40f, 0.80f, 0.20f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.40f, 0.40f, 0.55f, 0.80f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.20f, 0.25f, 0.30f, 0.60f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.40f, 0.40f, 0.80f, 0.30f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.80f, 0.40f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.41f, 0.39f, 0.80f, 0.60f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.00f, 1.00f, 0.85f, 0.50f);
        colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.41f, 0.39f, 0.80f, 0.60f);
        colors[ImGuiCol_Button] = ImVec4(0.35f, 0.40f, 0.61f, 0.62f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.40f, 0.48f, 0.71f, 0.79f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.46f, 0.54f, 0.80f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.40f, 0.40f, 0.90f, 0.45f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.45f, 0.45f, 0.90f, 0.80f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.53f, 0.53f, 0.87f, 0.80f);
        colors[ImGuiCol_Separator] = ImVec4(0.50f, 0.50f, 0.50f, 0.60f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.60f, 0.60f, 0.70f, 1.00f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.70f, 0.70f, 0.90f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.78f, 0.82f, 1.00f, 0.60f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.78f, 0.82f, 1.00f, 0.90f);
        colors[ImGuiCol_Tab] = ImVec4(0.34f, 0.34f, 0.68f, 0.79f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.45f, 0.45f, 0.90f, 0.80f);
        colors[ImGuiCol_TabActive] = ImVec4(0.40f, 0.40f, 0.73f, 0.84f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.28f, 0.28f, 0.57f, 0.82f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.35f, 0.35f, 0.65f, 0.84f);
        colors[ImGuiCol_DockingPreview] = ImVec4(0.40f, 0.40f, 0.90f, 0.31f);
        colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.27f, 0.27f, 0.38f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.45f, 1.00f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.26f, 0.26f, 0.28f, 1.00f);
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.07f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.45f, 0.45f, 0.90f, 0.80f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);








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
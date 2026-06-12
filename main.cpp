#include "./Original/RyoEngine.h"
#ifdef _DEBUG
#include "Original/Externals/imgui/imgui.h"
#endif

using namespace RyoEngine;

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    // エンジン初期化
    RyoEngine::Initialize();

    uint32_t tex = LoadTex("resources/flower.png");
    // --- メインループ ---
    while (GetWinApp()->ProcessMessage()) {
        // フレーム開始
        NewFrame();

        // ----------------------
        // -- 更新処理（Update） --
        // ----------------------

        ImGui::ShowDemoWindow();
        ImGui::Begin("lll");
        uint32_t gameTexHandle = GetDxCommon()->GetGameTexHandle();

        ImTextureID gameTexID = (ImTextureID)GetTexManager()->GetGPUHandle(gameTexHandle).ptr;
        ImGui::Image(gameTexID, ImVec2(500.0f, 500.0f), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), ImVec4(1.0f, 1.0f, 1.0f, 1.0f), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        ImGui::End();
        
        ImGui::Begin("a");
        ImGui::Image((ImTextureID)GetTexManager()->GetGPUHandle(tex).ptr, ImVec2(500.0f, 500.0f), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), ImVec4(1.0f, 1.0f, 1.0f, 1.0f), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        ImGui::End();

        // ----------------------
        // ------ 更新終了 -------
        // ----------------------
        




        // --------------------------------------------------------------------------------
        // Reflect
       
        
        // --------------------------------------------------------------------------------





        // ----------------------
        // --- 描画処理 (Draw) ---
        // ----------------------
        // [3D描画フェーズ]
        Begin3dDraw();
        


        // 3D終了----------------------------------------------------
        


        // [2D描画フェーズ]
        Begin2dDraw();



        // 2D終了----------------------------------------------------
        


        // ----------------------
        // ------ 描画終了 -------
        // ----------------------

        // フレーム終了
        EndFrame();
    }
    
    // エンジン終了
    RyoEngine::Finalize();

    return 0;
}
#include "./Original/RyoEngine.h"
#ifdef _DEBUG
#include "Original/Externals/imgui/imgui.h"
#endif

using namespace RyoEngine;

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    // エンジン初期化
    RyoEngine::Initialize();

    // --- メインループ ---
    while (GetWinApp()->ProcessMessage()) {
        // 入力受付とImGuiフレーム開始
        Input::Update();
        ImGuiManager::NewFrame();

        // ----------------------
        // -- 更新処理（Update） --
        // ----------------------



        // ----------------------
        // ------ 更新終了 -------
        // ----------------------
        




        // --------------------------------------------------------------------------------
        // Reflect
       
        
        // --------------------------------------------------------------------------------





        // ----------------------
        // --- 描画処理 (Draw) ---
        // ----------------------
        GetDxCommon()->PreDraw();

        // [3D描画フェーズ]
        GetModelCommon()->BeginDraw();
        


        // 3D終了----------------------------------------------------
        


        // [2D描画フェーズ]
        GetSpriteCommon()->BeginDraw();



        // 2D終了----------------------------------------------------
        


        // ----------------------
        // ------ 描画終了 -------
        // ----------------------
        // ImGuiフレーム終了
        ImGuiManager::EndFrame(GetDxCommon()->GetCommandList());
        // 画面表示（PostDraw、コマンドリスト実行、スワップチェーン入れ替え）
        GetDxCommon()->PostDraw();
    }
    
    // エンジン終了
    RyoEngine::Finalize();

    return 0;
}
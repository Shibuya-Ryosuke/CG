#include "./Original/RyoEngine.h"
#ifdef _DEBUG
#include "Original/ImGui/ImGuiAllInclude.h"
#endif

using namespace RyoEngine;

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    // エンジン初期化
    RyoEngine::Initialize();

    DebugCamera* debugCamera = new DebugCamera();
    debugCamera->SetTranslate({0.0f,1.0f, -40.0f });

    // 使うときだけ
    //AnimEdit::Initialize();
    //AnimEdit::LoadSettings();

    // --- メインループ ---
    while (GetWinApp()->ProcessMessage()) {
        // フレーム開始
        NewFrame();

        // ----------------------
        // -- 更新処理（Update） --
        // ----------------------
        
        // AnimEdit::Update();

        debugCamera->SetAvailable(RyoEngine::GetOnTheGameView());
        debugCamera->Update();

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
    
    delete debugCamera;
    // エンジン終了
    RyoEngine::Finalize();

    return 0;
}
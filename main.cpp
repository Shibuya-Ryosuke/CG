#include "./Original/RyoEngine.h"
#ifdef _DEBUG
#include "Original/ImGui/ImGuiAllInclude.h"
#endif

using namespace RyoEngine;

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    // エンジン初期化
    RyoEngine::Initialize();

    // カメラ
    DebugCamera* debugCamera = new DebugCamera();
    debugCamera->SetTranslate({ 0.0f,0.3f,-9.2f });

    // 球
    Mesh sphere;
    sphere.CreateSphere({ 0.0f,0.0f }, 24);
    sphere.SetTranslate({ 0.0f,0.0f,0.0f });
    sphere.SetTex("resources/uvChecker.png");
    
    // --- メインループ ---
    while (GetWinApp()->ProcessMessage()) {
        // フレーム開始
        NewFrame();

        // ----------------------
        // -- 更新処理（Update） --
        // ----------------------
        
        // 各種更新
        debugCamera->SetAvailable(RyoEngine::GetOnTheGameView());
        debugCamera->Update();

        sphere.Update(*debugCamera);
        // ----------------------
        // ------ 更新終了 -------
        // ----------------------
       


        // --------------------------------------------------------------------------------
        // Reflect
       
        
        // --------------------------------------------------------------------------------





        // ----------------------
        // --- 描画処理 (Draw) ---
        // ----------------------
        
        sphere.Draw();

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
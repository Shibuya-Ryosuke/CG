#include "./Original/RyoEngine.h"
#ifdef _DEBUG
#include "Original/ImGui/ImGuiAllInclude.h"
#endif

#include "RailSTG/Scene/SceneManager.h"

using namespace RyoEngine;

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    // エンジン初期化
    RyoEngine::Initialize();

    // ゲーム初期化
    SceneManager sceneManager;
    sceneManager.Initialize(Scene::Game);

    // --- メインループ ---
    while (GetWinApp()->ProcessMessage()) {
        // フレーム開始
        NewFrame();
       

        // ----------------------
        // -- 更新処理（Update） --
        // ----------------------
        
        // 更新
#ifdef _DEBUG
        ImGui::Begin("RailSTG_debug");
#endif
        sceneManager.Update();
#ifdef _DEBUG
        ImGui::End();
#endif
        // ----------------------
        // ------ 更新終了 -------
        // ----------------------
       





        // ----------------------
        // --- 描画処理 (Draw) ---
        // ----------------------
        
        // 描画
        sceneManager.Draw();

        // ----------------------
        // ------ 描画終了 -------
        // ----------------------

        // フレーム終了
        EndFrame();
    }
    
    // ゲーム終了
    sceneManager.Finalize();

    // エンジン終了
    RyoEngine::Finalize();

    return 0;
}
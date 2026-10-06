#include <RyoEngine.h>
#include <Test/Test.h>
#ifdef _DEBUG
#include <ImGui/ImGuiAllInclude.h>
#endif

#include "Application/GameName/GameName.h"


using namespace RyoEngine;

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    // エンジン初期化
    RyoEngine::Initialize(L"Application");

    std::unique_ptr<IGame> game = std::make_unique<Test::Test>();
    game->Initialize();

    // --- メインループ ---
    while (GetWinApp()->ProcessMessage()) {
        // フレーム開始
        NewFrame();

        // ----------------------
        // -- 更新処理（Update） --
        // ----------------------
        ImGui::ShowDemoWindow();
        // ゲーム
        game->Update();

        // ----------------------
        // ------ 更新終了 -------
        // ----------------------
       

        // ----------------------
        // --- 描画処理 (Draw) ---
        // ----------------------
       
        // ゲーム
        game->Draw();

        // ----------------------
        // ------ 描画終了 -------
        // ----------------------

        // フレーム終了
        EndFrame();
    }
    
    // ゲーム終了
    game->Finalize();

    // エンジン終了
    RyoEngine::Finalize();

    return 0;
}
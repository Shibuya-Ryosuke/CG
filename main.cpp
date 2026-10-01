#include <RyoEngine.h>
#include <Test/TestManager.h>
#ifdef _DEBUG
#include <ImGui/ImGuiAllInclude.h>
#endif

#include "Application/GameName/GameName.h"


using namespace RyoEngine;

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    // エンジン初期化
    RyoEngine::Initialize(L"Application");

    // テスト型
    TestManager test{};
    test.Initialize();

    // デバッグカメラ
    DebugCamera debugCamera{};
    debugCamera.Initialize();
    debugCamera.SetAvailable(true);

    std::unique_ptr<IGame> game = std::make_unique<Game1::Game1>();
    game->Initialize(debugCamera);

    // 全てのInitializeが終わったらJsonの読込
    ParamEditor::Load();
    LightManager::Load();

    // --- メインループ ---
    while (GetWinApp()->ProcessMessage()) {
        // フレーム開始
        NewFrame();

        // ----------------------
        // -- 更新処理（Update） --
        // ----------------------
        // テスト
        test.Update();


        // ゲーム
        //game->Update();


        // カメラ
        debugCamera.Update();

        // ----------------------
        // ------ 更新終了 -------
        // ----------------------
       





        // ----------------------
        // --- 描画処理 (Draw) ---
        // ----------------------
        // テスト
        test.Draw(debugCamera);
        

        // ゲーム
        //game->Draw();


        // 描画
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
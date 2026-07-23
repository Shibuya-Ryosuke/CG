#include "./Original/RyoEngine.h"
#include "player/player.h"
#include "enemy/enemy.h"
#ifdef _DEBUG
#include "Original/ImGui/ImGuiAllInclude.h"
#endif

using namespace RyoEngine;

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    // エンジン初期化
    RyoEngine::Initialize();

    Player player;
    player.Create();
    player.Initialize();

    Enemy enemy;
    enemy.Create();
    enemy.Initialize();

    DebugCamera* debugCamera = new DebugCamera();
    debugCamera->SetTranslate({0.0f,1.0f, -40.0f });

    AnimEdit::LoadSettings();

    // --- メインループ ---
    while (GetWinApp()->ProcessMessage()) {
        // フレーム開始
        NewFrame();

        // ----------------------
        // -- 更新処理（Update） --
        // ----------------------
        

        AnimEdit::Update();

        if (Input::TriggerKey(DIK_R)) {
            AnimEdit::LoadSettings();
            player.Initialize();
            enemy.Initialize();
        }

        debugCamera->SetAvailable(RyoEngine::GetOnTheGameView());
        debugCamera->Update();

        player.Update(*debugCamera);
        if (player.GetModel()->GetTranslate().x >= 0.0f) {
            player.SetIsHit(true);
            enemy.SetIsAlive(false);
        }
        enemy.Update(*debugCamera);

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
        
        player.Draw();
        enemy.Draw();

        // 3D終了----------------------------------------------------
        


        // [2D描画フェーズ]
        Begin2dDraw();
        if (player.GetIsPlay()) {
            PrintText("isPlay = true", { 0,0 });
        } else {
            PrintText("isPlay = false", { 0,0 });
        }
        if (player.GetisHit()) {
            PrintText("isHit = true", { 0,22 });
        } else {
            PrintText("isHit = false", { 0,22 });
        }
        if (enemy.GetIsAlive()) {
            PrintText("isAlive = true", { 0,44 });
        } else {
            PrintText("isAlive = false", { 0,44 });
        }
        // 2D終了----------------------------------------------------
        


        // ----------------------
        // ------ 描画終了 -------
        // ----------------------

        // フレーム終了
        EndFrame();
    }

    player.Finalize();
    enemy.Finalize();
    delete debugCamera;
    // エンジン終了
    RyoEngine::Finalize();

    return 0;
}
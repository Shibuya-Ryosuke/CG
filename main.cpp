#include "./Original/RyoEngine.h"
#include "player/player.h"
#ifdef _DEBUG
#include "Original/ImGui/ImGuiAllInclude.h"
#endif


using namespace RyoEngine;

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    // エンジン初期化
    RyoEngine::Initialize();

    Player player;
    player.Initialize();

    bool toggleB = false;
    AnimEdit::RegisterTriggerFlag("toggleB", &toggleB);

    Model* model = Model::Create("resources/TR.obj","bbb");
    model->SetTex("resources/wall.png");

    DebugCamera* debugCamera = new DebugCamera();
    debugCamera->SetTranslate({debugCamera->GetTranslate().x,debugCamera->GetTranslate().y -2.0f, -100.0f });

    AnimEdit::LoadSettings();

    // --- メインループ ---
    while (GetWinApp()->ProcessMessage()) {
        // フレーム開始
        NewFrame();

        // ----------------------
        // -- 更新処理（Update） --
        // ----------------------
        
        AnimEdit::Update();

        debugCamera->Update();
        player.Update(*debugCamera);
        model->Update(*debugCamera);
        if (Input::TriggerKey(DIK_B)) {
            toggleB = !toggleB;
        }

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
        model->Draw();
        

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

    delete model;

    delete debugCamera;
    // エンジン終了
    RyoEngine::Finalize();

    return 0;
}
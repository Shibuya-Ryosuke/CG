#include "./Original/RyoEngine.h"
#ifdef _DEBUG
#include "Original/ImGui/ImGuiAllInclude.h"
#endif


using namespace RyoEngine;

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    // エンジン初期化
    RyoEngine::Initialize();

    Model* model = Model::Create("resources/mirror.obj");
    model->SetTex("resources/flower.png");

    Model* model2 = Model::Create("resources/TR.obj");
    model2->SetTex("resources/wall.png");

    DebugCamera* debugCamera = new DebugCamera();


    // --- メインループ ---
    while (GetWinApp()->ProcessMessage()) {
        // フレーム開始
        NewFrame();

        // ----------------------
        // -- 更新処理（Update） --
        // ----------------------
        
        AnimEdit::Update();

        debugCamera->Update();
        model->Update(*debugCamera);
        model2->Update(*debugCamera);

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
        
        model->Draw();
        model2->Draw();

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
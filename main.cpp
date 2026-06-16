#include "./Original/RyoEngine.h"
#ifdef _DEBUG
#include "Original/Externals/imgui/imgui.h"
#endif

using namespace RyoEngine;

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    // エンジン初期化
    RyoEngine::Initialize();

    DebugCamera* d = new DebugCamera;
    d->Initialize();

    Mesh triangle;
    triangle.CreateTriangle({ 0.0f,0.0f,1.0f }, { 2.0f,2.0f });

    Mesh sphere;
    sphere.CreateSphere({ 1.0f,-1.0f,3.0f }, 12, { 1.0f,1.0f,1.0f,1.0f });
    sphere.SetTex("resources/flower.png");

    Model* model = Model::Create("resources/mirror.obj");
    model->Initialize();
    
    int o = 2;
    float n = 3.0f;
    Logger::LogWarning("int {}, alpha {}", o, n);
    
    // --- メインループ ---
    while (GetWinApp()->ProcessMessage()) {
        // フレーム開始
        NewFrame();

        // ----------------------
        // -- 更新処理（Update） --
        // ----------------------

        Vector3 a = sphere.GetDLDirection();

        ImGui::Begin("debug");
        ImGui::DragFloat3("light", &a.x, 0.001f, -1.0f, 1.0f);
        ImGui::End();
        sphere.SetDLDirection(a);


        d->Update();
        model->Update(*d);
        triangle.Update(*d);
        sphere.Update(*d);
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
        triangle.Draw();
        sphere.Draw();
       
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
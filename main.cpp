#include "./Original/RyoEngine.h"
#ifdef _DEBUG
#include "Original/ImGui/ImGuiAllInclude.h"
#endif

using namespace RyoEngine;

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    // エンジン初期化
    RyoEngine::Initialize();

    // ゲーム内カメラ
    Camera camera;
    camera.Initialize();

    // デバッグカメラ
    DebugCamera debugCamera;
    debugCamera.Initialize();
    debugCamera.SetTranslate({ 0.0f,0.3f,-9.2f });

    // アクティブカメラ
    Camera* activeCamera = nullptr;

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
        
        // アクティブの反転(お試し)
        if (Input::TriggerKey(DIK_K)) {
            if (camera.IsActive()) {
                camera.SetActive(false);
            } else {
                camera.SetActive(true);
            }
        }

        if (!camera.IsActive()) {
            debugCamera.SetAvailable(RyoEngine::GetOnTheGameView());
        }

        // アクティブカメラを決定
        activeCamera = camera.IsActive() ? &camera : &debugCamera;
        activeCamera->Update();
       
        SetCameraForPrimitive(*activeCamera);
        DrawSphere({ 0.0f,0.0f,0.0f }, 2.0f, 16, { 1.0f,1.0f,1.0f }, PrimitiveDrawMode::Fill);
        sphere.Update(*activeCamera);
        // ----------------------
        // ------ 更新終了 -------
        // ----------------------
       


        // --------------------------------------------------------------------------------
        // Reflect
       
        
        // --------------------------------------------------------------------------------





        // ----------------------
        // --- 描画処理 (Draw) ---
        // ----------------------
        
        if (camera.IsActive()) {
            PrintText("camera", { 10,10 });
        } else {
            PrintText("debug", { 10,10 });
        }
        //sphere.Draw();

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
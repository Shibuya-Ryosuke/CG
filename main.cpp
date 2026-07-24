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
    debugCamera->SetTranslate({0.0f,1.0f, -40.0f });

    // スプライト
    Sprite sprite;
    sprite.Initialize("resources/uvChecker.png");

    // 球
    Mesh sphere;
    sphere.CreateSphere({ 0.0f,0.0f }, 24);
    sphere.SetTex("resources/uvChecker.png");


    // --- メインループ ---
    while (GetWinApp()->ProcessMessage()) {
        // フレーム開始
        NewFrame();

        // ----------------------
        // -- 更新処理（Update） --
        // ----------------------
        ImGui::Begin("CG2");

        // 球操作
        Transform sphereTransform{
            .scale = sphere.GetScale(),
            .rotate = sphere.GetRotate(),
            .translate = sphere.GetTranslate()
        };
        DirectionalLight sphereDL = sphere.GetDirectionalLight();
        ImGui::DragFloat3("sphere S", &sphereTransform.scale.x, 0.01f, -5.0f, 5.0f);
        ImGui::DragFloat3("sphere R", &sphereTransform.rotate.x, 0.01f, -5.0f, 5.0f);
        ImGui::DragFloat3("sphere T", &sphereTransform.translate.x, 0.01f, -5.0f, 5.0f);
        ImGui::Spacing();
        ImGui::DragFloat4("light color", &sphereDL.color.x, 0.01f, -5.0f, 5.0f);
        ImGui::DragFloat3("light direction", &sphereDL.direction.x, 0.01f, -5.0f, 5.0f);
        ImGui::DragFloat("light intensity", &sphereDL.intensity, 0.01f, -5.0f, 5.0f);
        
        // スプライト操作
        Vector2 spriteS = sprite.GetScale();
        float spriteR = sprite.GetRotate();
        Vector2 spriteT = sprite.GetTranslate();
        Vector2 uvS = sprite.GetUVScale();
        float uvR = sprite.GetUVRotate();
        Vector2 uvT = sprite.GetUVTranslate();
        ImGui::DragFloat2("sprite S", &spriteS.x, 0.01f, -5.0f, 5.0f);
        ImGui::DragFloat("sprite R", &spriteR, 0.01f, -5.0f, 5.0f);
        ImGui::DragFloat2("sprite T", &spriteT.x, 0.01f, -5.0f, 5.0f);
        ImGui::DragFloat2("uv S", &uvS.x, 0.01f, -5.0f, 5.0f);
        ImGui::DragFloat("sprite R", &uvR, 0.01f, -5.0f, 5.0f);
        ImGui::DragFloat2("uv T", &uvT.x, 0.01f, -5.0f, 5.0f);

        ImGui::End();

        // カメラ更新
        debugCamera->SetAvailable(RyoEngine::GetOnTheGameView());
        debugCamera->Update();

        // 球更新
        sphere.Update(*debugCamera);

        // スプライト更新
        sprite.Update();
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
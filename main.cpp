#include "./Original/RyoEngine.h"
#ifdef _DEBUG
#include "Original/ImGui/ImGuiAllInclude.h"
#endif

#include "Application/Scene/SceneManager.h"
#include "Application/GameResources/GameSound.h"
#include "Application/GameResources/GameTex.h"

using namespace RyoEngine;

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    // エンジン初期化
    RyoEngine::Initialize();

    // リソース初期化
    GameImage::Initialize();
    GameSound::Initialize();

    // ゲーム初期化
    SceneManager sceneManager{};
    sceneManager.Initialize(Scene::Title);

    std::unique_ptr<Model> ground = Model::Create("resources/test/ground.obj");
    std::unique_ptr<Model> sky = Model::Create("resources/test/skydome.obj");
    std::unique_ptr<Model> flower = Model::Create("resources/test/TR.obj");
    std::unique_ptr<Model> bunny = Model::Create("resources/test/bunny.obj");
    flower->SetTranslateY(1.0f);
    bunny->SetTranslate({ 2.0f,1.5f,1.0f });
    DebugCamera camera{};
    camera.Initialize();
    camera.SetAvailable(true);

    float emissiveIntensity = 0.0f;
    Vector3 emissiveColor{};
    // --- メインループ ---
    while (GetWinApp()->ProcessMessage()) {
        // フレーム開始
        NewFrame();
       

        // ----------------------
        // -- 更新処理（Update） --
        // ----------------------
        
        // 更新
        sceneManager.Update();
        camera.Update();

        ImGui::Begin("bunny emissive");
        ImGui::DragFloat("intensity", &emissiveIntensity, 0.005f, 0.0f, 100.0f);
        ImGui::ColorEdit3("color", &emissiveColor.x);
        ImGui::End();
        bunny->SetEmissive(emissiveColor, emissiveIntensity);
        ground->Update(camera);
        sky->Update(camera);
        flower->Update(camera);
        bunny->Update(camera);
        // ----------------------
        // ------ 更新終了 -------
        // ----------------------
       





        // ----------------------
        // --- 描画処理 (Draw) ---
        // ----------------------
        
        // 描画
        sceneManager.Draw();
        ground->Draw();
        sky->Draw();
        flower->Draw();
        bunny->Draw();
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
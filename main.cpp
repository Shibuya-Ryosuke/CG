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
    RyoEngine::Initialize(L"Application");

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

    InstancedModel bullets{};
    InstancedModel::Handle id{};

    bullets.Initialize("resources/test/TR.obj");
    flower->SetTranslateY(1.0f);
    bunny->SetTranslate({ 0.0f,5.0f,0.0f });
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
        ImGui::Text("InstancedModel\nid : %d", id);
        ImGui::Text("count : %d", bullets.GetInstanceCount());
        ImGui::End();
        bunny->SetEmissive(emissiveColor, emissiveIntensity);

        if (Input::PushKey(DIK_A)) {
            Vector3 pos{ float(std::rand() % 100 - 50),float(std::rand()%20),float(std::rand() % 100 - 50)};
            id = bullets.AddInstance(pos);
        } else if (Input::PushKey(DIK_S)) {
            id = uint32_t(bullets.GetInstanceCount());
            id--;
            bullets.RemoveInstance(id);
            id--;
        }
        // まとめて行列計算
        bullets.UpdateBuffer();

        // ----------------------
        // ------ 更新終了 -------
        // ----------------------
       





        // ----------------------
        // --- 描画処理 (Draw) ---
        // ----------------------
        
        // 行列の確定
        ground->TransferMatrix(camera);
        sky->TransferMatrix(camera);
        flower->TransferMatrix(camera);
        bunny->TransferMatrix(camera);

        // 描画
        sceneManager.Draw();
        ground->Draw();
        sky->Draw();
        flower->Draw();
        bunny->Draw();
        bullets.Draw(camera);
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
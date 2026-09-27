#include "./Original/RyoEngine.h"
#include "Original/Test/TestManager.h"
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

    // テスト型
    TestManager test{};
    test.Initialize();

    DebugCamera camera{};
    camera.Initialize();
    camera.SetAvailable(true);


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
        
        // 更新
        sceneManager.Update();
        test.Update();
        camera.Update();

        // ----------------------
        // ------ 更新終了 -------
        // ----------------------
       





        // ----------------------
        // --- 描画処理 (Draw) ---
        // ----------------------
        
       
        // 描画
        sceneManager.Draw();
        test.Draw(camera);

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
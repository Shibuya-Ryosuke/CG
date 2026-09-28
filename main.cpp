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
       
        if (Input::TriggerKey(DIK_0)) {
            Change3DBlendMode(BlendMode::None);
        } else if (Input::TriggerKey(DIK_1)) {
            Change3DBlendMode(BlendMode::Normal);
        } else if (Input::TriggerKey(DIK_2)) {
            Change3DBlendMode(BlendMode::Add);
        } else if (Input::TriggerKey(DIK_3)) {
            Change3DBlendMode(BlendMode::Subtract);
        } else if (Input::TriggerKey(DIK_4)) {
            Change3DBlendMode(BlendMode::Multiply);
        } else if (Input::TriggerKey(DIK_5)) {
            Change3DBlendMode(BlendMode::Screen);
        }

        if (Input::TriggerKey(DIK_Z)) {
            Change2DBlendMode(BlendMode::None);
        } else if (Input::TriggerKey(DIK_X)) {
            Change2DBlendMode(BlendMode::Normal);
        } else if (Input::TriggerKey(DIK_C)) {
            Change2DBlendMode(BlendMode::Add);
        } else if (Input::TriggerKey(DIK_V)) {
            Change2DBlendMode(BlendMode::Subtract);
        } else if (Input::TriggerKey(DIK_B)) {
            Change2DBlendMode(BlendMode::Multiply);
        } else if (Input::TriggerKey(DIK_N)) {
            Change2DBlendMode(BlendMode::Screen);
        }
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
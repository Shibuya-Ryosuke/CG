#include "./Original/OriginalEngine.h"
#ifdef _DEBUG
#include "Original/Externals/imgui/imgui.h"
#endif

using namespace Engine;

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    // エンジン初期化
    Engine::Initialize();

    // テクスチャ
    uint32_t textureHandle = textureManager_->Load("resources/uvChecker.png");

    // 3d
    Object3d* model = Object3d::Create("resources/axis.obj");
    model->SetTexture(textureHandle);
    model->SetReflectionMode(ShadingMode::HALF_LAMBERT);

    // 画像
    Sprite* sprite = new Sprite();
    sprite->Initialize(textureHandle, {0.0f,0.0f});

    // 音
    uint32_t alarm = Audio::LoadAudio("resources/Alarm01.wav");
    Audio::PlayAudio(alarm, 1.0f);

    // カメラ
    Camera* camera = new Camera();

    // デバッグカメラ
    DebugCamera* debugCamera = new DebugCamera();



    // --- メインループ ---
    while (GetWinApp()->ProcessMessage()) {
        // --- 更新処理 (Update) ---
        // 入力受付
        Input::Update();

        ImGuiManager::Begin();

#ifdef _DEBUG
        ImGui::ShowDemoWindow();
        
#endif

        // Aキーでカメラ切り替え
        if (Input::TriggerKey(DIK_A)) {
            debugCamera->ToggleIsAvailable();
        }

        // カメラの種類によって更新変更
        if (debugCamera->GetIsAvailable()) {
            debugCamera->Update();
            model->Update(*debugCamera);
        } else {
            camera->Update();
            model->Update(*camera);
        }

        sprite->Update();



        // --- 描画処理 (Draw) ---
        GetDxCommon()->PreDraw();

        // [3D描画フェーズ]
        GetObject3dCommon()->BeginDraw();

        model->Draw();
       

        // [2D描画フェーズ]
        GetSpriteCommon()->BeginDraw();

        // デバッグカメラ時画像を描画しない
        if (!debugCamera->GetIsAvailable()) {
            sprite->Draw();
        }

        ImGuiManager::End(GetDxCommon()->GetCommandList());

        // 画面表示（PostDraw、コマンドリスト実行、スワップチェーン入れ替え）
        GetDxCommon()->PostDraw();
    }
    
    // 生ポインタ解放
    delete debugCamera;
    debugCamera = nullptr;

    delete camera;
    camera = nullptr;

    delete sprite;
    sprite = nullptr;

    delete model;
    model = nullptr;

    // エンジン終了
    Engine::Finalize();

    return 0;
}
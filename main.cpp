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
    uint32_t a = textureManager_->Load("resources/brick.png");
    // 3d
    Object3d* model = Object3d::Create("resources/axis.obj");
    model->SetTexture(textureHandle);
    model->SetTranslate({ 0.0f,-2.0f,-2.0f });

    Object3d* modelG = Object3d::Create("resources/mapping.obj");
    modelG->SetTexture(a);
    modelG->SetTranslate({ 0.0f,-3.5f,4.0f });

    ReflectObject* reflectModel = new ReflectObject();
    reflectModel->Initialize("resources/mirror.obj");
    // 音
    //uint32_t alarm = Audio::LoadAudio("resources/Alarm01.wav");
    //Audio::PlayAudio(alarm, 1.0f);

    // カメラ
    Camera* camera = new Camera();

    // デバッグカメラ
    DebugCamera* debugCamera = new DebugCamera();
    debugCamera->ToggleIsAvailable();
    

    // --- メインループ ---
    while (GetWinApp()->ProcessMessage()) {
        // --- 更新処理 (Update) ---
        // 入力受付
        Input::Update();

        ImGuiManager::Begin();

        Vector3 modelT = model->GetTranslate();
        Vector3 modelR = model->GetRotate();
        Vector3 modelS = model->GetScale();

#ifdef _DEBUG
        ImGui::Text("Model : Axis");
        ImGui::SliderFloat3("translate", &modelT.x, -10.0f, 10.0f);
        ImGui::SliderFloat3("rotate", &modelR.x, 0.0f, 10.0f);
        ImGui::SliderFloat3("scale", &modelS.x, -1.0f, 1.0f);
        ImGui::NewLine();

        ImGui::SliderFloat3("a", &debugCamera->GetTranslate().x, -20.0f, 20.0f);

        ImGui::SliderFloat3("rotate", &debugCamera->GetRotate().x, 0.0f, 0.0f);

       
#endif

        model->SetTranslate(modelT);
        model->SetRotate(modelR);
        model->SetScale(modelS);

        // Aキーでカメラ切り替え
        if (Input::TriggerKey(DIK_A)) {
            debugCamera->ToggleIsAvailable();
        }

        // カメラの種類によって更新変更
        if (debugCamera->GetIsAvailable()) {
            debugCamera->Update();
            reflectModel->Update(*debugCamera);
            model->Update(*debugCamera);
            modelG->Update(*debugCamera);
        } else {
            camera->Update();
            reflectModel->Update(*camera);
            model->Update(*camera);
        }


        // 描画先を鏡テクスチャに切り替え
        GetReflectCommon()->PreDraw();
        // 鏡の中用の描画設定
        GetObject3dCommon()->BeginDraw(Object3dCommon::DrawType::REFLECT);

        model->ReflectUpdate(*debugCamera,reflectModel->GetWorldMatrix());
        model->ReflectDraw();

        modelG->ReflectUpdate(*debugCamera, reflectModel->GetWorldMatrix());
        modelG->ReflectDraw();
        GetReflectCommon()->PostDraw();
        // 鏡終わり


        // --- 描画処理 (Draw) ---
        GetDxCommon()->PreDraw();

        // [3D描画フェーズ]
        GetObject3dCommon()->BeginDraw();
    
        modelG->Draw();
        model->Draw();

        reflectModel->Draw();


        // [2D描画フェーズ]
        GetSpriteCommon()->BeginDraw();

        ImGuiManager::End(GetDxCommon()->GetCommandList());
        // 画面表示（PostDraw、コマンドリスト実行、スワップチェーン入れ替え）
        GetDxCommon()->PostDraw();
    }
    
    // 生ポインタ解放
    delete debugCamera;
    debugCamera = nullptr;

    delete camera;
    camera = nullptr;

    delete model;
    model = nullptr;

    // エンジン終了
    Engine::Finalize();

    return 0;
}
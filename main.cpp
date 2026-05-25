#include "./Original/OriginalEngine.h"
#ifdef _DEBUG
#include "Original/Externals/imgui/imgui.h"
#endif

using namespace Engine;


int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    // エンジン初期化
    Engine::Initialize();

    // テクスチャ
    uint32_t textureHandle = GetTxManager()->Load("resources/uvChecker.png");
    uint32_t a = GetTxManager()->Load("resources/brick.png");
    uint32_t b = GetTxManager()->Load("resources/a.png");

    // 3d
    Object3d* model = Object3d::Create("resources/axis.obj");
    model->SetTexture(textureHandle);
    model->SetTranslate({ 0.0f,-2.0f,-2.0f });

    Object3d* modelGround = Object3d::Create("resources/mapping.obj");
    modelGround->SetTexture(a);
    modelGround->SetTranslate({ 0.0f,-3.5f,4.0f });

    ReflectObject* leftMirror = new ReflectObject();
    leftMirror->Initialize("resources/mirror.obj");
    leftMirror->SetTranslate({ -4.0f,-3.0f,8.0f });

    ReflectObject* rightMirror = new ReflectObject();
    rightMirror->Initialize("resources/mirror.obj");
    rightMirror->SetTranslate({ 4.0f,-3.0f,8.0f });
    rightMirror->SetRotate({ 0.0f,0.5f,0.0f });

    // カメラ
    Camera* camera = new Camera();

    // デバッグカメラ
    DebugCamera* debugCamera = new DebugCamera();
    debugCamera->ToggleIsAvailable();

    Sprite* sprite = new Sprite;
    sprite->Initialize(b, { 0.0f, 0.0f });

    // ImGuiで初期化させる
    //debugCamera->SetTranslate({ 0.0f,0.0f,0.0f });
    //debugCamera->SetRotate({ 0.0f,0.0f,0.0f });

    //model->SetTranslate({ 0.0f,-2.0f,-2.0f });
    //model->SetRotate({ 0.0f,0.0f,0.0f });

    // 音
    //uint32_t alarm = Audio::LoadAudio("resources/Alarm01.wav");
    //Audio::PlayAudio(alarm, 1.0f);


    // --- メインループ ---
    while (GetWinApp()->ProcessMessage()) {
        // --- 更新処理 (Update) ---
        // 入力受付
        Input::Update();

        ImGuiManager::NewFrame();

        Vector3 modelT = model->GetTranslate();
        Vector3 modelR = model->GetRotate();
        Vector3 modelS = model->GetScale();

        Vector3 lMirrorT = leftMirror->GetTranslate();
        Vector3 lMirrorR = leftMirror->GetRotate();
        Vector3 lMirrorS = leftMirror->GetScale();

        Vector3 rMirrorT = rightMirror->GetTranslate();
        Vector3 rMirrorR = rightMirror->GetRotate();
        Vector3 rMirrorS = rightMirror->GetScale();

#ifdef _DEBUG
        ImGui::Begin("Axis");
        ImGui::DragFloat3("Axis : translate", &modelT.x, 0.01f, -10.0f, 10.0f);
        ImGui::DragFloat3("Axis : rotate", &modelR.x, 0.01f, 0.0f, 10.0f);
        ImGui::DragFloat3("Axis : scale", &modelS.x, 0.01f, -1.0f, 1.0f);
        ImGui::End();

        ImGui::Begin("Mirror");
        ImGui::DragFloat3("Left Mirror : translate", &lMirrorT.x, 0.01f, -10.0f, 10.0f);
        ImGui::DragFloat3("Left Mirror : rotate", &lMirrorR.x, 0.01f, 0.0f, 10.0f);
        ImGui::DragFloat3("Left Mirror : scale", &lMirrorS.x, 0.01f, -1.0f, 1.0f);
        ImGui::NewLine();
        ImGui::DragFloat3("Right Mirror : translate", &rMirrorT.x, 0.01f, -10.0f, 10.0f);
        ImGui::DragFloat3("Right Mirror : rotate", &rMirrorR.x, 0.01f, 0.0f, 10.0f);
        ImGui::DragFloat3("Right Mirror : scale", &rMirrorS.x, 0.01f, -1.0f, 1.0f);
        ImGui::End();

        ImGui::Begin("Camera");
        ImGui::Text("DebugCamera");
        ImGui::DragFloat3("DebugCamera : translate", &debugCamera->GetTranslate().x, 0.01f, -20.0f, 20.0f);
        ImGui::DragFloat3("DebugCamera : rotate", &debugCamera->GetRotate().x, 0.01f, 0.0f, 0.0f);
        ImGui::End();

#endif

        model->SetTranslate(modelT);
        model->SetRotate(modelR);
        model->SetScale(modelS);

        leftMirror->SetTranslate(lMirrorT);
        leftMirror->SetRotate(lMirrorR);
        leftMirror->SetScale(lMirrorS);

        rightMirror->SetTranslate(rMirrorT);
        rightMirror->SetRotate(rMirrorR);
        rightMirror->SetScale(rMirrorS);

        // Aキーでカメラ切り替え
        if (Input::TriggerKey(DIK_A)) {
            debugCamera->ToggleIsAvailable();
        }

        // カメラの種類によって更新変更
        if (debugCamera->GetIsAvailable()) {
            debugCamera->Update();
            leftMirror->Update(*debugCamera);
            rightMirror->Update(*debugCamera);
            model->Update(*debugCamera);
            modelGround->Update(*debugCamera);
        } else {
            camera->Update();
            leftMirror->Update(*camera);
            model->Update(*camera);
        }
        sprite->Update();

        // 描画先を鏡テクスチャに切り替え
        GetReflectCommon()->PreDraw();
        // 鏡の中用の描画設定
        GetObject3dCommon()->BeginDraw(Object3dCommon::DrawType::REFLECT);

        model->ReflectUpdate(*debugCamera,leftMirror->GetWorldMatrix());
        model->ReflectDraw();

        modelGround->ReflectUpdate(*debugCamera, leftMirror->GetWorldMatrix());
        modelGround->ReflectDraw();
        GetReflectCommon()->PostDraw();
        // 鏡終わり


        // --- 描画処理 (Draw) ---
        GetDxCommon()->PreDraw();

        // [3D描画フェーズ]
        GetObject3dCommon()->BeginDraw();
    
        modelGround->Draw();
        model->Draw();

        leftMirror->Draw();
        rightMirror->Draw();


        // [2D描画フェーズ]
        GetSpriteCommon()->BeginDraw();
        if (Input::PushKey(DIK_Z)) {
            sprite->Draw();
        }

        ImGuiManager::EndFrame(GetDxCommon()->GetCommandList());
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
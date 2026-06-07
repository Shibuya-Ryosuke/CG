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
    uint32_t brick = GetTxManager()->Load("resources/brick.png");
    uint32_t wall = GetTxManager()->Load("resources/wall.png");

    // モデルリスト
    std::vector<Object3d*> models;

    // 3d
    Object3d* model = Object3d::Create("resources/TR.obj");
    model->SetTexture(textureHandle);
    model->SetTranslate({ 0.0f,-2.0f,-2.0f });
    models.push_back(model);

    Object3d* modelGround = Object3d::Create("resources/mapping.obj");
    modelGround->SetTexture(brick);
    modelGround->SetTranslate({ 0.0f,-3.5f,4.0f });
    models.push_back(modelGround);

    Object3d* modelWall = Object3d::Create("resources/wall.obj");
    modelWall->SetTexture(wall);
    modelWall->SetTranslate({ 0.0f,-0.0f,-25.0f });
    modelWall->SetScale({ 3.0f,1.0f,1.0f });
    models.push_back(modelWall);

    ReflectObject* leftMirror = new ReflectObject();
    leftMirror->Initialize("resources/mirror.obj");
    leftMirror->SetTranslate({ -0.15f,-3.0f,8.0f });

    ReflectObject* rightMirror = new ReflectObject();
    rightMirror->Initialize("resources/mirror.obj");
    rightMirror->SetTranslate({ 7.85f,-3.0f,8.0f });
    rightMirror->SetRotate({ 0.0f,0.5f,0.0f });

    // 登録
    for (auto* m : models) {
        leftMirror->RegisterObject(m);
        rightMirror->RegisterObject(m);
    }

    // カメラ
    Camera* camera = new Camera();

    // デバッグカメラ
    DebugCamera* debugCamera = new DebugCamera();
    debugCamera->SetRotate({ 0.135f,0.0f,0.0f });
    debugCamera->ToggleIsAvailable();

    // ImGuiで初期化させる
    //debugCamera->SetTranslate({ 0.0f,0.0f,0.0f });
    //debugCamera->SetRotate({ 0.0f,0.0f,0.0f });

    //model->SetTranslate({ 0.0f,-2.0f,-2.0f });
    //model->SetRotate({ 0.0f,0.0f,0.0f });

    // 音
    //uint32_t alarm = Audio::LoadAudio("resources/Alarm01.wav");
    //Audio::PlayAudio(alarm, 1.0f);

    
    Sprite sprite{};
    sprite.Initialize(rightMirror->GetSrvIndex(), {0.0f,0.0f});
    sprite.SetSize({ 1280,720 });

    bool check = false;

    // --- メインループ ---
    while (GetWinApp()->ProcessMessage()) {
        // --- 更新処理 (Update) ---
        // 入力受付
        Input::Update();

        ImGuiManager::NewFrame();

        Vector3 modelT = model->GetTranslate();
        Vector3 modelR = model->GetRotate();
        Vector3 modelS = model->GetScale();
        Vector4 modelColor = model->GetColor();

        Vector3 lMirrorT = leftMirror->GetTranslate();
        Vector3 lMirrorR = leftMirror->GetRotate();
        Vector3 lMirrorS = leftMirror->GetScale();

        Vector3 rMirrorT = rightMirror->GetTranslate();
        Vector3 rMirrorR = rightMirror->GetRotate();
        Vector3 rMirrorS = rightMirror->GetScale();

#ifdef _DEBUG
        ImGui::Begin("Model");
        ImGui::DragFloat3("Model : translate", &modelT.x, 0.01f, -10.0f, 10.0f);
        ImGui::DragFloat3("Model : rotate", &modelR.x, 0.01f, 0.0f, 10.0f);
        ImGui::DragFloat3("Model : scale", &modelS.x, 0.01f, -1.0f, 1.0f);
        ImGui::DragFloat4("Model : color", &modelColor.x, 0.01f, 0.0f, 1.0f);
        ImGui::End();

        ImGui::Begin("Mirror");
        ImGui::DragFloat3("Left Mirror : translate", &lMirrorT.x, 0.01f, -10.0f, 10.0f);
        ImGui::DragFloat3("Left Mirror : rotate", &lMirrorR.x, 0.01f, -10.0f, 10.0f);
        ImGui::DragFloat3("Left Mirror : scale", &lMirrorS.x, 0.01f, -1.0f, 1.0f);
        ImGui::NewLine();
        ImGui::DragFloat3("Right Mirror : translate", &rMirrorT.x, 0.01f, -10.0f, 10.0f);
        ImGui::DragFloat3("Right Mirror : rotate", &rMirrorR.x, 0.01f, -10.0f, 10.0f);
        ImGui::DragFloat3("Right Mirror : scale", &rMirrorS.x, 0.01f, -1.0f, 1.0f);
        ImGui::End();

        ImGui::Begin("Sprite Texture");
        if (ImGui::Checkbox("Left Mirror", &check)) {
            if (check) {
                sprite.SetTexture(leftMirror->GetSrvIndex());
            } else {
                sprite.SetTexture(rightMirror->GetSrvIndex());
            }
        }
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
        model->SetColor(modelColor);

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

        if (Input::TriggerKey(DIK_SPACE)) {
            debugCamera->SetTranslate({ 0.0f,0.0f,-20.0f });
            debugCamera->SetRotate({ 0.135f,0.0f,0.0f });

            model->SetTranslate({ 0.0f,-2.0f,-2.0f });
            model->SetRotate({ 0.0f,0.0f,0.0f });
            model->SetScale({ 1.0f,1.0f,1.0f });

            leftMirror->SetTranslate({ -0.15f,-3.0f,8.0f });

            rightMirror->SetTranslate({ 7.85f,-3.0f,8.0f });
            rightMirror->SetRotate({ 0.0f,0.5f,0.0f });
        }

        // カメラの種類によって更新変更
        if (debugCamera->GetIsAvailable()) {
            debugCamera->Update();
            model->Update(*debugCamera);
            modelGround->Update(*debugCamera);
            modelWall->Update(*debugCamera);
            leftMirror->Update(*debugCamera);
            rightMirror->Update(*debugCamera);
        } else {
            camera->Update();
            model->Update(*camera);
            modelGround->Update(*camera);
            modelWall->Update(*camera);
            leftMirror->Update(*camera);
            rightMirror->Update(*camera);
        }

        sprite.Update();

        // 反射テクスチャに書き込むための更新＆描画
        if (debugCamera->GetIsAvailable()) {
            leftMirror->ReflectProcess(*debugCamera);
            rightMirror->ReflectProcess(*debugCamera);
        } else {
            leftMirror->ReflectProcess(*camera);
            rightMirror->ReflectProcess(*camera);
        }


        // --- 描画処理 (Draw) ---
        GetDxCommon()->PreDraw();

        // [3D描画フェーズ]
        GetObject3dCommon()->BeginDraw();
    
        modelGround->Draw();
        model->Draw();
        modelWall->Draw();

        leftMirror->Draw();
        rightMirror->Draw();


        // [2D描画フェーズ]
        GetSpriteCommon()->BeginDraw();
        if (Input::PushKey(DIK_Z)) {
            sprite.Draw();
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
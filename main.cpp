#include "./Original/RyoEngine.h"
#ifdef _DEBUG
#include "Original/ImGui/ImGuiAllInclude.h"
#endif

using namespace RyoEngine;

void ModelOperate(Model* model,const char* id) {
    // モデル
    Transform transform{
        .scale = model->GetScale(),
        .rotate = model->GetRotate(),
        .translate = model->GetTranslate()
    };
    DirectionalLight dl = model->GetDirectionalLight();

    ImGui::PushID(id);
    if (ImGui::CollapsingHeader(id)) {
        // SRT
        if (ImGui::TreeNodeEx("transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragFloat3("scale", &transform.scale.x, 0.01f, -5.0f, 5.0f);
            ImGui::DragFloat3("rotate", &transform.rotate.x, 0.01f, -5.0f, 5.0f);
            ImGui::DragFloat3("translate", &transform.translate.x, 0.01f, -50.0f, 50.0f);
            ImGui::TreePop();
        }
        ImGui::Spacing();
        // ライト
        if (ImGui::TreeNodeEx("light", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragFloat4("color", &dl.color.x, 0.01f, -1.0f, 1.0f);
            ImGui::DragFloat3("direction", &dl.direction.x, 0.01f, -5.0f, 5.0f);
            ImGui::DragFloat("intensity", &dl.intensity, 0.01f, -5.0f, 5.0f);
            ImGui::TreePop();
        }
        ImGui::Spacing();
        // ランバート
        if (ImGui::TreeNodeEx("Lambert Mode", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::RadioButton("Lambert", model->GetLambert() == ShadingMode::LAMBERT)) {
                model->SetLambert(ShadingMode::LAMBERT);
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Half Lambert", model->GetLambert() == ShadingMode::HALF_LAMBERT)) {
                model->SetLambert(ShadingMode::HALF_LAMBERT);
            }
            ImGui::TreePop();
        }
    }
    ImGui::PopID();
    // セット
    model->SetTransform(transform);
    model->SetDirectionalLight(dl);

    // 余白
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    // エンジン初期化
    RyoEngine::Initialize();

    // カメラ
    DebugCamera* debugCamera = new DebugCamera();
    debugCamera->SetTranslate({ -11.370f,1.3f,-28.4f });
    debugCamera->SetRotate({ -0.02f,3.315f,0.0f });

    // 画像
    uint32_t uvTex = LoadTex("resources/uvChecker.png");

    // スプライト
    Sprite sprite;
    sprite.Initialize(uvTex);
    sprite.SetTranslate({ sprite.GetTexSize().x / 2.0f,sprite.GetTexSize().y / 2.0f });

    // 球
    Mesh sphere;
    sphere.CreateSphere({ 0.0f,0.0f }, 24);
    sphere.SetTex(uvTex);

    // モデル
    // 平面
    Model* plane = Model::Create("resources/plane.obj");
    plane->SetTex(uvTex);
    plane->SetTranslate({ 5.0f,0.0f,0.0f });
    // ティーポット
    Model* teapot = Model::Create("resources/teapot.obj");
    teapot->SetTex(uvTex);
    teapot->SetTranslate({ 10.0f,0.0f,0.0f });
    // ウサギ
    Model* bunny = Model::Create("resources/bunny.obj");
    bunny->SetTex(uvTex);
    bunny->SetTranslate({ 15.0f,0.0f,0.0f });
    // マルチメッシュ
    Model* multi = Model::Create("resources/multiMesh.obj");
    multi->SetTranslate({ 20.0f,0.0f,0.0f });

    // 音
    uint32_t se = Audio::GetInstance()->LoadAudio("resources/Alarm01.wav");

    // --- メインループ ---
    while (GetWinApp()->ProcessMessage()) {
        // フレーム開始
        NewFrame();

        // ----------------------
        // -- 更新処理（Update） --
        // ----------------------
        // ImGui
        ImGui::Begin("CG2");

        // カメラ操作
        Transform cameraTransform{
            .scale = 0.0f,
            .rotate = debugCamera->GetRotate(),
            .translate = debugCamera->GetTranslate()
        };
        ImGui::PushID("camera");
        if(ImGui::CollapsingHeader("camera")) {
            ImGui::DragFloat3("rotate", &cameraTransform.rotate.x, 0.01f, -5.0f, 5.0f);
            ImGui::DragFloat3("translate", &cameraTransform.translate.x, 0.01f, -5.0f, 5.0f);
        }
        ImGui::PopID();
        // セット
        debugCamera->SetRotate(cameraTransform.rotate);
        debugCamera->SetTranslate(cameraTransform.translate);


        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();


        // モデル(今は平面)
        ModelOperate(plane, "model");


        // 球操作
        Transform sphereTransform{
            .scale = sphere.GetScale(),
            .rotate = sphere.GetRotate(),
            .translate = sphere.GetTranslate()
        };
        DirectionalLight sphereDL = sphere.GetDirectionalLight();

        ImGui::PushID("sphere");
        if(ImGui::CollapsingHeader("sphere")) {
            // SRT
            if (ImGui::TreeNodeEx("transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::DragFloat3("scale", &sphereTransform.scale.x, 0.01f, -5.0f, 5.0f);
                ImGui::DragFloat3("rotate", &sphereTransform.rotate.x, 0.01f, -5.0f, 5.0f);
                ImGui::DragFloat3("translate", &sphereTransform.translate.x, 0.01f, -5.0f, 5.0f);
                ImGui::TreePop();
            }
            ImGui::Spacing();
            // ライト
            if (ImGui::TreeNodeEx("light", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::DragFloat4("color", &sphereDL.color.x, 0.01f, -1.0f, 1.0f);
                ImGui::DragFloat3("direction", &sphereDL.direction.x, 0.01f, -5.0f, 5.0f);
                ImGui::DragFloat("intensity", &sphereDL.intensity, 0.01f, -5.0f, 5.0f);
                ImGui::TreePop();
            }
            ImGui::Spacing();
            // ランバート
            if (ImGui::TreeNodeEx("Lambert Mode", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (ImGui::RadioButton("Lambert", sphere.GetLambert() == ShadingMode::LAMBERT)) {
                    sphere.SetLambert(ShadingMode::LAMBERT);
                }
                ImGui::SameLine();
                if (ImGui::RadioButton("Half Lambert", sphere.GetLambert() == ShadingMode::HALF_LAMBERT)) {
                    sphere.SetLambert(ShadingMode::HALF_LAMBERT);
                }
                ImGui::TreePop();
            }
        }
        ImGui::PopID();
        // セット
        sphere.SetTransform(sphereTransform);
        sphere.SetDirectionalLight(sphereDL);


        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();


        // スプライト操作
        Vector2 spriteS = sprite.GetScale();
        float spriteR = sprite.GetRotate();
        Vector2 spriteT = sprite.GetTranslate();
        Vector2 uvS = sprite.GetUVScale();
        float uvR = sprite.GetUVRotate();
        Vector2 uvT = sprite.GetUVTranslate();
        if (ImGui::CollapsingHeader("sprite")) {
            // SRT
            ImGui::PushID("sprite");
            if (ImGui::TreeNodeEx("transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::DragFloat2("scale", &spriteS.x, 0.01f, -5.0f, 5.0f);
                ImGui::DragFloat("rotate", &spriteR, 0.01f, -5.0f, 5.0f);
                ImGui::DragFloat2("translate", &spriteT.x, 1.0f, 0.0f, -1280.0f);
                ImGui::TreePop();
            }
            ImGui::Spacing();
            ImGui::PopID();

            //uv
            ImGui::PushID("uv");
            if (ImGui::TreeNodeEx("uv", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::DragFloat2("scale", &uvS.x, 0.01f, -5.0f, 5.0f);
                ImGui::DragFloat("rotate", &uvR, 0.01f, -5.0f, 5.0f);
                ImGui::DragFloat2("translate", &uvT.x, 0.01f, -5.0f, 5.0f);
                ImGui::TreePop();
            }
            ImGui::Spacing();
            ImGui::PopID();
        }
        // セット
        sprite.SetScale(spriteS);
        sprite.SetRotate(spriteR);
        sprite.SetTranslate(spriteT);
        sprite.SetUVScale(uvS);
        sprite.SetUVRotate(uvR);
        sprite.SetUVTranslate(uvT);


        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();


        // ティーポット
        ModelOperate(teapot, "teapot");
        // ウサギ
        ModelOperate(bunny, "bunny");
        // マルチメッシュ
        ModelOperate(multi, "multi");

        // オーディオ
        if (ImGui::Button("audio")) {
            Audio::GetInstance()->PlayAudio(se, 0.5f);
        }
        // リセット
        if (ImGui::Button("Initialize")) {
            // カメラ
            debugCamera->SetTranslate({ -11.370f,1.3f,-28.4f });
            debugCamera->SetRotate({ -0.02f,3.315f,0.0f });

            // スプライト
            sprite.SetScale({ 1.0f,1.0f });
            sprite.SetRotate(0.0f);
            sprite.SetTranslate({ sprite.GetTexSize().x / 2.0f,sprite.GetTexSize().y / 2.0f });
            // uv
            sprite.SetScale({ 1.0f,1.0f });
            sprite.SetUVRotate(0.0f);
            sprite.SetTranslate({ 0.0f,0.0f });

            // 球
            sphere.SetScale({ 1.0f,1.0f,1.0f });
            sphere.SetRotate({ 0.0f,0.0f,0.0f });
            sphere.SetTranslate({ 0.0f,0.0f,0.0f });

            // モデル
            // 平面
            plane->SetScale({ 1.0f,1.0f,1.0f });
            plane->SetRotate({ 0.0f,0.0f,0.0f });
            plane->SetTranslate({ 5.0f,0.0f,0.0f });
            // ティーポット
            teapot->SetScale({ 1.0f,1.0f,1.0f });
            teapot->SetRotate({ 0.0f,0.0f,0.0f });
            teapot->SetTranslate({ 10.0f,0.0f,0.0f });
            // ウサギ
            bunny->SetScale({ 1.0f,1.0f,1.0f });
            bunny->SetRotate({ 0.0f,0.0f,0.0f });
            bunny->SetTranslate({ 15.0f,0.0f,0.0f });
            // マルチメッシュ
            multi->SetScale({ 1.0f,1.0f,1.0f });
            multi->SetRotate({ 0.0f,0.0f,0.0f });
            multi->SetTranslate({ 20.0f,0.0f,0.0f });
        }
        ImGui::End();

        // ゲームパッド操作
        if (Input::GetJoystickTrigger(XINPUT_GAMEPAD_B)) {
            Audio::GetInstance()->PlayAudio(se, 0.5f);
        }

        // 各種更新
        debugCamera->SetAvailable(RyoEngine::GetOnTheGameView());
        debugCamera->Update();

        plane->Update(*debugCamera);
        sphere.Update(*debugCamera);
        teapot->Update(*debugCamera);
        bunny->Update(*debugCamera);
        multi->Update(*debugCamera);

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
        plane->Draw();
        sphere.Draw();
        teapot->Draw();
        bunny->Draw();
        multi->Draw();

        // 3D終了----------------------------------------------------
        


        // [2D描画フェーズ]
        Begin2dDraw();
        sprite.Draw();

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
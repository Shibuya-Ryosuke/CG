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

    // スプライト
    Sprite sprite;
    sprite.Initialize("resources/uvChecker.png");
    sprite.SetTranslate({ sprite.GetTexSize().x / 2.0f,sprite.GetTexSize().y / 2.0f });

    // 球
    Mesh sphere;
    sphere.CreateSphere({ 0.0f,0.0f }, 24);
    sphere.SetTex("resources/uvChecker.png");

    // モデル
    // 平面
    Model* plane = Model::Create("resources/plane.obj");
    plane->SetTex("resources/uvChecker.png");
    plane->SetRotate({ 0.0f,3.0f,0.0f });
    plane->SetTranslate({ 5.0f,0.0f,0.0f });
    // ティーポット
    Model* teapot = Model::Create("resources/teapot.obj");
    teapot->SetTex("resources/checkerBoard.png");
    teapot->SetTranslate({ 10.0f,0.0f,0.0f });
    // ウサギ
    Model* bunny = Model::Create("resources/bunny.obj");
    bunny->SetTex("resources/uvChecker.png");
    bunny->SetTranslate({ 15.0f,0.0f,0.0f });

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
        Transform modelTransform{
            .scale = plane->GetScale(),
            .rotate = plane->GetRotate(),
            .translate = plane->GetTranslate()
        };
        DirectionalLight modelDL = plane->GetDirectionalLight();

        ImGui::PushID("Model");
        if (ImGui::CollapsingHeader("plane")) {
            // SRT
            if (ImGui::TreeNodeEx("transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::DragFloat3("scale", &modelTransform.scale.x, 0.01f, -5.0f, 5.0f);
                ImGui::DragFloat3("rotate", &modelTransform.rotate.x, 0.01f, -5.0f, 5.0f);
                ImGui::DragFloat3("translate", &modelTransform.translate.x, 0.01f, -5.0f, 5.0f);
                ImGui::TreePop();
            }
            ImGui::Spacing();
            // ライト
            if (ImGui::TreeNodeEx("light", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::DragFloat4("color", &modelDL.color.x, 0.01f, -1.0f, 1.0f);
                ImGui::DragFloat3("direction", &modelDL.direction.x, 0.01f, -5.0f, 5.0f);
                ImGui::DragFloat("intensity", &modelDL.intensity, 0.01f, -5.0f, 5.0f);
                ImGui::TreePop();
            }
            ImGui::Spacing();
            // ランバート
            if (ImGui::TreeNodeEx("Lambert Mode", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (ImGui::RadioButton("Lambert", plane->GetLambert() == ShadingMode::LAMBERT)) {
                    plane->SetLambert(ShadingMode::LAMBERT);
                }
                ImGui::SameLine();
                if (ImGui::RadioButton("Half Lambert", plane->GetLambert() == ShadingMode::HALF_LAMBERT)) {
                    plane->SetLambert(ShadingMode::HALF_LAMBERT);
                }
                ImGui::TreePop();
            }
        }
        ImGui::PopID();
        // セット
        plane->SetTransform(modelTransform);
        plane->SetDirectionalLight(modelDL);


        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();


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


        // オーディオ
        if (ImGui::Button("audio")) {
            Audio::GetInstance()->PlayAudio(se, 0.5f);
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
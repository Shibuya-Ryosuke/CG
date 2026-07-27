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
    Vector4 color = model->GetColor();
    Vector2 uvT = model->GetUVTranslate();
    float uvR = model->GetUVRotate();
    Vector2 uvS = model->GetUVScale();

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
            ImGui::ColorEdit4("color", &dl.color.x);
            ImGui::DragFloat3("direction", &dl.direction.x, 0.01f, -5.0f, 5.0f);
            ImGui::DragFloat("intensity", &dl.intensity, 0.01f, -5.0f, 5.0f);
            ImGui::TreePop();
        }
        ImGui::Spacing();
       
        // マテリアル
        if (ImGui::TreeNodeEx("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
            // uv
            ImGui::DragFloat2("UV Translate", &uvT.x, 0.01f, -50.0f, 50.0f);
            ImGui::DragFloat("UV Rotate", &uvR, 0.01f, -50.0f, 50.0f);
            ImGui::DragFloat2("UV Scale", &uvS.x, 0.01f, -10.0f, 10.0f);
            ImGui::Spacing();
            // マテリアル自体の色
            ImGui::ColorEdit4("Material Color", &color.x);
            ImGui::Spacing();
            // ランバート
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
    model->SetUVSRT(uvS, uvR, uvT);
    model->SetColor(color);
}

void Space() {
    // 余白
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}
// 対象の enum class
enum class State {
    sphere_plane,
    sprite,
    teapot,
    bunny,
    multi_mesh,
    multi_material,
};

const char* GetStateName(State state) {
    switch (state) {
    case State::sphere_plane:   return "Sphere Plane";
    case State::sprite:         return "Sprite";
    case State::teapot:         return "Teapot";
    case State::bunny:          return "Bunny";
    case State::multi_mesh:     return "Multi Mesh";
    case State::multi_material: return "Multi Material";
    }
    return "Unknown";
}

void RenderStateCombo(State& currentState) {
    const char* items[] = {
        "Sphere Plane",
        "Sprite",
        "Teapot",
        "Bunny",
        "Multi Mesh",
        "Multi Material"
    };

    // 現在の enum を int のインデックスに変換
    int currentItemIndex = static_cast<int>(currentState);

    // コンボボックスの描画
    if (ImGui::BeginCombo("State", items[currentItemIndex])) {
        for (int i = 0; i < IM_ARRAYSIZE(items); i++) {
            bool isSelected = (currentItemIndex == i);

            if (ImGui::Selectable(items[i], isSelected)) {
                currentItemIndex = i;
                currentState = static_cast<State>(i); // 選択されたら enum を更新
            }

            // 初期フォーカスを現在の選択項目に合わせる
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
}

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    // エンジン初期化
    RyoEngine::Initialize();

    // 正面を向かせる定数
    const float kRotateY = 3.18f;

    // 状態
    State state = State::sphere_plane;

    // カメラ
    DebugCamera* debugCamera = new DebugCamera();
    debugCamera->SetTranslate({ 0.0f,0.3f,-9.2f });

    // 画像
    uint32_t uvTex = LoadTex("resources/uvChecker.png");

    // スプライト
    Sprite sprite;
    sprite.Initialize(uvTex);
    sprite.SetTranslate({ sprite.GetTexSize().x / 2.0f,sprite.GetTexSize().y / 2.0f });

    // 球
    Mesh sphere;
    sphere.CreateSphere({ 0.0f,0.0f }, 24);
    sphere.SetTranslate({ -1.7f,0.0f,0.0f });
    sphere.SetTex(uvTex);

    // モデル
    // 平面
    Model* plane = Model::Create("resources/plane.obj");
    plane->SetTranslate({ 1.7f,0.0f,0.0f });
    plane->SetRotate({ 0.0f,kRotateY,0.0f });
    // ティーポット
    Model* teapot = Model::Create("resources/teapot.obj");
    teapot->SetRotate({ 0.0f,kRotateY,0.0f });
    // ウサギ
    Model* bunny = Model::Create("resources/bunny.obj");
    bunny->SetRotate({ 0.0f,kRotateY,0.0f });
    // マルチメッシュ
    Model* multiMesh = Model::Create("resources/multiMesh.obj");
    multiMesh->SetRotate({ 0.0f,kRotateY,0.0f });
    // マルチマテリアル
    Model* multiMaterial = Model::Create("resources/multiMaterial.obj");
    multiMaterial->SetRotate({ 0.0f,kRotateY,0.0f });
    // 音
    uint32_t se = Audio::GetInstance()->LoadAudio("resources/Alarm01.wav");



    
    multiMaterial->SetTexByName("resources/flower.png", "Material");
    
    // --- メインループ ---
    while (GetWinApp()->ProcessMessage()) {
        // フレーム開始
        NewFrame();

        // ----------------------
        // -- 更新処理（Update） --
        // ----------------------
        
        // switchの外でやる必要あり
        // 球操作
        Transform sphereTransform{
            .scale = sphere.GetScale(),
            .rotate = sphere.GetRotate(),
            .translate = sphere.GetTranslate()
        };
        DirectionalLight sphereDL = sphere.GetDirectionalLight();
        // uv
        Vector2 sphereUVS = sphere.GetUVScale();
        float sphereUVR = sphere.GetUVRotate();
        Vector2 sphereUVT = sphere.GetUVTranslate();
        // color
        Vector4 sphereColor = sphere.GetColor();

        // スプライト操作
        Vector2 spriteS = sprite.GetScale();
        float spriteR = sprite.GetRotate();
        Vector2 spriteT = sprite.GetTranslate();
        // uv
        Vector2 uvS = sprite.GetUVScale();
        float uvR = sprite.GetUVRotate();
        Vector2 uvT = sprite.GetUVTranslate();
        // color
        Vector4 spriteColor = sprite.GetColor();

        // マルチマテリアル
        // モデル
        Transform multiMaterialTransform{
            .scale = multiMaterial->GetScale(),
            .rotate = multiMaterial->GetRotate(),
            .translate = multiMaterial->GetTranslate()
        };
        DirectionalLight multiMaterialDL = multiMaterial->GetDirectionalLight();
        // color
        Vector4 multiMaterialColor1 = multiMaterial->GetColorByName("Material");
        Vector4 multiMaterialColor2 = multiMaterial->GetColorByName("Material.001");
        
        // uv
        Vector2 uvT1 = multiMaterial->GetUVTranslate(multiMaterial->GetMeshIndexByName("Material"));
        float uvR1 = multiMaterial->GetUVRotate(multiMaterial->GetMeshIndexByName("Material"));
        Vector2 uvS1 = multiMaterial->GetUVScale(multiMaterial->GetMeshIndexByName("Material"));

        Vector2 uvT2 = multiMaterial->GetUVTranslate(multiMaterial->GetMeshIndexByName("Material.001"));
        float uvR2 = multiMaterial->GetUVRotate(multiMaterial->GetMeshIndexByName("Material.001"));
        Vector2 uvS2 = multiMaterial->GetUVScale(multiMaterial->GetMeshIndexByName("Material.001"));

        // ImGui
        ImGui::Begin("CG2");

        // リセット
        //if (ImGui::Button("SRT Initialize")) {
        //    // カメラ
        //    debugCamera->SetTranslate({ 0.0f,0.3f,-9.2f });

        //    // スプライト
        //    sprite.SetScale({ 1.0f,1.0f });
        //    sprite.SetRotate(0.0f);
        //    sprite.SetTranslate({ sprite.GetTexSize().x / 2.0f,sprite.GetTexSize().y / 2.0f });
        //    // uv
        //    sprite.SetScale({ 1.0f,1.0f });
        //    sprite.SetUVRotate(0.0f);
        //    sprite.SetTranslate({ 0.0f,0.0f });

        //    // 球
        //    sphere.SetScale({ 1.0f,1.0f,1.0f });
        //    sphere.SetRotate({ 0.0f,0.0f,0.0f });
        //    sphere.SetTranslate({ 0.0f,0.0f,0.0f });
        //    // 

        //    // モデル
        //    // 平面
        //    plane->SetScale({ 1.0f,1.0f,1.0f });
        //    plane->SetRotate({ 0.0f,kRotateY,0.0f });
        //    plane->SetTranslate({ 4.0f,0.0f,0.0f });
        //    // ティーポット
        //    teapot->SetScale({ 1.0f,1.0f,1.0f });
        //    teapot->SetRotate({ 0.0f,kRotateY,0.0f });
        //    teapot->SetTranslate({ 0.0f,0.0f,0.0f });
        //    // ウサギ
        //    bunny->SetScale({ 1.0f,1.0f,1.0f });
        //    bunny->SetRotate({ 0.0f,kRotateY,0.0f });
        //    bunny->SetTranslate({ 0.0f,0.0f,0.0f });
        //    // マルチメッシュ
        //    multiMesh->SetScale({ 1.0f,1.0f,1.0f });
        //    multiMesh->SetRotate({ 0.0f,kRotateY,0.0f });
        //    multiMesh->SetTranslate({ 0.0f,0.0f,0.0f });
        //    // マルチマテリアル
        //    multiMaterial->SetScale({ 1.0f,1.0f,1.0f });
        //    multiMaterial->SetRotate({ 0.0f,kRotateY,0.0f });
        //    multiMaterial->SetTranslate({ 0.0f,0.0f,0.0f });
        //}

        ImGui::SameLine();
        // オーディオ
        if (ImGui::Button("play audio")) {
            Audio::GetInstance()->PlayAudio(se, 0.5f);
        }

        // カメラ操作
        Transform cameraTransform{
            .scale = 0.0f,
            .rotate = debugCamera->GetRotate(),
            .translate = debugCamera->GetTranslate()
        };
        ImGui::PushID("camera");
        if (ImGui::CollapsingHeader("camera")) {
            ImGui::DragFloat3("rotate", &cameraTransform.rotate.x, 0.01f, -5.0f, 5.0f);
            ImGui::DragFloat3("translate", &cameraTransform.translate.x, 0.01f, -5.0f, 5.0f);
        }
        ImGui::PopID();
        // セット
        debugCamera->SetRotate(cameraTransform.rotate);
        debugCamera->SetTranslate(cameraTransform.translate);
        Space();


        // コンボで変更
        RenderStateCombo(state);
        ImGui::Spacing();


        switch (state) {
        case State::sphere_plane:
            // モデル(今は平面)
            ModelOperate(plane, "plane");

            // 球
            ImGui::PushID("sphere");
            if (ImGui::CollapsingHeader("sphere")) {
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
                    ImGui::ColorEdit4("color", &sphereDL.color.x);
                    ImGui::DragFloat3("direction", &sphereDL.direction.x, 0.01f, -5.0f, 5.0f);
                    ImGui::DragFloat("intensity", &sphereDL.intensity, 0.01f, -5.0f, 5.0f);
                    ImGui::TreePop();
                }
                ImGui::Spacing();

                // マテリアル
                if (ImGui::TreeNodeEx("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
                    // uv
                    ImGui::DragFloat2("UV Scale", &sphereUVS.x, 0.01f, -10.0f, 10.0f);
                    ImGui::DragFloat("UV Rotate", &sphereUVR, 0.01f, -50.0f, 50.0f);
                    ImGui::DragFloat2("UV Translate", &sphereUVT.x, 0.01f, -50.0f, 50.0f);
                    ImGui::Spacing();
                    // マテリアル自体の色
                    ImGui::ColorEdit4("Material Color", &sphereColor.x);
                    ImGui::Spacing();
                    // ランバート
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
            sphere.SetUVSRT(sphereUVS, sphereUVR, sphereUVT);
            sphere.SetColor(sphereColor);
            Space();
            break;



        case State::sprite:
            ImGui::PushID("sprite");
            if (ImGui::CollapsingHeader("sprite")) {
                // SRT
                if (ImGui::TreeNodeEx("transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::DragFloat2("scale", &spriteS.x, 0.01f, -5.0f, 5.0f);
                    ImGui::DragFloat("rotate", &spriteR, 0.01f, -5.0f, 5.0f);
                    ImGui::DragFloat2("translate", &spriteT.x, 1.0f, 0.0f, -1280.0f);
                    ImGui::TreePop();
                }
                ImGui::Spacing();

                // マテリアル
                if (ImGui::TreeNodeEx("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
                    // uv
                    if (ImGui::TreeNodeEx("uv", ImGuiTreeNodeFlags_DefaultOpen)) {
                        ImGui::DragFloat2("scale", &uvS.x, 0.01f, -5.0f, 5.0f);
                        ImGui::DragFloat("rotate", &uvR, 0.01f, -5.0f, 5.0f);
                        ImGui::DragFloat2("translate", &uvT.x, 0.01f, -5.0f, 5.0f);
                        ImGui::TreePop();
                    }
                    // マテリアル自体の色
                    ImGui::ColorEdit4("Material Color", &spriteColor.x);
                    ImGui::Spacing();
                    ImGui::TreePop();
                }
            }
            ImGui::PopID();
            // セット
            sprite.SetScale(spriteS);
            sprite.SetRotate(spriteR);
            sprite.SetTranslate(spriteT);
            sprite.SetUVSRT(uvS, uvR, uvT);
            sprite.SetColor(spriteColor);
            Space();
            break;



        case State::teapot:
            // ティーポット
            ModelOperate(teapot, "teapot");
            Space();
            break;

        case State::bunny:
            // ウサギ
            ModelOperate(bunny, "bunny");
            Space();
            break;

        case State::multi_mesh:
            // マルチメッシュ
            ModelOperate(multiMesh, "multiMesh");
            Space();
            break;

        case State::multi_material:
            // マルチマテリアル
            if (ImGui::CollapsingHeader("multiMaterial")) {
                ImGui::PushID("multiMaterial");
                // SRT
                if (ImGui::TreeNodeEx("transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::DragFloat3("scale", &multiMaterialTransform.scale.x, 0.01f, -5.0f, 5.0f);
                    ImGui::DragFloat3("rotate", &multiMaterialTransform.rotate.x, 0.01f, -5.0f, 5.0f);
                    ImGui::DragFloat3("translate", &multiMaterialTransform.translate.x, 0.01f, -50.0f, 50.0f);
                    ImGui::TreePop();
                }
                ImGui::Spacing();
                // ライト
                if (ImGui::TreeNodeEx("light", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::ColorEdit4("color", &multiMaterialDL.color.x);
                    ImGui::DragFloat3("direction", &multiMaterialDL.direction.x, 0.01f, -5.0f, 5.0f);
                    ImGui::DragFloat("intensity", &multiMaterialDL.intensity, 0.01f, -5.0f, 5.0f);
                    ImGui::TreePop();
                }
                ImGui::Spacing();
                ImGui::PopID();

                ImGui::PushID("Cube_multi");
                // マテリアル
                if (ImGui::TreeNodeEx("UV & Color", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::DragFloat2("UV Translate", &uvT1.x, 0.01f, -50.0f, 50.0f);
                    ImGui::DragFloat("UV Rotate", &uvR1, 0.01f, -50.0f, 50.0f);
                    ImGui::DragFloat2("UV Scale", &uvS1.x, 0.01f, -10.0f, 10.0f);
                    ImGui::Spacing();
                    // マテリアル自体の色
                    ImGui::ColorEdit4("Material Color", &multiMaterialColor1.x);
                    ImGui::Spacing();
                    // ランバート
                    if (ImGui::RadioButton("Lambert", multiMaterial->GetLambertByName("Material") == ShadingMode::LAMBERT)) {
                        multiMaterial->SetLambertByName(ShadingMode::LAMBERT,"Material");
                    }
                    ImGui::SameLine();
                    if (ImGui::RadioButton("Half Lambert", multiMaterial->GetLambertByName("Material") == ShadingMode::HALF_LAMBERT)) {
                        multiMaterial->SetLambertByName(ShadingMode::HALF_LAMBERT, "Material");
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();

                ImGui::PushID("Plane_multi");
                // マテリアル
                if (ImGui::TreeNodeEx("UV & Color", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::DragFloat2("UV Translate", &uvT2.x, 0.01f, -50.0f, 50.0f);
                    ImGui::DragFloat("UV Rotate", &uvR2, 0.01f, -50.0f, 50.0f);
                    ImGui::DragFloat2("UV Scale", &uvS2.x, 0.01f, -10.0f, 10.0f);
                    ImGui::Spacing();
                    // マテリアル自体の色
                    ImGui::ColorEdit4("Material Color", &multiMaterialColor2.x);
                    ImGui::Spacing();
                    // ランバート
                    if (ImGui::RadioButton("Lambert", multiMaterial->GetLambertByName("Material.001") == ShadingMode::LAMBERT)) {
                        multiMaterial->SetLambertByName(ShadingMode::LAMBERT, "Material.001");
                    }
                    ImGui::SameLine();
                    if (ImGui::RadioButton("Half Lambert", multiMaterial->GetLambertByName("Material.001") == ShadingMode::HALF_LAMBERT)) {
                        multiMaterial->SetLambertByName(ShadingMode::HALF_LAMBERT, "Material.001");
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
            // セット
            multiMaterial->SetTransform(multiMaterialTransform);
            multiMaterial->SetDirectionalLight(multiMaterialDL);
            // cube
            multiMaterial->SetUVScale(uvS1, multiMaterial->GetMeshIndexByName("Material"));
            multiMaterial->SetUVRotate(uvR1, multiMaterial->GetMeshIndexByName("Material"));
            multiMaterial->SetUVTranslate(uvT1, multiMaterial->GetMeshIndexByName("Material"));
            multiMaterial->SetColorByName(multiMaterialColor1, "Material");
            // plane
            multiMaterial->SetUVScale(uvS2, multiMaterial->GetMeshIndexByName("Material.001"));
            multiMaterial->SetUVRotate(uvR2, multiMaterial->GetMeshIndexByName("Material.001"));
            multiMaterial->SetUVTranslate(uvT2, multiMaterial->GetMeshIndexByName("Material.001"));
            multiMaterial->SetColorByName(multiMaterialColor2, "Material.001");
            Space();
            break;
        }
        ImGui::End();

        // ゲームパッド操作
        if (Input::GetJoystickTrigger(XINPUT_GAMEPAD_B)) {
            Audio::GetInstance()->PlayAudio(se, 0.5f);
        }


        // 各種更新
        debugCamera->SetAvailable(RyoEngine::GetOnTheGameView());
        debugCamera->Update();

        switch (state) {
        case State::sphere_plane:
            plane->Update(*debugCamera);
            sphere.Update(*debugCamera);
            break;

        case State::sprite:
            sprite.Update();
            break;

        case State::teapot:
            teapot->Update(*debugCamera);
            break;

        case State::bunny:
            bunny->Update(*debugCamera);
            break;

        case State::multi_mesh:
            multiMesh->Update(*debugCamera);
            break;

        case State::multi_material:
            multiMaterial->Update(*debugCamera);
            break;
        }
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
        switch (state) {
        case State::sphere_plane:
            plane->Draw();
            sphere.Draw();
            break;

        case State::sprite:
            break;

        case State::teapot:
            teapot->Draw();
            break;

        case State::bunny:
            bunny->Draw();
            break;

        case State::multi_mesh:
            multiMesh->Draw();
            break;

        case State::multi_material:
            multiMaterial->Draw();
            break;
        }

        // 3D終了----------------------------------------------------
        


        // [2D描画フェーズ]
        Begin2dDraw();
        if (state == State::sprite) {
            sprite.Draw();
        }

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
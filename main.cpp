#include "./Original/OriginalEngine.h"
#ifdef _DEBUG
#include "Original/Externals/imgui/imgui.h"
#endif

using namespace Engine;


int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    // エンジン初期化
    Engine::Initialize();

    const uint32_t kMax2dTriangles = 2;
    const uint32_t kMaxModels = 50;
    const uint32_t kMaxMirror = 4;
    const uint32_t kRespawnTime = 5;
    const float kSpace = 1.5f;

    bool isActiveMirror[4]{ false,false,false,false };
    bool isVideoProd = false;

    // カメラ
    DebugCamera* debugCamera = new DebugCamera();
    debugCamera->Initialize();

    // 再出現設定
    uint32_t respawnTime = kRespawnTime;
    bool isAlive[kMaxModels]{};
    Transform moveValue[kMaxModels]{};
    Vector4 spawnColor[kMaxModels]{};
    for (uint32_t i = 0; i < kMaxModels;i++) {
        isAlive[i] = false;
        moveValue[i] = { {0.5f,0.5f,0.5f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
        spawnColor[i] = { 1.0f,1.0f,1.0f,1.0f };
    }

    // テクスチャ
    uint32_t textures[2]{
        GetTexManager()->Load("resources/flower.png"),
        GetTexManager()->Load("resources/wall.png")
    };
    uint32_t backTex = GetTexManager()->Load("resources/back.png");

    // 2D
    Sprite triangles2d[kMax2dTriangles]{};
    uint32_t currentTextures[kMax2dTriangles]{};
    for (uint32_t i = 0; i < kMax2dTriangles;i++) {
        triangles2d[i].InitializeTriangle(textures[i], { -3.5f + i * 7.0f, 0.0f, 0.0f }, { 5.0f,5.0f });
        currentTextures[i] = textures[i];
    }

    // 3D
    // modelを格納する配列
    std::vector<Object3d*> models{};
    // model
    Object3d* model[kMaxModels]{};
    for (uint32_t i = 0; i < kMaxModels; i++) {
        model[i] = Object3d::Create("resources/Triangle.obj");
        model[i]->SetScale({ 0.5f,0.5f,0.5f });
        model[i]->SetTranslate({0.0f,500.0f,0.0});
        debugCamera->Initialize();
        Vector3 debugCameraT = debugCamera->GetTranslate();
        debugCameraT.z = -85.0f;
        debugCamera->SetTranslate(debugCameraT);
        model[i]->Update(*debugCamera);
        models.push_back(model[i]);
    }

    Object3d* wall = Object3d::Create("resources/CG_hyouka_Mirror.obj");
    wall->SetTranslate({ 0.0f,0.0f,12.0f });
    wall->SetScale({ 2.0f,3.0f,1.0f });
    wall->SetColor({ 0.7f,1.0f,1.0f,1.0f });
    wall->SetTexture(backTex);
    
    // ReflectModel
    ReflectObject* mirror[kMaxMirror]{};
    for (uint32_t i = 0; i < kMaxMirror;i++) {
        mirror[i] = ReflectObject::Create("resources/CG_hyouka_Mirror.obj");
        for (auto* m : models) {
            mirror[i]->RegisterObject(m);
        }
    }
    // 右左
    mirror[0]->SetTranslate({ 26.8f,0.0f,-27.0f });
    mirror[0]->SetRotate({ 0.0f,1.14f,0.0f });
    mirror[1]->SetTranslate({ -17.5f,0.0f,-27.0f });
    mirror[1]->SetRotate({ 0.0f,-1.24f,0.0f });
    // 下上
    mirror[2]->SetTranslate({ 2.5f,-7.5f,-27.0f });
    mirror[2]->SetRotate({ 1.57f,1.5f,0.0f });
    mirror[3]->SetTranslate({ -0.7f,7.5f,-27.0f });
    mirror[3]->SetRotate({ -1.57f,-1.5f,0.0f });

    // model用に変えたTrnaslateを元に戻す
    debugCamera->Initialize();
    // --- メインループ ---
    while (GetWinApp()->ProcessMessage()) {
        Input::Update();
        ImGuiManager::NewFrame();

        // ----------------------
        // -- 更新処理（Update） --
        // ----------------------

#ifdef _DEBUG
        if(isVideoProd){
            Transform mTransform[4]{};
            for (uint32_t i = 0; i < kMaxMirror;i++) {
                mTransform[i].scale = mirror[i]->GetScale();
                mTransform[i].rotate = mirror[i]->GetRotate();
                mTransform[i].translate = mirror[i]->GetTranslate();
            }

            ImGui::Begin("Toggle Active Mirror");
            if (ImGui::Button("right")) {
                isActiveMirror[0] = !isActiveMirror[0];
            }
            ImGui::SameLine();
            if (ImGui::Button("left")) {
                isActiveMirror[1] = !isActiveMirror[1];
            }
            ImGui::SameLine();
            if (ImGui::Button("bottom")) {
                isActiveMirror[2] = !isActiveMirror[2];
            }
            ImGui::SameLine();
            if (ImGui::Button("top")) {
                isActiveMirror[3] = !isActiveMirror[3];
            }
            ImGui::End();

            ImGui::Begin("Mirrors");
            ImGui::Text("right");
            ImGui::DragFloat3("right : translate", &mTransform[0].translate.x, 0.1f, -1000.0f, 1000.0f);
            ImGui::DragFloat3("right : rotate", &mTransform[0].rotate.x, 0.01f, -10.0f, 10.0f);
            ImGui::DragFloat3("right : scale", &mTransform[0].scale.x, 0.01f, -1.0f, 1.0f);
            ImGui::Dummy(ImVec2(0.0f, kSpace));

            ImGui::Text("left");
            ImGui::DragFloat3("left : translate", &mTransform[1].translate.x, 0.1f, -1000.0f, 1000.0f);
            ImGui::DragFloat3("left : rotate", &mTransform[1].rotate.x, 0.01f, -10.0f, 10.0f);
            ImGui::DragFloat3("left : scale", &mTransform[1].scale.x, 0.01f, -1.0f, 1.0f);
            ImGui::Dummy(ImVec2(0.0f, kSpace));

            ImGui::Text("bottom");
            ImGui::DragFloat3("bottom : translate", &mTransform[2].translate.x, 0.1f, -1000.0f, 1000.0f);
            ImGui::DragFloat3("bottom : rotate", &mTransform[2].rotate.x, 0.01f, -10.0f, 10.0f);
            ImGui::DragFloat3("bottom : scale", &mTransform[2].scale.x, 0.01f, -1.0f, 1.0f);
            ImGui::Dummy(ImVec2(0.0f, kSpace));

            ImGui::Text("top");
            ImGui::DragFloat3("top : translate", &mTransform[3].translate.x, 0.1f, -1000.0f, 1000.0f);
            ImGui::DragFloat3("top : rotate", &mTransform[3].rotate.x, 0.01f, -10.0f, 10.0f);
            ImGui::DragFloat3("top : scale", &mTransform[3].scale.x, 0.01f, -1.0f, 1.0f);

            ImGui::NewLine();
            ImGui::Separator();
            ImGui::NewLine();

            for (uint32_t i = 0; i < kMaxMirror; i++) {
                mirror[i]->SetScale(mTransform[i].scale);
                mirror[i]->SetRotate(mTransform[i].rotate);
                mirror[i]->SetTranslate(mTransform[i].translate);
            }

            ImGui::Text("Camera & Mirrors");
            if (ImGui::Button("Initialize")) {
                // カメラ
                debugCamera->Initialize();
                Vector3 translate = debugCamera->GetTranslate();
                translate.z = -85.0f;
                debugCamera->SetTranslate(translate);

                // 鏡
                mirror[0]->SetTranslate({ 26.8f,0.0f,-27.0f });
                mirror[0]->SetRotate({ 0.0f,1.14f,0.0f });
                mirror[1]->SetTranslate({ -17.5f,0.0f,-27.0f });
                mirror[1]->SetRotate({ 0.0f,-1.24f,0.0f });
                mirror[2]->SetTranslate({ 2.5f,-7.5f,-27.0f });
                mirror[2]->SetRotate({ 1.57f,1.5f,0.0f });
                mirror[3]->SetTranslate({ -0.7f,7.5f,-27.0f });
                mirror[3]->SetRotate({ -1.57f,-1.5f,0.0f });
            }
            ImGui::End();


        } else {
            Transform transforms[kMax2dTriangles]{};
            Vector4 color[kMax2dTriangles]{};
            for (uint32_t i = 0; i < kMax2dTriangles;i++) {
                transforms[i].translate = triangles2d[i].GetTranslate();
                transforms[i].rotate = triangles2d[i].GetRotate();
                transforms[i].scale = triangles2d[i].GetScale();
                color[i] = triangles2d[i].GetColor();
            }

            ImGui::Begin("Triangles");
            // 左の三角形
            ImGui::Text("Transform");
            ImGui::DragFloat3("left translate", &transforms[0].translate.x, 0.01f, -10.0f, 10.0f);
            ImGui::DragFloat3("left rotate", &transforms[0].rotate.x, 0.01f, -5.0f, 5.0f);
            ImGui::DragFloat3("left scale", &transforms[0].scale.x, 0.01f, -5.0f, 5.0f);
            ImGui::Dummy(ImVec2(0.0f, kSpace));

            ImGui::Text("Color");
            ImGui::ColorEdit4("left color", &color[0].x);
            ImGui::Dummy(ImVec2(0.0f, kSpace));

            ImGui::Text("Texture");
            if (ImGui::Button("left flower")) {
                currentTextures[0] = textures[0];
            }
            ImGui::SameLine();
            if (ImGui::Button("left wall")) {
                currentTextures[0] = textures[1];
            }
            ImGui::SameLine();
            if (ImGui::Button("left none")) {
                currentTextures[0] = GetSpriteCommon()->GetWhiteTex();
            }

            ImGui::NewLine();
            ImGui::Separator();
            ImGui::NewLine();

            // 右の三角形
            ImGui::Text("Transform");
            ImGui::DragFloat3("right translate", &transforms[1].translate.x, 0.01f, -10.0f, 10.0f);
            ImGui::DragFloat3("right rotate", &transforms[1].rotate.x, 0.01f, -5.0f, 5.0f);
            ImGui::DragFloat3("right scale", &transforms[1].scale.x, 0.01f, -5.0f, 5.0f);
            ImGui::Dummy(ImVec2(0.0f, kSpace));

            ImGui::Text("Color");
            ImGui::ColorEdit4("right color", &color[1].x);
            ImGui::Dummy(ImVec2(0.0f, kSpace));

            ImGui::Text("Texture");
            if (ImGui::Button("right flower ")) {
                currentTextures[1] = textures[0];
            }
            ImGui::SameLine();
            if (ImGui::Button("right wall ")) {
                currentTextures[1] = textures[1];
            }
            ImGui::SameLine();
            if (ImGui::Button("right none ")) {
                currentTextures[1] = GetSpriteCommon()->GetWhiteTex();
            }

            ImGui::NewLine();
            ImGui::Separator();
            ImGui::NewLine();

            // セット
            for (uint32_t i = 0; i < kMax2dTriangles;i++) {
                triangles2d[i].SetTranslate(transforms[i].translate);
                triangles2d[i].SetRotate(transforms[i].rotate);
                triangles2d[i].SetScale(transforms[i].scale);
                triangles2d[i].SetColor(color[i]);
                triangles2d[i].SetTexture(currentTextures[i]);
            }

            ImGui::Text("Camera & Triangles");
            if (ImGui::Button("Initialize")) {
                triangles2d[0].SetTranslate({-3.5f,0.0f,0.0f});
                triangles2d[1].SetTranslate({ 3.5f,0.0f,0.0f });
                for (uint32_t i = 0; i < kMax2dTriangles;i++) {
                    triangles2d[i].SetRotate({0.0f,0.0f,0.0f});
                    triangles2d[i].SetScale({1.0f,1.0f,1.0f});
                    triangles2d[i].SetColor({1.0f,1.0f,1.0f,1.0f});
                    triangles2d[i].SetTexture(textures[i]);
                    currentTextures[i] = textures[i];
                }
                debugCamera->Initialize();
            }
            ImGui::End();
        }

        // 操作方法
        ImGui::Begin("Infomation");
        ImGui::Text("* About camera operation *");
        ImGui::Dummy(ImVec2(0.0f, kSpace));
        ImGui::Text("right click hold : rotate");
        ImGui::Text("wheel scroll     : translate z");
        ImGui::Text("wheel crick hold : translate x,y");
        ImGui::Dummy(ImVec2(0.0f, kSpace));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0.0f, kSpace));
        ImGui::Text("* Switching video production *");
        ImGui::Dummy(ImVec2(0.0f, kSpace));
        ImGui::Text("please push [ space ]");
        ImGui::End();

#endif
        // 映像演出との切り替え
        if (Input::TriggerKey(DIK_SPACE)) {
            isVideoProd = !isVideoProd;
            debugCamera->Initialize();
            if (isVideoProd) {
                Vector3 debugCameraT = debugCamera->GetTranslate();
                debugCameraT.x = 3.0f;
                debugCameraT.z = -85.0f;
                debugCamera->SetTranslate(debugCameraT);
            }
        }

        // カメラのアップデート
        debugCamera->Update();

        if (isVideoProd) {
            // 3dのアップデート
            // タイマー
            if (respawnTime > 0) {
                respawnTime--;
            }

            wall->Update(*debugCamera);
            for (uint32_t i = 0; i < kMaxModels;i++) {
                if (isAlive[i]) {
                    Vector3 translate = model[i]->GetTranslate();
                    Vector3 rotate = model[i]->GetRotate();

                    translate += moveValue[i].translate;
                    rotate += moveValue[i].rotate;

                    model[i]->SetTranslate(translate);
                    model[i]->SetRotate(rotate);
                    model[i]->Update(*debugCamera);

                    float distance = debugCamera->GetTranslate().z - translate.z;
                    if (distance >= 0.0f) {
                        isAlive[i] = false;
                        model[i]->SetTranslate({ 0.0f,-500.0f,0.0f });
                    }
                } else {
                    if (respawnTime <= 0) {
                        Vector3 randT{
                            static_cast<float>(std::rand()) / RAND_MAX - 0.2f,
                            static_cast<float>(std::rand()) / RAND_MAX * 0.4f - 0.2f,
                            -(static_cast<float>(std::rand()) / RAND_MAX * 0.4f + 0.3f)
                        };
                        Vector3 randR{
                            static_cast<float>(std::rand()) / RAND_MAX -0.5f,
                            static_cast<float>(std::rand()) / RAND_MAX -0.5f,
                            static_cast<float>(std::rand()) / RAND_MAX -0.5f
                        };
                        float randS = (static_cast<float>(std::rand()) / RAND_MAX) + 0.2f;
                        Vector4 randColor{
                            static_cast<float>(std::rand()) / RAND_MAX,
                            static_cast<float>(std::rand()) / RAND_MAX,
                            static_cast<float>(std::rand()) / RAND_MAX,
                            static_cast<float>(std::rand()) / RAND_MAX * 0.6f + 0.4f
                        };

                        respawnTime = kRespawnTime;
                        moveValue[i].translate = randT;
                        moveValue[i].rotate = randR;
                        spawnColor[i] = randColor;
                        isAlive[i] = true;

                        model[i]->SetTranslate({ 3.0f,0.0f,0.0f });
                        model[i]->SetScale({ randS, randS, randS });
                        model[i]->SetColor(randColor);
                    }
                }
            }
            // 鏡のアップデート
            for (uint32_t i = 0; i < kMaxMirror;i++) {
                if (!isActiveMirror[i]) {
                    continue;
                }
                mirror[i]->Update(*debugCamera);
            }
        } else {
            // 2dアップデート
            for (uint32_t i = 0; i < kMax2dTriangles;i++) {
                triangles2d[i].Update(*debugCamera);
            }
        }

        // ----------------------
        // ------ 更新終了 -------
        // ----------------------
        

        // --------------------------------------------------------------------------------
        // Reflect
        if (isVideoProd) {
            for (uint32_t i = 0; i < kMaxMirror;i++) {
                if (!isActiveMirror[i]) {
                    continue;
                }
                mirror[i]->ReflectProcess(*debugCamera);
            }
        }
        
        // --------------------------------------------------------------------------------

        // ----------------------
        // --- 描画処理 (Draw) ---
        // ----------------------
        GetDxCommon()->PreDraw();
        // [3D描画フェーズ]
        GetObject3dCommon()->BeginDraw();
        if (isVideoProd) {
            wall->Draw();
            for (uint32_t i = 0; i < kMaxModels;i++) {
                model[i]->Draw();
            }
            for (uint32_t i = 0; i < kMaxMirror;i++) {
                if (!isActiveMirror[i]) {
                    continue;
                }
                mirror[i]->Draw();
            }
        }

        // [2D描画フェーズ]
        GetSpriteCommon()->BeginDraw();
        if (!isVideoProd) {
            for (uint32_t i = 0; i < kMax2dTriangles;i++) {
                triangles2d[i].Draw();
            };
        }

        // ----------------------
        // ------ 描画終了 -------
        // ----------------------
        ImGuiManager::EndFrame(GetDxCommon()->GetCommandList());
        // 画面表示（PostDraw、コマンドリスト実行、スワップチェーン入れ替え）
        GetDxCommon()->PostDraw();
    }
    
    // エンジン終了
    Engine::Finalize();

    return 0;
}
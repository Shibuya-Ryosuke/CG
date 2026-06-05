#include "./Original/OriginalEngine.h"
#ifdef _DEBUG
#include "Original/Externals/imgui/imgui.h"
#endif

using namespace Engine;


int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    // エンジン初期化
    Engine::Initialize();

    const uint32_t kMax2dTriangles = 2;
    const uint32_t kMaxModels = 5;
    const uint32_t kMaxMirror = 4;
    bool isActiveMirror[4]{ false,false,false,false };
    const float kSpace = 1.5f;
    const float modelSpeed = 0.2f;
    bool isVideoProd = false;

    // テクスチャ
    uint32_t textures[2]{
        GetTexManager()->Load("resources/flower.png"),
        GetTexManager()->Load("resources/wall.png")
    };

    // 2D
    Sprite triangles2d[kMax2dTriangles]{};
    Transform transforms[kMax2dTriangles]{};
    Vector4 color[kMax2dTriangles]{};
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
        model[i]->SetTranslate({ -2.0f + i * 2.0f,0.0f,20.0f });
        models.push_back(model[i]);
    }
    
    // ReflectModel
    ReflectObject* mirror[kMaxMirror]{};
    for (uint32_t i = 0; i < kMaxMirror;i++) {
        mirror[i] = ReflectObject::Create("resources/CG_hyouka_Mirror.obj");
        for (auto* m : models) {
            mirror[i]->RegisterObject(m);
        }
    }
    // 右左
    mirror[0]->SetTranslate({ 5.0f,0.0f,0.0f });
    mirror[1]->SetTranslate({ -5.0f,0.0f,0.0f });
    mirror[1]->SetScale({ -1.0f,1.0f,1.0f });
    // 上下
    mirror[2]->SetTranslate({ 0.0f,5.0f,0.0f });
    mirror[2]->SetRotate({ 0.0f,0.0f,std::numbers::pi_v<float> / 2.0f });
    mirror[3]->SetTranslate({ 0.0f,-5.0f,0.0f });
    mirror[3]->SetRotate({ 0.0f,0.0f,-(std::numbers::pi_v<float> / 2.0f) });

    // カメラ
    DebugCamera* debugCamera = new DebugCamera();
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

        } else {
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
                for (uint32_t i = 0; i < kMax2dTriangles;i++) {
                    triangles2d[i].InitializeTriangle(textures[i], { -3.5f + i * 7.0f, 0.0f, 0.0f }, { 5.0f,5.0f });
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
        }

        if (Input::TriggerKey(DIK_1)) {
            isActiveMirror[0] = !isActiveMirror[0];
        }
        if (Input::TriggerKey(DIK_2)) {
            isActiveMirror[1] = !isActiveMirror[1];
        }
        if (Input::TriggerKey(DIK_3)) {
            isActiveMirror[2] = !isActiveMirror[2];
        }
        if (Input::TriggerKey(DIK_4)) {
            isActiveMirror[3] = !isActiveMirror[3];
        }
        // カメラのアップデート
        debugCamera->Update();

        if (isVideoProd) {
            // 3dのアップデート
            for (uint32_t i = 0; i < kMaxModels;i++) {
                Vector3 translate = model[i]->GetTranslate();
                translate.z -= modelSpeed;
                model[i]->SetTranslate(translate);
                model[i]->Update(*debugCamera);
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
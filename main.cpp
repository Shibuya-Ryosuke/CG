#include "./Original/OriginalEngine.h"
#ifdef _DEBUG
#include "Original/Externals/imgui/imgui.h"
#endif

using namespace Engine;


int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    // エンジン初期化
    Engine::Initialize();

    const int32_t kMax = 2;
    const float kSpace = 1.5f;

    // テクスチャ
    uint32_t textures[2]{
        GetTexManager()->Load("resources/uvChecker.png"),
        GetTexManager()->Load("resources/brick.png")
    };

   

    Sprite triangles[kMax];
    Transform transforms[kMax]{};
    Vector4 color[kMax]{};
    uint32_t currentTextures[kMax]{};
    for (uint32_t i = 0; i < kMax;i++) {
        triangles[i].InitializeTriangle(textures[i], { -3.5f + i * 7.0f, 0.0f, 0.0f }, { 5.0f,5.0f });
        currentTextures[i] = textures[i];
    }


    DebugCamera* debugCamera = new DebugCamera();
    debugCamera->Initialize();

    // --- メインループ ---
    while (GetWinApp()->ProcessMessage()) {
        Input::Update();
        ImGuiManager::NewFrame();

      
        for (uint32_t i = 0; i < kMax;i++) {
            transforms[i].translate = triangles[i].GetTranslate();
            transforms[i].rotate = triangles[i].GetRotate();
            transforms[i].scale = triangles[i].GetScale();
            color[i] = triangles[i].GetColor();
        }

#ifdef _DEBUG
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
        if (ImGui::Button("left uvChecker")) {
            currentTextures[0] = textures[0];
        }
        ImGui::SameLine();
        if (ImGui::Button("left brick")) {
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
        if (ImGui::Button("right uvChecker ")) {
            currentTextures[1] = textures[0];
        }
        ImGui::SameLine();
        if (ImGui::Button("right brick ")) {
            currentTextures[1] = textures[1];
        }
        ImGui::SameLine();
        if (ImGui::Button("right none ")) {
            currentTextures[1] = GetSpriteCommon()->GetWhiteTex();
        }

        ImGui::NewLine();
        ImGui::Separator();
        ImGui::NewLine();

        for (uint32_t i = 0; i < kMax;i++) {
            triangles[i].SetTranslate(transforms[i].translate);
            triangles[i].SetRotate(transforms[i].rotate);
            triangles[i].SetScale(transforms[i].scale);
            triangles[i].SetColor(color[i]);
            triangles[i].SetTexture(currentTextures[i]);
        }

        ImGui::Text("Camera & Triangles");
        if (ImGui::Button("Initialize")) {
            for (uint32_t i = 0; i < kMax;i++) {
                triangles[i].InitializeTriangle(textures[i], { -3.5f + i * 7.0f, 0.0f, 0.0f }, { 5.0f,5.0f });
                currentTextures[i] = textures[i];
            }
            debugCamera->Initialize();
        }
        ImGui::End();

        ImGui::Begin("Camera");
        ImGui::DragFloat3("rotate", &debugCamera->GetRotate().x, 0.01f, -1000.0f, 1000.0f);
        ImGui::DragFloat3("translate", &debugCamera->GetTranslate().x, 0.01f, -1000.0f, 1000.0f);
        ImGui::End();

#endif
        

        debugCamera->Update();

        for (uint32_t i = 0; i < kMax;i++) {
            triangles[i].Update(*debugCamera);
        }

        // --- 描画処理 (Draw) ---
        GetDxCommon()->PreDraw();

        // [3D描画フェーズ]
        GetObject3dCommon()->BeginDraw();
    

        // [2D描画フェーズ]
        GetSpriteCommon()->BeginDraw();
        for (uint32_t i = 0; i < kMax;i++) {
            triangles[i].Draw();
        };

        ImGuiManager::EndFrame(GetDxCommon()->GetCommandList());
        // 画面表示（PostDraw、コマンドリスト実行、スワップチェーン入れ替え）
        GetDxCommon()->PostDraw();
    }
    
    // エンジン終了
    Engine::Finalize();

    return 0;
}
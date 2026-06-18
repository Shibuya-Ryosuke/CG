#include "./Original/RyoEngine.h"
#ifdef _DEBUG
#include "Original/Externals/imgui/imgui.h"
#endif

using namespace RyoEngine;

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    // エンジン初期化
    RyoEngine::Initialize();

    Sprite s;
    s.Initialize("resources/flower.png", { 300.0f,400.0f });

    // --- メインループ ---
    while (GetWinApp()->ProcessMessage()) {
        // フレーム開始
        NewFrame();

        // ----------------------
        // -- 更新処理（Update） --
        // ----------------------


        s.Update();
        
        // ----------------------
        // ------ 更新終了 -------
        // ----------------------
        
        Vector2 a = s.GetTranslate();
        float b = s.GetRotate();
        Vector2 c = s.GetScale();

        Transform x = s.GetUVTransform();
        ImGui::Begin("a");
        ImGui::DragFloat2("trans", &a.x, 1.0f, -1000.0f, 1000.0f);
        ImGui::DragFloat("rotate", &b, 1.0f, -1000.0f, 1000.0f);
        ImGui::DragFloat2("scale", &c.x, 1.0f, -1000.0f, 1000.0f);
        ImGui::DragFloat2("uvt", &x.translate.x, 0.01f, -100.0f, 100.0f);
        ImGui::DragFloat3("uvr", &x.rotate.x, 0.01f, -100.0f, 100.0f);
        ImGui::DragFloat2("uvs", &x.scale.x, 0.01f, -100.0f, 100.0f);
        ImGui::End();

        s.SetTranslate(a);
        s.SetRotate(b);
        s.SetScale(c);
        s.SetUVTransform(x);


        // --------------------------------------------------------------------------------
        // Reflect
       
        
        // --------------------------------------------------------------------------------





        // ----------------------
        // --- 描画処理 (Draw) ---
        // ----------------------
        // [3D描画フェーズ]
        Begin3dDraw();
       
        // 3D終了----------------------------------------------------
        


        // [2D描画フェーズ]
        Begin2dDraw();
        PrintText("hello", { 100.0f,300.0f });
        PrintText("wait", { 200.0f,100.0f });
        PrintText("you\ncome", { 100.0f,320.0f });
        s.Draw();

        // 2D終了----------------------------------------------------
        


        // ----------------------
        // ------ 描画終了 -------
        // ----------------------

        // フレーム終了
        EndFrame();
    }
    
    // エンジン終了
    RyoEngine::Finalize();

    return 0;
}
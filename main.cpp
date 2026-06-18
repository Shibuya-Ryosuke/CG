#include "./Original/RyoEngine.h"
#ifdef _DEBUG
#include "Original/Externals/imgui/imgui.h"
#endif

using namespace RyoEngine;

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    // エンジン初期化
    RyoEngine::Initialize();

    // フォント
       // 1. インスタンスの作成（スマートポインタが安全でおすすめです）
    std::unique_ptr<RyoEngine::FontLoader> gameFont = std::make_unique<RyoEngine::FontLoader>();

    // 2. 初期化関数を呼び出す（パスはご自身の環境（Resources内など）に合わせてください）
    if (!gameFont->Initialize("Original/Resources/font.fnt", "Original/Resources/font_0.png")) {
        assert(false);
    }
    // --- メインループ ---
    while (GetWinApp()->ProcessMessage()) {
        // フレーム開始
        NewFrame();

        // ----------------------
        // -- 更新処理（Update） --
        // ----------------------



       
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
       
        // 3D終了----------------------------------------------------
        


        // [2D描画フェーズ]
        Begin2dDraw();
        gameFont->ScreenPrint("h", { 100.0f,100.0f });

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
#include "Framework.h"
#include <RyoEngine.h>

namespace RyoEngine {

    Framework::Framework(std::wstring windowTitle)
        : windowTitle_(std::move(windowTitle)) {}

    void Framework::Run() {
        // エンジン初期化
        RyoEngine::Initialize(windowTitle_.c_str());

        Initialize();

        // --- メインループ ---
        while (GetWinApp()->ProcessMessage()) {
            // フレーム開始
            NewFrame();

            // ----------------------
            // -- 更新処理（Update） --
            // ----------------------

            // ゲーム
            Update();

            // カメラ確定
            CameraManager::UpdateAll();

            // ----------------------
            // ------ 更新終了 -------
            // ----------------------


            // ----------------------
            // --- 描画処理 (Draw) ---
            // ----------------------

            // ゲーム
            Draw();

            // ----------------------
            // ------ 描画終了 -------
            // ----------------------

            // フレーム終了
            EndFrame();
        }

        // ゲーム終了
        Finalize();

        // エンジン終了
        RyoEngine::Finalize();
    }

}
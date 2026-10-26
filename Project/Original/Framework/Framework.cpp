#include "Framework.h"
#include <RyoEngine.h>

namespace RyoEngine {

    Framework::Framework(std::wstring windowTitle)
        : windowTitle_(std::move(windowTitle)) {
    }

    void Framework::Run() {
        // エンジン初期化
        RyoEngine::Initialize(windowTitle_.c_str());

        Initialize();

        // --- メインループ ---
        while (WinApp::ProcessMessage()) {
            // フレーム開始
            NewFrame();

            // ----------------------
            // -- 更新処理（Update） --
            // ----------------------

            // ゲーム
            Update();

            // シーン(切り替え予約の処理 → 現在のシーンの更新)
            SceneManager::Update();

            // カメラ確定
            CameraManager::UpdateAll();

            // ----------------------
            // ------ 更新終了 -------
            // ----------------------


            // ----------------------
            // --- 描画処理 (Draw) ---
            // ----------------------

            // シーン
            SceneManager::Draw();

            // ゲーム(全シーンの上に重ねるもの)
            Draw();

            // ----------------------
            // ------ 描画終了 -------
            // ----------------------

            // フレーム終了
            EndFrame();
        }

        // シーン終了
        SceneManager::Finalize();

        // ゲーム終了
        Finalize();

        // エンジン終了
        RyoEngine::Finalize();
    }

}

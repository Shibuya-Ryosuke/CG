#include "OriginalEngine.h"

namespace Engine {

    void Framework::Initialize(const wchar_t* title, int32_t width, int32_t height) {
        // 1. WindowsAPIの初期化[cite: 1]
        winApp_ = WinApp::GetInstance();
        winApp_->Initialize(title, width, height);

        // 2. DirectX基盤の初期化[cite: 2]
        dxCommon_ = DirectXCommon::GetInstance();
        dxCommon_->Initialize(winApp_,width,height);

        // 3. テクスチャマネージャーの初期化[cite: 6]
        textureManager_ = TextureManager::GetInstance();
        textureManager_->Initialize();

        // 4. 各種描画共通部の初期化[cite: 9, 11]
        object3dCommon_ = Object3dCommon::GetInstance();
        object3dCommon_->Initialize();

        spriteCommon_ = SpriteCommon::GetInstance();
        spriteCommon_->Initialize();

        // 5. オーディオの初期化[cite: 15]
        audio_ = Audio::GetInstance();
        audio_->Initialize();
    }

    void Framework::Finalize() {
        // 初期化と逆の順序で解放
        audio_->Finalize();
        Audio::DestroyInstance();

        spriteCommon_->Finalize();
        object3dCommon_->Finalize();
        textureManager_->Finalize();

        // DirectXの基盤を止める（デバイスなどの破棄）
        dxCommon_->Finalize();

        // 最後に WindowsAPI の登録を解除する
        winApp_->Finalize();
    }

    bool Framework::ProcessMessage() {
        return winApp_->ProcessMessage();
    }

    void Framework::BeginFrame() {
        // レンダーターゲットのクリア、コマンドリストのリセットなど[cite: 2]
        dxCommon_->PreDraw();
    }

    void Framework::EndFrame() {
        // コマンドリストのクローズと実行、画面フリップ[cite: 2]
        dxCommon_->PostDraw();
    }

}
#include "OriginalEngine.h"

namespace Engine {
    // ここで実際に定義（初期化）する。これが「1つだけ」存在する実体になる
    WinApp* winApp_ = nullptr;
    DirectXCommon* dxCommon_ = nullptr;
    ShaderCompiler* shaderCompiler_ = nullptr;
    TextureManager* textureManager_ = nullptr;
    Object3dCommon* object3dCommon_ = nullptr;
    SpriteCommon* spriteCommon_ = nullptr;
    Audio* audio_ = nullptr;

    void Initialize() {
        winApp_ = WinApp::GetInstance();
        winApp_->Initialize(L"test");

        // DirectX基盤の初期化
        dxCommon_ = DirectXCommon::GetInstance();
        dxCommon_->Initialize(winApp_);

        shaderCompiler_ = Engine::ShaderCompiler::GetInstance();
        shaderCompiler_->Initialize();

        // 入力関係初期化
        Input::Initialize(winApp_->GetHInstance(), winApp_->GetHwnd());

        // テクスチャマネージャーの初期化
        textureManager_ = TextureManager::GetInstance();
        textureManager_->Initialize();

        // 各種描画共通部の初期化
        object3dCommon_ = Object3dCommon::GetInstance();
        object3dCommon_->Initialize();

        spriteCommon_ = SpriteCommon::GetInstance();
        spriteCommon_->Initialize();

        // オーディオの初期化
        audio_ = Audio::GetInstance();
        audio_->Initialize();
    }

    void Finalize() {
        // 4. 終了処理
        // 各リソースの解放、WinAppのUnregisterClassなどが走る
        // 初期化と逆の順序で解放
        audio_->Finalize();
        Audio::DestroyInstance();

        spriteCommon_->Finalize();
        object3dCommon_->Finalize();
        textureManager_->Finalize();
        shaderCompiler_->Finalize();

        // DirectXの基盤を止める（デバイスなどの破棄）
        dxCommon_->Finalize();

        // 最後に WindowsAPI の登録を解除する
        winApp_->Finalize();

    }
}
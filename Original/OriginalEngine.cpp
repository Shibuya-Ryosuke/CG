#include "OriginalEngine.h"
#include <cstdlib>
#include <ctime>
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")

namespace Engine {

    // 匿名名前空間：この cpp ファイルの中からしかアクセスできない領域
    namespace {
        WinApp* winApp_ = nullptr;
        DirectXCommon* dxCommon_ = nullptr;
        ShaderCompiler* shaderCompiler_ = nullptr;
        TextureManager* textureManager_ = nullptr;
        Object3dCommon* object3dCommon_ = nullptr;
        SpriteCommon* spriteCommon_ = nullptr;
        ReflectCommon* reflectCommon_ = nullptr;
        Audio* audio_ = nullptr;
    }

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

        reflectCommon_ = ReflectCommon::GetInstance();
        reflectCommon_->Initialize();

        // オーディオの初期化
        audio_ = Audio::GetInstance();
        audio_->Initialize();

        // ImGui初期化
        ImGuiManager::Initialize(
            winApp_->GetHwnd(),
            dxCommon_->GetDevice(),
            static_cast<int>(dxCommon_->GetBackBufferCount()),
            dxCommon_->GetBackBufferFormat()
        );

        std::srand(static_cast<unsigned int>(std::time(nullptr)));
    }

    void Finalize() {
        // 4. 終了処理
        // 各リソースの解放、WinAppのUnregisterClassなどが走る
        // 初期化と逆の順序で解放
        ImGuiManager::Finalize();

        audio_->Finalize();
        Audio::DestroyInstance();

        reflectCommon_->Finalize();
        spriteCommon_->Finalize();
        object3dCommon_->Finalize();
        textureManager_->Finalize();
        shaderCompiler_->Finalize();

        // DirectXの基盤を止める（デバイスなどの破棄）
        dxCommon_->Finalize();

        // 最後に WindowsAPI の登録を解除する
        winApp_->Finalize();

        Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
            debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
        }
    }

    // --- ゲッターの実装 ---
    // これらは Engine 名前空間の関数なので、上の匿名名前空間にある変数にアクセスできます。
    WinApp* GetWinApp() { return winApp_; }
    DirectXCommon* GetDxCommon() { return dxCommon_; }
    TextureManager* GetTexManager() { return textureManager_; }
    Object3dCommon* GetObject3dCommon() { return object3dCommon_; }
    SpriteCommon* GetSpriteCommon() { return spriteCommon_; }
    ReflectCommon* GetReflectCommon() { return reflectCommon_; }
}
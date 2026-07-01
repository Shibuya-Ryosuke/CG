#include "RyoEngine.h"
#include "Externals/imgui/imgui.h"
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")

namespace RyoEngine {
#ifdef _DEBUG
    namespace {
        Vector2 viewSize_{ 720.0f, 405.0f };
    }
    void SetImGuiViewSize(Vector2 viewSize) {
        viewSize_ = viewSize;
    }
#else
    void SetImGuiViewSize(Vector2 viewSize) {};
#endif
    // 匿名名前空間：この cpp ファイルの中からしかアクセスできない領域
    namespace {
        WinApp* winApp_ = nullptr;
        DirectXCommon* dxCommon_ = nullptr;
        ShaderCompiler* shaderCompiler_ = nullptr;
        TextureManager* textureManager_ = nullptr;
        ModelCommon* modelCommon_ = nullptr;
        SpriteCommon* spriteCommon_ = nullptr;
        ReflectCommon* reflectCommon_ = nullptr;
        Font* fontOutputer_ = nullptr;
        Audio* audio_ = nullptr;

        std::chrono::high_resolution_clock::time_point lastTime_;
        float deltaTime_ = 0.0f;
        float fps_ = 0.0f;
        float smoothedFps_ = 0.0f;

        std::chrono::steady_clock::time_point cpuStart_;
        float cpuFrameTime_ = 0.0f;
        float cpuFps_ = 0.0f;

    }

    void Initialize() {
        Logger::Initialize();

        winApp_ = WinApp::GetInstance();
        winApp_->Initialize(L"test");

        // DirectX基盤の初期化
        dxCommon_ = DirectXCommon::GetInstance();
        dxCommon_->Initialize(winApp_);

        shaderCompiler_ = RyoEngine::ShaderCompiler::GetInstance();
        shaderCompiler_->Initialize();

        // 入力関係初期化
        Input::Initialize(winApp_->GetHInstance(), winApp_->GetHwnd());

        // テクスチャマネージャーの初期化
        textureManager_ = TextureManager::GetInstance();
        textureManager_->Initialize();

        dxCommon_->CreateGameRenderTarget();

        // 各種描画共通部の初期化
        modelCommon_ = ModelCommon::GetInstance();
        modelCommon_->Initialize();

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
        // エディタ初期化
        AnimEdit::Initialize();

        // フォント
        fontOutputer_ = new Font();
        fontOutputer_->Initialize("Original/Resources/font.fnt", "Original/Resources/font_0.png");

        std::srand(static_cast<unsigned int>(std::time(nullptr)));

        lastTime_ = std::chrono::high_resolution_clock::now();

        Logger::Log("\n\n\n* Game Start * \n\n");
    }

    void Finalize() {
        Logger::Log("\n\n\n* Game Finish *\n\n");
        
        fontOutputer_->Finalize();
        delete fontOutputer_;
        fontOutputer_ = nullptr;

        ImGuiManager::Finalize();

        audio_->Finalize();
        Audio::DestroyInstance();

        reflectCommon_->Finalize();
        spriteCommon_->Finalize();
        modelCommon_->Finalize();
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

        Logger::Finalize();
    }

    void Begin3dDraw() {
        GetDxCommon()->PreDraw();
        GetModelCommon()->BeginDraw();
    }

    void Begin2dDraw() {
        GetSpriteCommon()->BeginDraw();
    }

    void NewFrame() {
        cpuStart_ = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = currentTime - lastTime_;
        deltaTime_ = elapsed.count();
        lastTime_ = currentTime;
        if (deltaTime_ > 0.0f) {
            fps_ = 1.0f / deltaTime_;
            smoothedFps_ = (smoothedFps_ * 0.9f) + (fps_ * 0.1f);
        }

        // 念のためゼロ除算（クラッシュ）防止
        if (deltaTime_ > 0.0f) {
            fps_ = 1.0f / deltaTime_;
            // 毎フレーム数値がガタガタ動くと見づらいので、10%ずつ近づけて滑らかにする（お好みで）
            smoothedFps_ = (smoothedFps_ * 0.9f) + (fps_ * 0.1f);
        }

        Input::Update();

#ifdef _DEBUG

        static float logTimer = 0.0f;
        logTimer += deltaTime_;       // 毎フレームの経過時間を足していく

        if (logTimer >= 5.0f) {       // 1.0秒（以上）経ったら
            Logger::Log("Engine is running... FPS: {:.1f}", smoothedFps_);

            logTimer -= 5.0f;
        }

        ImGuiManager::NewFrame();

        // ゲーム画面
        ImGui::Begin("Game View");
        D3D12_GPU_DESCRIPTOR_HANDLE gameTexHandle = dxCommon_->GetGameTextureGPUHandle();
        ImVec2 viewSize{ viewSize_.x,viewSize_.y };
        ImGui::Image(reinterpret_cast<ImTextureID>(gameTexHandle.ptr), viewSize);
        ImGui::End();

        // fps
        ImGui::Begin("Performance");
        ImGui::Text("FPS: %.1f", smoothedFps_);
        ImGui::Text("DeltaTime: %.4f s (%.2f ms)", deltaTime_, deltaTime_ * 1000.0f);
        ImGui::NewLine();

        ImGui::Text("cpuFps : %.1f", cpuFps_);
        ImGui::Text("cpuFrameTime : %.6f s (%.3f ms)", cpuFrameTime_, cpuFrameTime_ * 1000.0f);
        ImGui::End();

        // ログ
        ImGui::Begin("Log Console");
        // 上部にクリアボタンを配置
        if (ImGui::Button("Clear Log History")) {
            RyoEngine::Logger::Clear();
        }
        ImGui::Separator();

        // スクロール領域の作成
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        // ログ書き込み中の描画のバグを防ぐためロックを取得
        {
            std::lock_guard<std::mutex> lock(RyoEngine::Logger::GetMutex());
            const auto& logs = RyoEngine::Logger::GetLogHistory();

            // ログを一行ずつ描画
            for (const auto& log : logs) {
                // 文字列の先頭に応じて色を変えるカスタム（お好みで）
                if (log.find("[Error]") != std::string::npos) {
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", log.c_str());
                } else if (log.find("[Warning]") != std::string::npos) {
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", log.c_str());
                } else if (log.find("[Success]") != std::string::npos) {
                    ImGui::TextColored(ImVec4(0.1f, 0.7f, 1.0f, 1.0f), "%s", log.c_str());
                } else {
                    ImGui::TextUnformatted(log.c_str());
                }
            }
        }

        // 新しいログが追加されたら自動で最下部までスクロール
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();
        ImGui::End();
#endif
    }

    void EndFrame() {
        fontOutputer_->DrawAllText();

        auto cpuEnd = std::chrono::high_resolution_clock::now();

        // CPUの処理時間を計算 (秒単位)
        std::chrono::duration<float> cpuElapsed = cpuEnd - cpuStart_;
        cpuFrameTime_ = cpuElapsed.count();

        // FPS換算 (もしこの処理だけでループしたら何FPS出るか)
        cpuFps_ = (cpuFrameTime_ > 0.0f) ? (1.0f / cpuFrameTime_) : 0.0f;

        GetDxCommon()->PostDraw();  // ImGuiの終了処理はこの中にいる
    }

    uint32_t LoadTex(const std::string& filePath) {
        return GetTexManager()->Load(filePath);
    }

    // --- ゲッターの実装 ---
    // これらは Engine 名前空間の関数なので、上の匿名名前空間にある変数にアクセスできます。
    WinApp* GetWinApp() { return winApp_; }
    DirectXCommon* GetDxCommon() { return dxCommon_; }
    TextureManager* GetTexManager() { return textureManager_; }
    ModelCommon* GetModelCommon() { return modelCommon_; }
    SpriteCommon* GetSpriteCommon() { return spriteCommon_; }
    ReflectCommon* GetReflectCommon() { return reflectCommon_; }
    Font* GetFontOutputter() { return fontOutputer_; }


    float GetDeltaTime() { return deltaTime_; }
    float GetFPS() { return fps_; }
}
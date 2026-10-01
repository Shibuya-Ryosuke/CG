#include "RyoEngine.h"
#include "Externals/imgui/imgui.h"
#include <random>
#include <chrono>
#include <dxgidebug.h>
#include <algorithm>
#pragma comment(lib, "dxguid.lib")

namespace RyoEngine {
#ifdef _DEBUG
    namespace {
        Vector2 viewSize_{ 720.0f, 405.0f };
        bool onTheGameView = false;
    }
    void SetImGuiViewSize(Vector2 viewSize) {
        viewSize_ = viewSize;
    }
    bool GetOnTheGameView() {
        return onTheGameView;
    }
#else
    void SetImGuiViewSize(Vector2 viewSize) {};
    bool GetOnTheGameView() { return true; };
#endif
    // 匿名名前空間：この cpp ファイルの中からしかアクセスできない領域
    namespace {
        WinApp* winApp_ = nullptr;
        DirectXCommon* dxCommon_ = nullptr;
        ShaderCompiler* shaderCompiler_ = nullptr;
        TextureManager* textureManager_ = nullptr;
       
        ModelCommon* modelCommon_ = nullptr;
        SpriteCommon* spriteCommon_ = nullptr;
        //ReflectCommon* reflectCommon_ = nullptr;
        Font* fontOutputer_ = nullptr;
        Audio* audio_ = nullptr;

        DebugCamera debugCamera_{};
    }

    void Initialize(const wchar_t* title) {
        Logger::Initialize();

        winApp_ = WinApp::GetInstance();
        winApp_->Initialize(title);

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

        // HDRとか
        PostProcess::GetInstance()->Initialize();

        // ライトマネージャー
        LightManager::Initialize();

        // シャドウ
        ShadowMap::Initialize();

        // 各種描画共通部の初期化
        PrimitiveRenderer::Initialize();

        modelCommon_ = ModelCommon::GetInstance();
        modelCommon_->Initialize();

        InstancedModelCommon::GetInstance()->Initialize();

        spriteCommon_ = SpriteCommon::GetInstance();
        spriteCommon_->Initialize();

        //reflectCommon_ = ReflectCommon::GetInstance();
        //reflectCommon_->Initialize();

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

        // フォント
        fontOutputer_ = new Font();
        fontOutputer_->Initialize("Resources/EngineResources/Debugfont/debugfont.fnt", "Resources/EngineResources/Debugfont/debugfont.png");

        TimeManager::Initialize();

        // デバッグカメラ初期化
        debugCamera_.Initialize();
        // デバッグ時操作有効、それ以外では無効
#ifdef _DEBUG
        debugCamera_.SetAvailable(true);
#else
        debugCamera_.SetAvailable(false);
#endif

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

        //reflectCommon_->Finalize();
        spriteCommon_->Finalize();
        InstancedModelCommon::GetInstance()->Finalize();
        modelCommon_->Finalize();
        PrimitiveRenderer::Finalize();
        ShadowMap::Finalize();
        LightManager::Finalize();
        textureManager_->Finalize();
        shaderCompiler_->Finalize();

        PostProcess::GetInstance()->Finalize();

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
        PostProcess::GetInstance()->BeginScenePass();   // ← 追加：描画先をHDRバッファへ切り替える
        GetModelCommon()->BeginDraw();
    }

    void Begin2dDraw() {
        GetSpriteCommon()->BeginDraw();
    }

    void NewFrame() {
        PrimitiveRenderer::NewFrame();
        modelCommon_->CommandsClear();
        spriteCommon_->CommandsClear();
        InstancedModelCommon::GetInstance()->CommandsClear();

        Input::Update();

        TimeManager::NewFrame();

#ifdef _DEBUG

        static float logTimer = 0.0f;
        logTimer += TimeManager::GetDeltaTime();       // 毎フレームの経過時間を足していく

        if (logTimer >= 5.0f) {       // 1.0秒（以上）経ったら
            Logger::Log("Engine is running... FPS: {:.1f}", TimeManager::GetSmoothedFps());

            logTimer -= 5.0f;
        }

        ImGuiManager::NewFrame();
        debugCamera_.Update();

        // ゲーム画面
        ImGui::Begin("Game View");
        D3D12_GPU_DESCRIPTOR_HANDLE gameTexHandle = dxCommon_->GetGameTextureGPUHandle();
        ImVec2 viewSize{ viewSize_.x,viewSize_.y };
        ImGui::Image(reinterpret_cast<ImTextureID>(gameTexHandle.ptr), viewSize);
        if (!Input::IsMousePush(0) && !Input::IsMousePush(1) && !Input::IsMousePush(2)) {
            if (ImGui::IsItemHovered()) {
                onTheGameView = true;
            } else {
                onTheGameView = false;
            }
        }
        ImGui::End();

        // ライト
        LightManager::GetInstance()->DrawImGui();
        // ポストプロセス(HDR/ブルームのON-OFF)
        PostProcess::GetInstance()->DrawImGui();

        // fps
        ImGui::Begin("Performance");
        ImGui::Text("FPS: %.1f", TimeManager::GetFps());
        ImGui::Text("DeltaTime: %.4f s (%.2f ms)", TimeManager::GetDeltaTime(), TimeManager::GetDeltaTime() * 1000.0f);
        ImGui::NewLine();

        ImGui::Text("cpuFps : %.1f", TimeManager::GetCpuFps());
        ImGui::Text("cpuFrameTime : %.6f s (%.3f ms)", TimeManager::GetCpuFrameTime(), TimeManager::GetCpuFrameTime() * 1000.0f);
        ImGui::End();

        // ログ
        ImGui::Begin("Log Console");
        // 上部にクリアボタンを配置
        if (ImGui::Button("ログの履歴を削除")) {
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
        ParamEditor::DrawImGuiWindow();

        PrimitiveRenderer::Flush();

        // シャドウパス：ライト視点で深度だけ先に描画する
        ShadowMap::BeginShadowPass();
        modelCommon_->DrawShadow();
        ShadowMap::GetInstance()->EndShadowPass();

        // 3d描画
        Begin3dDraw();
        modelCommon_->Draw();
        InstancedModelCommon::GetInstance()->Draw();

        // HDR→LDR合成 (トーンマッピングのON/OFFはここで反映される)
        PostProcess::GetInstance()->EndScenePass();
        PostProcess::GetInstance()->Composite();

        // 2d描画
        Begin2dDraw();
        spriteCommon_->Draw();

        fontOutputer_->DrawAllText();

        TimeManager::EndFrame();

        GetDxCommon()->PostDraw();  // ImGuiの終了処理はこの中にいる
    }

    int32_t RandomInt32_t(int32_t min, int32_t max) {
        static std::random_device rd;
        static std::mt19937 gen(rd());

        // 順序が逆になっていたら自動で入れ替える安全策
        int32_t actualMin = std::min(min, max);
        int32_t actualMax = std::max(min, max);

        std::uniform_int_distribution<int32_t> dist(actualMin, actualMax);
        return dist(gen);
    }

    float RandomFloat(float min, float max) {
        static std::random_device rd;
        static std::mt19937 gen(rd());

        // 順序が逆になっていたら自動で入れ替える安全策
        float actualMin = std::min(min, max);
        float actualMax = std::max(min, max);

        std::uniform_real_distribution<float> dist(actualMin, actualMax);
        return dist(gen);
    }

    uint32_t LoadTex(const std::string& filePath) {
        std::string fullPath = "Resources/ApplicationResources/" + filePath;
        return GetTexManager()->Load(fullPath);
    }

    uint32_t LoadBGM(const std::string& filePath) {
        std::string fullPath = "Resources/ApplicationResources/" + filePath;
        return Audio::LoadBGM(fullPath);
    }

    uint32_t LoadSE(const std::string& filePath) {
        std::string fullPath = "Resources/ApplicationResources/" + filePath;
        return Audio::LoadSE(fullPath);
    }

    void Change2DBlendMode(BlendMode blendMode) {
        spriteCommon_->SetBlendMode(blendMode);
    }

    void Change3DBlendMode(BlendMode blendMode) {
        modelCommon_->SetBlendMode(blendMode);
        InstancedModelCommon::GetInstance()->SetBlendMode(blendMode);
    }

    Camera& GetActiveCamera(Camera& mainCamera) {
#ifdef _DEBUG
        // デバッグカメラが有効のとき
        if (debugCamera_.GetIsAvailable()) {
            return debugCamera_;
        }
#endif
        return mainCamera;
    }

    DebugCamera& GetDebugCamera() {
        return debugCamera_;
    }

    // --- ゲッターの実装 ---
    // これらは Engine 名前空間の関数なので、上の匿名名前空間にある変数にアクセスできます。
    WinApp* GetWinApp() { return winApp_; }
    DirectXCommon* GetDxCommon() { return dxCommon_; }
    TextureManager* GetTexManager() { return textureManager_; }
    ModelCommon* GetModelCommon() { return modelCommon_; }
    SpriteCommon* GetSpriteCommon() { return spriteCommon_; }
    //ReflectCommon* GetReflectCommon() { return reflectCommon_; }
    Font* GetFontOutputter() { return fontOutputer_; }


    float GetDeltaTime() { return TimeManager::GetDeltaTime(); }
    float GetFps() { return TimeManager::GetFps(); }
    float GetScaleTime() { return TimeManager::GetScaleTime(); }
    float GetTimeScale() { return TimeManager::GetScale(); }
    void SetTimeScale(float scale) { return TimeManager::SetScale(scale); }
}
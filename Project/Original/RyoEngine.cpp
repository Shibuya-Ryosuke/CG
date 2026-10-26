#include "RyoEngine.h"
#include <imgui.h>
#include "Graphics/3D/ModelManager.h"
#include "Editor/Gizmo.h"
#include "Editor/ModelPicker.h"
#include "Camera/CameraManager.h"
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
        //ReflectCommon* reflectCommon_ = nullptr;
        std::unique_ptr<Font> fontOutputer_;
    }

    void Initialize(const wchar_t* title) {
        Logger::Initialize();

        WinApp::Initialize(title);

        // DirectX基盤の初期化
        DirectXCommon::Initialize();

        // srvマネージャーの初期化
        SrvManager::Initialize();

        ShaderCompiler::Initialize();

        // 入力関係初期化
        Input::Initialize(WinApp::GetHInstance(), WinApp::GetHwnd());

        // テクスチャマネージャーの初期化
        TextureManager::Initialize();

        DirectXCommon::CreateGameRenderTarget();

        // HDRとか
        PostProcess::Initialize();

        // ライトマネージャー
        LightManager::Initialize();

        // シャドウ
        ShadowMap::Initialize();

        // 各種描画共通部の初期化
        PrimitiveRenderer::Initialize();

        ModelCommon::Initialize();

        ModelManager::Initialize();

        GPUParticleCommon::Initialize();
        GPUParticleManager::Initialize();

        SpriteCommon::Initialize();

        //reflectCommon_ = ReflectCommon::GetInstance();
        //reflectCommon_->Initialize();

        // オーディオの初期化
        Audio::Initialize();

        // ImGui初期化
        ImGuiManager::Initialize(
            WinApp::GetHwnd(),
            DirectXCommon::GetDevice(),
            static_cast<int>(DirectXCommon::GetBackBufferCount()),
            DirectXCommon::GetBackBufferFormat()
        );

        // フォント
        fontOutputer_ = std::make_unique<Font>();
        fontOutputer_->Initialize("Resources/EngineResources/Debugfont/debugfont.fnt", "Resources/EngineResources/Debugfont/debugfont.png");

        TimeManager::Initialize();

        // カメラマネージャー(デバッグカメラの初期化・操作可否の設定もこの中で行う)
        CameraManager::Initialize();

        Logger::Log("\n\n\n* Game Start * \n\n");
    }

    void Finalize() {
        Logger::Log("\n\n\n* Game Finish *\n\n");

        CameraManager::Finalize();

        fontOutputer_->Finalize();
        fontOutputer_.reset();

        ImGuiManager::Finalize();

        Audio::Finalize();

        //reflectCommon_->Finalize();
        SpriteCommon::Finalize();
        GPUParticleManager::Finalize();
        GPUParticleCommon::Finalize();
        ModelManager::Finalize();
        ModelCommon::Finalize();
        PrimitiveRenderer::Finalize();
        ShadowMap::Finalize();
        LightManager::Finalize();
        TextureManager::Finalize();
        SrvManager::Finalize();
        ShaderCompiler::Finalize();

        PostProcess::Finalize();

        // DirectXの基盤を止める（デバイスなどの破棄）
        DirectXCommon::Finalize();

        // 最後に WindowsAPI の登録を解除する
        WinApp::Finalize();

        Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
            debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
        }

        Logger::Finalize();
    }

    void Begin3dDraw() {
        DirectXCommon::PreDraw();
        PostProcess::BeginScenePass();   // ← 追加：描画先をHDRバッファへ切り替える
        ModelCommon::BeginDraw();
    }

    void Begin2dDraw() {
        SpriteCommon::BeginDraw();
    }

    void NewFrame() {
        PrimitiveRenderer::NewFrame();
        ModelCommon::CommandsClear();
        SpriteCommon::CommandsClear();
        ModelManager::NewFrame();
        GPUParticleCommon::CommandsClear();

        Input::Update();

        Audio::Update();

        TimeManager::NewFrame();

#ifdef _DEBUG

        static float logTimer = 0.0f;
        logTimer += TimeManager::GetDeltaTime();       // 毎フレームの経過時間を足していく

        if (logTimer >= 5.0f) {       // 1.0秒（以上）経ったら
            Logger::Log("Engine is running... FPS: {:.1f}", TimeManager::GetSmoothedFps());

            logTimer -= 5.0f;
        }

        ImGuiManager::NewFrame();
        Gizmo::BeginFrame();
        CameraManager::UpdateDebugCamera();

        // ゲーム画面
        ImGui::Begin("Game View");
        D3D12_GPU_DESCRIPTOR_HANDLE gameTexHandle = DirectXCommon::GetGameTextureGPUHandle();
        ImVec2 viewSize{ viewSize_.x,viewSize_.y };
        ImGui::Image(reinterpret_cast<ImTextureID>(gameTexHandle.ptr), viewSize);
        // ギズモの描画範囲 = ゲーム画面の画像範囲
        Gizmo::SetViewRect(ImGui::GetItemRectMin().x, ImGui::GetItemRectMin().y,
            ImGui::GetItemRectMax().x, ImGui::GetItemRectMax().y);
        if (!Input::IsMousePush(0) && !Input::IsMousePush(1) && !Input::IsMousePush(2)) {
            // ギズモ操作中はデバッグカメラ・クリック選択を止める
            if (ImGui::IsItemHovered() && !Gizmo::IsBusy()) {
                onTheGameView = true;
            } else {
                onTheGameView = false;
            }
        }
        ImGui::End();

        // ライト
        LightManager::DrawImGui();
        // シャドウ(範囲・カメラ追従)
        ShadowMap::DrawImGui();
        // ギズモ操作モード(W/E/R・ローカル/ワールド・スナップ)
        Gizmo::DrawToolbar();
        // ポストプロセス(HDR/ブルームのON-OFF)
        PostProcess::DrawImGui();
        // GPUパーティクル
        GPUParticleManager::DrawImGui();
        // カメラ(一覧・切り替え)
        CameraManager::DrawImGui();

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

        // GPUパーティクルのシミュレーション(Dispatch)：RTV/DSVに依存しないので一番早く実行する
        GPUParticleCommon::Dispatch();

        // シャドウパス：ライト視点で深度だけ先に描画する
        ShadowMap::BeginShadowPass();
        ModelCommon::DrawShadow();
        ModelManager::DrawShadow();
        ShadowMap::EndShadowPass();

        // 即時描画
        PrimitiveRenderer::Flush();

        // 3d描画
        Begin3dDraw();
        ModelCommon::Draw();
        ModelManager::Draw();
        GPUParticleCommon::Draw();

        // HDR→LDR合成 (トーンマッピングのON/OFFはここで反映される)
        PostProcess::EndScenePass();
        PostProcess::Composite();

        // 2d描画
        Begin2dDraw();
        SpriteCommon::Draw();

        fontOutputer_->DrawAllText();

        TimeManager::EndFrame();

        // クリック選択・ギズモ編集(ゲーム画面上のModelを自動で対象にする)
        ModelPicker::Update(GetOnTheGameView());

        DirectXCommon::PostDraw();  // ImGuiの終了処理はこの中にいる
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

    uint32_t LoadObj(const std::string& filePath) {
        std::string fullPath = "Resources/ApplicationResources/" + filePath;
        return ModelManager::LoadObj(fullPath);
    }

    uint32_t LoadTex(const std::string& filePath, bool isSRGB) {
        std::string fullPath = "Resources/ApplicationResources/" + filePath;
        return TextureManager::Load(fullPath, isSRGB);
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
        SpriteCommon::SetBlendMode(blendMode);
    }

    void Change3DBlendMode(BlendMode blendMode) {
        ModelCommon::SetBlendMode(blendMode);
        ModelManager::SetBlendMode(blendMode);
    }

    // --- ゲッターの実装 ---
    // これは Engine 名前空間の関数なので、上の匿名名前空間にある変数にアクセスできます。
    //ReflectCommon* GetReflectCommon() { return reflectCommon_; }
    Font* GetFontOutputter() { return fontOutputer_.get(); }


    float GetDeltaTime() { return TimeManager::GetDeltaTime(); }
    float GetFps() { return TimeManager::GetFps(); }
    float GetScaleTime() { return TimeManager::GetScaleTime(); }
    float GetTimeScale() { return TimeManager::GetScale(); }
    void SetTimeScale(float scale) { return TimeManager::SetScale(scale); }
}
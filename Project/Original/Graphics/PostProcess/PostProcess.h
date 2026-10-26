#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <vector>
#include <chrono>

namespace RyoEngine {

    /// <summary>
    /// 3D描画用のHDR中間バッファ(R16G16B16A16_FLOAT)と、それをLDR(gameRenderTargetResource_)へ
    /// 書き戻す合成(コンポジット)パス、およびブルームを管理するクラス。
    ///
    /// フレームの流れ:
    ///   DirectXCommon::PreDraw() (gameRenderTargetResource_をRTVにセット・クリア。ここは変更なし)
    ///     → PostProcess::BeginScenePass() (RTVだけHDRバッファへ差し替える。DSVは共有のmain深度バッファのまま)
    ///       → 3Dモデルの描画 (ModelCommon::Draw()等。HDRバッファに書き込まれる)
    ///     → PostProcess::EndScenePass() (HDRバッファをSRVとして読める状態に遷移)
    ///     → PostProcess::Composite() (ブルームON時はここで内部的にRenderBloom()も走る。
    ///                                  最終的に露出→ACES/クリップを経てgameRenderTargetResource_へ書き戻す)
    ///       → 2D/ImGui描画 (今まで通りgameRenderTargetResource_に重ね書き)
    ///
    /// ブルームの仕組み(Call of Duty方式のダウンサンプル/アップサンプル):
    ///   1. 閾値抽出：シーンカラーの明るい部分だけを抜き出し、bloomLevels_[0](半解像度)へ
    ///   2. ダウンサンプル：bloomLevels_[0]→[1]→[2]...と、半分ずつ縮小しながらぼかしていく
    ///   3. アップサンプル+加算合成：一番小さいレベルから、1段階大きいレベルへ加算合成しながら戻っていく
    ///   4. 最終的にbloomLevels_[0]に、複数スケールがブレンドされた滲みが出来上がる
    /// </summary>
    class PostProcess {
    public:
        static void Initialize();
        static void Finalize();

        static void BeginScenePass();
        static void EndScenePass();

        /// <summary>
        /// (ブルームON時は内部でRenderBloom()を実行した上で)HDRバッファの内容を、
        /// 露出→ACES(ON)またはクリップ(OFF)によりLDRへ圧縮し、gameRenderTargetResource_へ書き戻す。
        /// EndScenePass()の後、2D描画の前に呼ぶこと。
        /// </summary>
        static void Composite();

        /// <summary>
        /// 露出・ACES・ブルームのON/OFFや数値を切り替えるImGuiパネルを描画する。
        /// </summary>
        static void DrawImGui();

        // --- 外部から直接切り替えたい場合用 ---
        static void SetACESEnabled(bool enabled) { GetInstance()->acesEnabled_ = enabled; }
        static bool IsACESEnabled() { return GetInstance()->acesEnabled_; }
        static void SetExposure(float exposure) { GetInstance()->exposure_ = exposure; }
        static float GetExposure() { return GetInstance()->exposure_; }
        static void SetBloomEnabled(bool enabled) { GetInstance()->bloomEnabled_ = enabled; }
        static bool IsBloomEnabled() { return GetInstance()->bloomEnabled_; }
        static void SetBloomThreshold(float threshold) { GetInstance()->bloomThreshold_ = threshold; }
        static float GetBloomThreshold() { return GetInstance()->bloomThreshold_; }
        static void SetBloomIntensity(float intensity) { GetInstance()->bloomIntensity_ = intensity; }
        static float GetBloomIntensity() { return GetInstance()->bloomIntensity_; }

        // --- 追加エフェクト ---
        static void SetDistortionEnabled(bool enabled) { GetInstance()->distortionEnabled_ = enabled; }
        static bool IsDistortionEnabled() { return GetInstance()->distortionEnabled_; }
        static void SetDistortionStrength(float strength) { GetInstance()->distortionStrength_ = strength; }
        static float GetDistortionStrength() { return GetInstance()->distortionStrength_; }

        static void SetGlitchEnabled(bool enabled) { GetInstance()->glitchEnabled_ = enabled; }
        static bool IsGlitchEnabled() { return GetInstance()->glitchEnabled_; }
        static void SetGlitchIntensity(float intensity) { GetInstance()->glitchIntensity_ = intensity; }
        static float GetGlitchIntensity() { return GetInstance()->glitchIntensity_; }

        static void SetChromaticAberrationEnabled(bool enabled) { GetInstance()->chromaticAberrationEnabled_ = enabled; }
        static bool IsChromaticAberrationEnabled() { return GetInstance()->chromaticAberrationEnabled_; }
        static void SetChromaticAberrationStrength(float strength) { GetInstance()->chromaticAberrationStrength_ = strength; }
        static float GetChromaticAberrationStrength() { return GetInstance()->chromaticAberrationStrength_; }

        // NOTE: 簡易版(3x3ボックスブラーの1パス)。強くかけるとバンディングが出やすいので、
        //       本格的なブラーが必要になったらブルームと同じダウンサンプル方式に切り替えること。
        static void SetBlurEnabled(bool enabled) { GetInstance()->blurEnabled_ = enabled; }
        static bool IsBlurEnabled() { return GetInstance()->blurEnabled_; }
        static void SetBlurStrength(float strength) { GetInstance()->blurStrength_ = strength; }
        static float GetBlurStrength() { return GetInstance()->blurStrength_; }

        static void SetGrayscaleEnabled(bool enabled) { GetInstance()->grayscaleEnabled_ = enabled; }
        static bool IsGrayscaleEnabled() { return GetInstance()->grayscaleEnabled_; }
        static void SetGrayscaleIntensity(float intensity) { GetInstance()->grayscaleIntensity_ = intensity; }
        static float GetGrayscaleIntensity() { return GetInstance()->grayscaleIntensity_; }

        static void SetNoiseEnabled(bool enabled) { GetInstance()->noiseEnabled_ = enabled; }
        static bool IsNoiseEnabled() { return GetInstance()->noiseEnabled_; }
        static void SetNoiseIntensity(float intensity) { GetInstance()->noiseIntensity_ = intensity; }
        static float GetNoiseIntensity() { return GetInstance()->noiseIntensity_; }

    private:
        static PostProcess* GetInstance();

        PostProcess() = default;
        ~PostProcess() = default;
        PostProcess(const PostProcess&) = delete;
        PostProcess& operator=(const PostProcess&) = delete;

        void CreateSceneColorResource();
        void CreateRootSignatureAndPSO();
        void TransitionSceneColor(D3D12_RESOURCE_STATES newState);

        void CreateBloomLevels();
        void CreateBloomPipelineStates();
        void RenderBloom();
        void TransitionBloomLevel(size_t index, D3D12_RESOURCE_STATES newState);
        void DrawFullscreenTriangle(uint32_t width, uint32_t height);

        // HDRシーンカラーバッファ本体 (3D描画がここに書き込まれる)
        Microsoft::WRL::ComPtr<ID3D12Resource> sceneColorResource_;
        D3D12_RESOURCE_STATES sceneColorState_ = D3D12_RESOURCE_STATE_RENDER_TARGET;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> sceneColorRtvHeap_;
        uint32_t sceneColorTextureHandle_ = 0; // TextureManager登録後のSRVハンドル(合成パスで読む用)

        // 合成パス専用のRootSignature (ブルームの各パスもこれを使い回す。t0/t1のSRVテーブル2つ+CBV1つの単純な構成)
        Microsoft::WRL::ComPtr<ID3D12RootSignature> compositeRootSignature_;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> compositePipelineState_;

        // ブルーム用の縮小バッファ群(半解像度、1/4解像度...と段階的に小さくなる)
        static constexpr uint32_t kBloomLevelCount = 5;
        struct BloomLevel {
            Microsoft::WRL::ComPtr<ID3D12Resource> resource;
            D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_RENDER_TARGET;
            Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap;
            uint32_t textureHandle = 0;
            uint32_t width = 0;
            uint32_t height = 0;
        };
        std::vector<BloomLevel> bloomLevels_;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> bloomThresholdPipelineState_;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> bloomDownsamplePipelineState_;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> bloomUpsamplePipelineState_; // 加算ブレンド有効

        // ImGuiで切り替えるフラグ・パラメータ
        // NOTE: 露出は圧縮(ACES/クリップ)の前に常にかかる単純な明るさ調整。ON/OFFの概念はなく常時有効。
        //       ACESフィルミックは、露出後の値を0〜1へ圧縮するカーブ。OFF時は単純なクリップになる。
        bool acesEnabled_ = true;
        float exposure_ = 1.0f;
        bool bloomEnabled_ = false;
        float bloomThreshold_ = 1.0f;   // これを超えた明るさの部分だけがブルームの対象になる
        float bloomIntensity_ = 1.0f;   // 最終的にシーンへ加算する際の強さ

        // --- 追加エフェクト用フラグ・パラメータ ---
        // NOTE: distortion/glitchはUVを歪ませてからサンプリングする(色を歪ませる系より先に適用)。
        //       blur/chromaticAberrationはサンプリング方法そのものを変える。
        //       grayscale/noiseは、露出・トーンマッピングを終えた最終画像に対する仕上げ処理。
        bool distortionEnabled_ = false;
        float distortionStrength_ = 0.01f;

        bool glitchEnabled_ = false;
        float glitchIntensity_ = 0.05f;

        bool chromaticAberrationEnabled_ = false;
        float chromaticAberrationStrength_ = 0.005f;

        bool blurEnabled_ = false;
        float blurStrength_ = 0.003f; // UV空間での直接のオフセット量(簡易実装のため解像度非依存の近似値)

        bool grayscaleEnabled_ = false;
        float grayscaleIntensity_ = 1.0f; // 0:元の色のまま 1:完全に白黒

        bool noiseEnabled_ = false;
        float noiseIntensity_ = 0.05f;

        // ノイズ/グリッチのアニメーションに使う経過時間の計測用
        std::chrono::steady_clock::time_point startTime_;

        // フラグ・パラメータを各ポストプロセスシェーダーへ渡すための定数バッファ
        struct PostProcessParams {
            uint32_t acesEnabled;
            uint32_t bloomEnabled;
            float exposure;
            float threshold;
            float bloomIntensity;

            uint32_t distortionEnabled;
            float distortionStrength;

            uint32_t glitchEnabled;
            float glitchIntensity;

            uint32_t chromaticAberrationEnabled;
            float chromaticAberrationStrength;

            uint32_t blurEnabled;
            float blurStrength;

            uint32_t grayscaleEnabled;
            float grayscaleIntensity;

            uint32_t noiseEnabled;
            float noiseIntensity;

            float time;
            float padding[3];
        };
        Microsoft::WRL::ComPtr<ID3D12Resource> compositeParamsResource_;
        PostProcessParams* compositeParamsData_ = nullptr;
    };
}
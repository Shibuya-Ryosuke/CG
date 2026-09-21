#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <vector>

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
        static PostProcess* GetInstance();

        void Initialize();
        void Finalize();

        void BeginScenePass();
        void EndScenePass();

        /// <summary>
        /// (ブルームON時は内部でRenderBloom()を実行した上で)HDRバッファの内容を、
        /// 露出→ACES(ON)またはクリップ(OFF)によりLDRへ圧縮し、gameRenderTargetResource_へ書き戻す。
        /// EndScenePass()の後、2D描画の前に呼ぶこと。
        /// </summary>
        void Composite();

        /// <summary>
        /// 露出・ACES・ブルームのON/OFFや数値を切り替えるImGuiパネルを描画する。
        /// </summary>
        void DrawImGui();

        // --- 外部から直接切り替えたい場合用 ---
        void SetACESEnabled(bool enabled) { acesEnabled_ = enabled; }
        bool IsACESEnabled() const { return acesEnabled_; }
        void SetExposure(float exposure) { exposure_ = exposure; }
        float GetExposure() const { return exposure_; }
        void SetBloomEnabled(bool enabled) { bloomEnabled_ = enabled; }
        bool IsBloomEnabled() const { return bloomEnabled_; }
        void SetBloomThreshold(float threshold) { bloomThreshold_ = threshold; }
        float GetBloomThreshold() const { return bloomThreshold_; }
        void SetBloomIntensity(float intensity) { bloomIntensity_ = intensity; }
        float GetBloomIntensity() const { return bloomIntensity_; }

    private:
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

        // フラグ・パラメータを各ポストプロセスシェーダーへ渡すための定数バッファ
        struct PostProcessParams {
            uint32_t acesEnabled;
            uint32_t bloomEnabled;
            float exposure;
            float threshold;
            float bloomIntensity;
            float padding[3];
        };
        Microsoft::WRL::ComPtr<ID3D12Resource> compositeParamsResource_;
        PostProcessParams* compositeParamsData_ = nullptr;
    };
}

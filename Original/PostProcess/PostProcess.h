#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>

namespace RyoEngine {

    /// <summary>
    /// 3D描画用のHDR中間バッファ(R16G16B16A16_FLOAT)と、それをLDR(gameRenderTargetResource_)へ
    /// 書き戻す合成(コンポジット)パスを管理するクラス。
    ///
    /// フレームの流れ:
    ///   DirectXCommon::PreDraw() (gameRenderTargetResource_をRTVにセット・クリア。ここは変更なし)
    ///     → PostProcess::BeginScenePass() (RTVだけHDRバッファへ差し替える。DSVは共有のmain深度バッファのまま)
    ///       → 3Dモデルの描画 (ModelCommon::Draw()等。HDRバッファに書き込まれる)
    ///     → PostProcess::EndScenePass() (HDRバッファをSRVとして読める状態に遷移)
    ///     → PostProcess::Composite() (HDRバッファを読み、トーンマッピングしてgameRenderTargetResource_へ書き戻す)
    ///       → 2D/ImGui描画 (今まで通りgameRenderTargetResource_に重ね書き)
    ///
    /// NOTE: 現段階ではブルームは未実装。HDR化と、それをLDRへ戻す合成パスのみ。
    ///       bloomEnabled_はImGuiのUIとフラグだけ先に用意してあり、実際のぼかし処理は今後追加する。
    /// </summary>
    class PostProcess {
    public:
        static PostProcess* GetInstance();

        void Initialize();
        void Finalize();

        void BeginScenePass();
        void EndScenePass();

        /// <summary>
        /// HDRバッファの内容を、露出を掛けた上でACES(ON)またはクリップ(OFF)により
        /// LDRへ圧縮し、gameRenderTargetResource_へ書き戻す。
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

    private:
        PostProcess() = default;
        ~PostProcess() = default;
        PostProcess(const PostProcess&) = delete;
        PostProcess& operator=(const PostProcess&) = delete;

        void CreateSceneColorResource();
        void CreateRootSignatureAndPSO();
        void TransitionSceneColor(D3D12_RESOURCE_STATES newState);

        // HDRシーンカラーバッファ本体 (3D描画がここに書き込まれる)
        Microsoft::WRL::ComPtr<ID3D12Resource> sceneColorResource_;
        D3D12_RESOURCE_STATES sceneColorState_ = D3D12_RESOURCE_STATE_RENDER_TARGET;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> sceneColorRtvHeap_;
        uint32_t sceneColorTextureHandle_ = 0; // TextureManager登録後のSRVハンドル(合成パスで読む用)

        // 合成パス専用のRootSignature/PSO (フルスクリーン三角形を描くだけの軽量な構成)
        Microsoft::WRL::ComPtr<ID3D12RootSignature> compositeRootSignature_;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> compositePipelineState_;

        // ImGuiで切り替えるフラグ・パラメータ
        // NOTE: 露出は圧縮(ACES/クリップ)の前に常にかかる単純な明るさ調整。ON/OFFの概念はなく常時有効。
        //       ACESフィルミックは、露出後の値を0〜1へ圧縮するカーブ。OFF時は単純なクリップになる。
        bool acesEnabled_ = true;
        float exposure_ = 1.0f;
        bool bloomEnabled_ = false; // ブルーム本体は未実装のため、デフォルトはOFF

        // フラグ・パラメータを合成シェーダーへ渡すための定数バッファ
        struct CompositeParams {
            uint32_t acesEnabled;
            uint32_t bloomEnabled;
            float exposure;
            float padding;
        };
        Microsoft::WRL::ComPtr<ID3D12Resource> compositeParamsResource_;
        CompositeParams* compositeParamsData_ = nullptr;
    };
}
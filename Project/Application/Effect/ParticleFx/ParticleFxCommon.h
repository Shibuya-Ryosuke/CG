#pragma once
#include <d3d12.h>
#include <dxcapi.h>
#include <wrl.h>
#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include "ParticleFxTypes.h"

namespace App {

    /// <summary>
    /// ParticleFx (アプリ側のGPUパーティクル) の
    /// ・シミュレーション用コンピュートシェーダー (リセット/本体)
    /// ・描画用シェーダー (通常合成/加算合成)
    /// のRootSignature/PSOをまとめて管理するクラス。
    ///
    /// シェーダーは Resources/ApplicationResources/HLSL/ParticleFx/ にあり、
    /// ReloadShaders() / ファイルの更新の自動検知 で、実行中に作り直せる (ホットリロード)。
    /// コンパイルに失敗したときは、エラーをログに出して前のPSOのまま動かし続ける。
    ///
    /// 実際のGPUコマンドは、エンジンの GPUParticleCommon の予約キュー
    /// (SetDispatchCommands / SetDrawCommands) に積んで、エンジンと同じタイミングで実行してもらう。
    /// </summary>
    class ParticleFxCommon {
    public:
        static void Initialize();
        static void Finalize();

        /// <summary>シェーダーを全部コンパイルし直して、PSOを作り直す。失敗したら前のまま (false)</summary>
        static bool ReloadShaders();

        /// <summary>シェーダーファイルの更新を見張って、変わっていたら作り直す (NRELEASEのみ)</summary>
        static void CheckHotReload(float deltaTime);

        /// <summary>PSOが揃っていて、描画・シミュレーションできるか</summary>
        static bool IsReady();

        static ID3D12RootSignature* GetComputeRootSignature() { return GetInstance()->computeRootSignature_.Get(); }
        static ID3D12PipelineState* GetResetPipelineState() { return GetInstance()->resetPipelineState_.Get(); }
        static ID3D12PipelineState* GetSimulatePipelineState() { return GetInstance()->simulatePipelineState_.Get(); }
        static ID3D12RootSignature* GetRenderRootSignature() { return GetInstance()->renderRootSignature_.Get(); }
        static ID3D12PipelineState* GetRenderPipelineState(ParticleFxBlend blend) {
            return GetInstance()->renderPipelineStates_[(blend == ParticleFxBlend::Additive) ? 1 : 0].Get();
        }

        /// <summary>最後のコンパイルエラー (無ければ空)</summary>
        static const std::string& GetLastError() { return GetInstance()->lastError_; }

        /// <summary>シェーダーの再読み込みボタンなど (エディタの中から呼ぶ。NRELEASEのみ)</summary>
        static void DrawImGui();

    private:
        static ParticleFxCommon* GetInstance();

        ParticleFxCommon() = default;
        ~ParticleFxCommon() = default;
        ParticleFxCommon(const ParticleFxCommon&) = delete;
        ParticleFxCommon& operator=(const ParticleFxCommon&) = delete;
        ParticleFxCommon(ParticleFxCommon&&) = delete;
        ParticleFxCommon& operator=(ParticleFxCommon&&) = delete;

        void CreateComputeRootSignature();
        void CreateRenderRootSignature();
        // シェーダーファイルを読み込んでコンパイルする。失敗したら false (outError にエラー文)
        bool CompileFile(const std::string& fileName, const wchar_t* profile,
            Microsoft::WRL::ComPtr<IDxcBlob>& outBlob, std::string& outError);
        // 今のシェーダーファイルの更新日時を覚え直す
        void RememberWriteTimes();
        bool HasShaderFileChanged();

        Microsoft::WRL::ComPtr<ID3D12RootSignature> computeRootSignature_;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> resetPipelineState_;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> simulatePipelineState_;
        Microsoft::WRL::ComPtr<ID3D12RootSignature> renderRootSignature_;
        // 0: 通常合成 / 1: 加算合成
        Microsoft::WRL::ComPtr<ID3D12PipelineState> renderPipelineStates_[2];

        std::string shaderFolder_ = "Resources/ApplicationResources/HLSL/ParticleFx/";
        std::string resetShaderFile_ = "ParticleFxReset.CS.hlsl";
        std::string simulateShaderFile_ = "ParticleFxSimulate.CS.hlsl";
        std::string vertexShaderFile_ = "ParticleFx.VS.hlsl";
        std::string pixelShaderFile_ = "ParticleFx.PS.hlsl";

        std::string lastError_;
        uint32_t reloadCount_ = 0;

        // --- ホットリロード ---
        bool autoReload_ = true;
        float watchInterval_ = 0.5f; // 何秒おきにファイルの更新を確認するか
        float watchTimer_ = 0.0f;
        std::map<std::string, std::filesystem::file_time_type> writeTimes_;
    };
}

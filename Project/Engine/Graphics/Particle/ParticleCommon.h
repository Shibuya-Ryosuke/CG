#pragma once
#include <d3d12.h>
#include <dxcapi.h>
#include <wrl.h>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <vector>
#include <map>
#include <string>
#include "ParticleTypes.h"

namespace RyoEngine {

    /// <summary>
    /// パーティクル (GPUパーティクル) の
    /// ・シミュレーション用コンピュートシェーダー (リセット/本体)
    /// ・描画用シェーダー (通常合成/加算合成)
    /// のRootSignature/PSOをまとめて管理するクラス。
    ///
    /// シェーダーは Resources/EngineResources/HLSL/Particle/ にあり、
    /// ReloadShaders() / ファイルの更新の自動検知 で、実行中に作り直せる (ホットリロード)。
    /// コンパイルに失敗したときは、エラーをログに出して前のPSOのまま動かし続ける。
    ///
    /// 実際のGPUコマンドはその場では発行せず、SetDispatchCommands / SetDrawCommands で予約しておき、
    /// EndFrame 内の正しいタイミングで Dispatch() / Draw() がまとめて実行する (Model の描画と同じ考え方)。
    /// </summary>
    class ParticleCommon {
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
        static ID3D12PipelineState* GetRenderPipelineState(ParticleBlend blend) {
            return GetInstance()->renderPipelineStates_[(blend == ParticleBlend::Additive) ? 1 : 0].Get();
        }

        /// <summary>最後のコンパイルエラー (無ければ空)</summary>
        static const std::string& GetLastError() { return GetInstance()->lastError_; }

        /// <summary>シェーダーの再読み込みボタンなど (エディタの中から呼ぶ。NRELEASEのみ)</summary>
        static void DrawImGui();

        // --- 予約キュー ---
        // シミュレーション(Dispatch)の予約。EndFrame の早い段階 (シャドウパスより前) に Dispatch() で実行される
        static void SetDispatchCommands(const std::function<void()>& function) { GetInstance()->dispatchCommands_.push_back(function); }
        // 描画の予約。3Dシーンパス中に Draw() で実行される
        static void SetDrawCommands(const std::function<void()>& function) { GetInstance()->drawCommands_.push_back(function); }
        static void Dispatch();
        static void Draw();
        // 予約をクリアする (NewFrame の先頭で呼ぶ)
        static void CommandsClear() { GetInstance()->dispatchCommands_.clear(); GetInstance()->drawCommands_.clear(); }

    private:
        static ParticleCommon* GetInstance();

        ParticleCommon() = default;
        ~ParticleCommon() = default;
        ParticleCommon(const ParticleCommon&) = delete;
        ParticleCommon& operator=(const ParticleCommon&) = delete;
        ParticleCommon(ParticleCommon&&) = delete;
        ParticleCommon& operator=(ParticleCommon&&) = delete;

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

        std::string shaderFolder_ = "Resources/EngineResources/HLSL/Particle/";
        std::string resetShaderFile_ = "ParticleReset.CS.hlsl";
        std::string simulateShaderFile_ = "ParticleSimulate.CS.hlsl";
        std::string vertexShaderFile_ = "Particle.VS.hlsl";
        std::string pixelShaderFile_ = "Particle.PS.hlsl";

        std::string lastError_;
        uint32_t reloadCount_ = 0;

        // --- ホットリロード ---
        bool autoReload_ = true;
        float watchInterval_ = 0.5f; // 何秒おきにファイルの更新を確認するか
        float watchTimer_ = 0.0f;
        std::map<std::string, std::filesystem::file_time_type> writeTimes_;

        // --- 予約キュー ---
        std::vector<std::function<void()>> dispatchCommands_;
        std::vector<std::function<void()>> drawCommands_;
    };
}

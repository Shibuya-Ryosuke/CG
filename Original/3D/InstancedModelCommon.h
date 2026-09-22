#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <functional>

namespace RyoEngine {

    /// <summary>
    /// GPUインスタンシング描画専用のRootSignature/PSOを管理するクラス。
    /// 既存のModelCommonとは完全に別系統(RootSignatureも共有しない)。
    ///
    /// ModelCommonと同様、実際の描画コマンドはその場では発行せず、SetDrawCommands()で
    /// 予約しておき、EndFrame()内の正しいタイミング(3Dシーンパス中)でDraw()がまとめて実行する。
    /// (InstancedModel::Draw()を呼んだ直後にGPUコマンドが飛ぶわけではないので注意)
    ///
    /// 通常のライティング(Directional/Point/Spot/Area+アンビエント)には対応するが、
    /// シャドウの落とし/受けには現状対応していない(将来の拡張課題)。
    /// </summary>
    class InstancedModelCommon {
    public:
        static InstancedModelCommon* GetInstance();

        void Initialize();
        void Finalize();

        ID3D12RootSignature* GetRootSignature() const { return rootSignature_.Get(); }
        ID3D12PipelineState* GetPipelineState() const { return pipelineState_.Get(); }

        // 描画コマンドの予約(InstancedModel::Draw()から呼ばれる)
        void SetDrawCommands(const std::function<void()>& function) { drawCommands_.push_back(function); }
        // 予約された描画コマンドを全て実行する(EndFrame()内、3Dシーンパス中に呼ぶこと)
        void Draw();
        // 予約をクリアする(NewFrame()の先頭で呼ぶこと)
        void CommandsClear() { drawCommands_.clear(); }

    private:
        InstancedModelCommon() = default;
        ~InstancedModelCommon() = default;
        InstancedModelCommon(const InstancedModelCommon&) = delete;
        InstancedModelCommon& operator=(const InstancedModelCommon&) = delete;

        void CreateRootSignature();
        void CreatePipelineState();

        Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;

        std::vector<std::function<void()>> drawCommands_;
    };
}
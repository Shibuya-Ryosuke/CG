#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <vector>
#include "ParticleTypes.h"

namespace RyoEngine {

    /// <summary>
    /// Particle の「1つのエフェクトぶんのGPUバッファ」を持つクラス。
    /// 発生の命令を積み (AddBatch)、パラメータを渡してシミュレーションを予約し (Update)、描画を予約する (Draw)。
    ///
    /// CPUが送るのは「どこに何粒」という命令だけで、1粒ずつの初期値(位置・速度・色…)は
    /// シミュレーションのコンピュートシェーダーが乱数で作る (1フレームに何万粒出してもCPUは重くならない)。
    ///
    /// ・パーティクル本体 / カウンター / 分裂の子のリスト : DEFAULT heap (UAV)
    /// ・発生の命令 / パラメータ                          : Upload heap (CPUが毎フレーム書く)
    /// ・生きている数                                     : Readback heap (1フレーム遅れで読む)
    /// </summary>
    class ParticleEmitter {
    public:
        /// <summary>1フレームに積める発生の命令の数 (= 1フレームに何回 Play できるか)</summary>
        static constexpr uint32_t kMaxBatchesPerFrame = 32;

        ParticleEmitter() = default;
        ~ParticleEmitter() = default;
        ParticleEmitter(const ParticleEmitter&) = delete;
        ParticleEmitter& operator=(const ParticleEmitter&) = delete;
        ParticleEmitter(ParticleEmitter&&) = delete;
        ParticleEmitter& operator=(ParticleEmitter&&) = delete;

        /// <param name="maxCount">同時に存在できる最大数</param>
        /// <param name="useSplit">分裂を使うか。false なら分裂の子のリストを確保しない (メモリが約1/3になる)</param>
        void Initialize(uint32_t maxCount, bool useSplit);
        void Finalize();

        /// <summary>
        /// 最大数・分裂の有無を変える。出ている粒は新しいバッファへコピーして引き継ぐ
        /// (新しい最大数より後ろの粒と、分裂待ちの子だけは消える)。初期化前なら Initialize と同じ
        /// </summary>
        void Resize(uint32_t maxCount, bool useSplit);

        bool IsInitialized() const { return particleResource_ != nullptr; }
        uint32_t GetMaxCount() const { return maxCount_; }
        /// <summary>分裂の子のリストを確保しているか</summary>
        bool UsesSplit() const { return useSplit_; }

        /// <summary>今生きている粒の数 (1フレーム前の値。分裂の子も含む)</summary>
        uint32_t GetAliveCount() const { return aliveCount_; }

        /// <summary>発生の命令を積む。このフレームの命令が上限(kMaxBatchesPerFrame)なら false</summary>
        bool AddBatch(const ParticleSpawnBatch& batch);

        /// <summary>次のシミュレーションで、今いる粒を全部消す</summary>
        void RequestClear() { clearRequested_ = true; }

        /// <summary>
        /// シミュレーションを予約する。
        /// simParams の spawnCount / maxCount / childReadSlot / clear / batchCount はこの中で上書きする。
        /// 実際のDispatchは EndFrame 内 (ParticleCommon::Dispatch) で行われる
        /// </summary>
        void Update(const ParticleSimParams& simParams, const ParticleSpawnParams& spawnParams);

        /// <summary>
        /// 描画を予約する。実際の描画は EndFrame 内 (ParticleCommon::Draw) で行われる
        /// </summary>
        void Draw(const ParticleRenderParams& params, uint32_t textureHandle, ParticleBlend blend);

    private:
        void InternalDispatch();
        void InternalDraw();
        void TransitionParticleBuffer(D3D12_RESOURCE_STATES newState);
        // 前のフレームのDispatchで数えた「生きている数」を読む
        void ReadAliveCount();
        // GPUバッファ (本体・カウンター・分裂の子・読み戻し) を作る
        void CreateParticleBuffers();
        // Resize で古いバッファから新しいバッファへ、出ている粒をコピーする (Dispatch の先頭で行う)
        void CopyFromRetiredBuffer();

        uint32_t maxCount_ = 0;
        bool useSplit_ = false;

        Microsoft::WRL::ComPtr<ID3D12Resource> particleResource_;
        D3D12_RESOURCE_STATES particleState_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        Microsoft::WRL::ComPtr<ID3D12Resource> counterResource_; // uint x 4 ([0]配った数 [1][2]分裂の子の数 [3]生きている数)
        Microsoft::WRL::ComPtr<ID3D12Resource> childResource_;   // 分裂の子のリスト (分裂を使うなら maxCount_ * 2、使わないなら最小限)
        Microsoft::WRL::ComPtr<ID3D12Resource> readbackResource_; // 生きている数をCPUで読むためのコピー先

        Microsoft::WRL::ComPtr<ID3D12Resource> batchResource_;
        ParticleSpawnBatch* batchData_ = nullptr;
        std::vector<ParticleSpawnBatch> pendingBatches_;

        Microsoft::WRL::ComPtr<ID3D12Resource> simParamsResource_;
        ParticleSimParams* simParamsData_ = nullptr;

        Microsoft::WRL::ComPtr<ID3D12Resource> spawnParamsResource_;
        ParticleSpawnParams* spawnParamsData_ = nullptr;

        Microsoft::WRL::ComPtr<ID3D12Resource> renderParamsResource_;
        ParticleRenderParams* renderParamsData_ = nullptr;

        uint32_t textureHandle_ = 0;
        ParticleBlend blend_ = ParticleBlend::Additive;

        uint32_t childReadSlot_ = 0; // 前フレームの分裂の子を読む側 (毎フレーム入れ替える)
        bool clearRequested_ = false;

        uint32_t aliveCount_ = 0;
        bool hasReadback_ = false;   // Dispatchを1回以上予約して、読める値があるか

        // --- Resize で粒を引き継ぐための、古いバッファ ---
        // コピーが済んで、GPUが使い終わった次のフレームの Update で解放する
        Microsoft::WRL::ComPtr<ID3D12Resource> retiredParticleResource_;
        D3D12_RESOURCE_STATES retiredParticleState_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        uint32_t retiredMaxCount_ = 0;
        bool copyPending_ = false;   // 次の Dispatch で古いバッファからコピーするか
    };
}

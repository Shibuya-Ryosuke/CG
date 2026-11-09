#include "ParticleEmitter.h"
#include "ParticleCommon.h"
#include "../../Core/Base/DirectXCommon.h"
#include "../../Core/Base/SrvManager.h"
#include "../2D/TextureManager.h"
#include <algorithm>
#include <cassert>

namespace RyoEngine {

    namespace {
        // バッファを作る共通処理 (committed resource なので中身は0で初期化されている)
        Microsoft::WRL::ComPtr<ID3D12Resource> CreateBuffer(size_t sizeInBytes, D3D12_HEAP_TYPE heapType,
            D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initialState) {
            D3D12_RESOURCE_DESC resourceDesc{};
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            resourceDesc.Width = sizeInBytes;
            resourceDesc.Height = 1;
            resourceDesc.DepthOrArraySize = 1;
            resourceDesc.MipLevels = 1;
            resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
            resourceDesc.SampleDesc.Count = 1;
            resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            resourceDesc.Flags = flags;

            D3D12_HEAP_PROPERTIES heapProperties{};
            heapProperties.Type = heapType;

            Microsoft::WRL::ComPtr<ID3D12Resource> resource;
            HRESULT hr = DirectXCommon::GetDevice()->CreateCommittedResource(
                &heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                initialState, nullptr, IID_PPV_ARGS(&resource));
            assert(SUCCEEDED(hr));
            return resource;
        }

        // DEFAULT heap上の、UAVとして読み書きできるバッファ
        Microsoft::WRL::ComPtr<ID3D12Resource> CreateUAVBuffer(size_t sizeInBytes) {
            return CreateBuffer(sizeInBytes, D3D12_HEAP_TYPE_DEFAULT,
                D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        }

        // 定数バッファは256byte単位の大きさにしておく
        size_t AlignTo256(size_t size) {
            return (size + 255) & ~static_cast<size_t>(255);
        }
    }

    void ParticleEmitter::CreateParticleBuffers() {
        particleResource_ = CreateUAVBuffer(sizeof(ParticleGPUData) * maxCount_);
        particleState_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        counterResource_ = CreateUAVBuffer(sizeof(uint32_t) * 4);
        // 分裂を使わないときは、シェーダーが子のリストに触れないので最小限だけ確保する
        // (ルート引数にはアドレスが必要なので、空にはしない)
        const size_t childCount = useSplit_ ? static_cast<size_t>(maxCount_) * 2 : 2;
        childResource_ = CreateUAVBuffer(sizeof(ParticleGPUData) * childCount);
        readbackResource_ = CreateBuffer(sizeof(uint32_t), D3D12_HEAP_TYPE_READBACK,
            D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST);
        childReadSlot_ = 0;
        hasReadback_ = false;
    }

    void ParticleEmitter::Resize(uint32_t maxCount, bool useSplit) {
        if (!IsInitialized()) {
            Initialize(maxCount, useSplit);
            return;
        }

        // コピー元は「まだGPUで書かれたことのあるバッファ」。同じフレームに何度 Resize しても、最初の1つを使う
        if (!copyPending_) {
            retiredParticleResource_ = particleResource_;
            retiredParticleState_ = particleState_;
            retiredMaxCount_ = maxCount_;
        }

        maxCount_ = (maxCount < 1) ? 1 : maxCount;
        useSplit_ = useSplit;
        CreateParticleBuffers();
        copyPending_ = true;
    }

    void ParticleEmitter::CopyFromRetiredBuffer() {
        auto commandList = DirectXCommon::GetCommandList();

        D3D12_RESOURCE_BARRIER toCopy[2] = {};
        toCopy[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        toCopy[0].Transition.pResource = retiredParticleResource_.Get();
        toCopy[0].Transition.StateBefore = retiredParticleState_;
        toCopy[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
        toCopy[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        toCopy[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        toCopy[1].Transition.pResource = particleResource_.Get();
        toCopy[1].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        toCopy[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
        toCopy[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        // 古いバッファがもともと COPY_SOURCE なら遷移は要らない
        const UINT barrierCount = (retiredParticleState_ == D3D12_RESOURCE_STATE_COPY_SOURCE) ? 1u : 2u;
        commandList->ResourceBarrier(barrierCount, (barrierCount == 2u) ? toCopy : &toCopy[1]);

        // 新しい最大数に収まる分だけコピーする
        const uint32_t copyCount = (std::min)(retiredMaxCount_, maxCount_);
        commandList->CopyBufferRegion(particleResource_.Get(), 0, retiredParticleResource_.Get(), 0,
            sizeof(ParticleGPUData) * static_cast<UINT64>(copyCount));

        D3D12_RESOURCE_BARRIER toUAV{};
        toUAV.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        toUAV.Transition.pResource = particleResource_.Get();
        toUAV.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        toUAV.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        toUAV.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &toUAV);
        particleState_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

        // 古いバッファは、GPUがこのコピーを終えた後 (次のフレームの Update) で解放する
        retiredParticleState_ = D3D12_RESOURCE_STATE_COPY_SOURCE;
        copyPending_ = false;
    }

    void ParticleEmitter::Initialize(uint32_t maxCount, bool useSplit) {
        Finalize();

        maxCount_ = (maxCount < 1) ? 1 : maxCount;
        useSplit_ = useSplit;
        ID3D12Device* device = DirectXCommon::GetDevice();

        CreateParticleBuffers();

        // 発生の命令 (1フレーム kMaxBatchesPerFrame 件まで)
        batchResource_ = DirectXCommon::CreateBufferResource(device, sizeof(ParticleSpawnBatch) * kMaxBatchesPerFrame);
        batchResource_->Map(0, nullptr, reinterpret_cast<void**>(&batchData_));
        pendingBatches_.reserve(kMaxBatchesPerFrame);

        simParamsResource_ = DirectXCommon::CreateBufferResource(device, AlignTo256(sizeof(ParticleSimParams)));
        simParamsResource_->Map(0, nullptr, reinterpret_cast<void**>(&simParamsData_));
        *simParamsData_ = ParticleSimParams{};

        spawnParamsResource_ = DirectXCommon::CreateBufferResource(device, AlignTo256(sizeof(ParticleSpawnParams)));
        spawnParamsResource_->Map(0, nullptr, reinterpret_cast<void**>(&spawnParamsData_));
        *spawnParamsData_ = ParticleSpawnParams{};

        renderParamsResource_ = DirectXCommon::CreateBufferResource(device, AlignTo256(sizeof(ParticleRenderParams)));
        renderParamsResource_->Map(0, nullptr, reinterpret_cast<void**>(&renderParamsData_));
        *renderParamsData_ = ParticleRenderParams{};

        textureHandle_ = TextureManager::GetWhiteTex();
        clearRequested_ = false;
        aliveCount_ = 0;
    }

    void ParticleEmitter::Finalize() {
        renderParamsResource_.Reset();
        renderParamsData_ = nullptr;
        spawnParamsResource_.Reset();
        spawnParamsData_ = nullptr;
        simParamsResource_.Reset();
        simParamsData_ = nullptr;
        batchResource_.Reset();
        batchData_ = nullptr;
        pendingBatches_.clear();
        readbackResource_.Reset();
        retiredParticleResource_.Reset();
        retiredMaxCount_ = 0;
        copyPending_ = false;
        childResource_.Reset();
        counterResource_.Reset();
        particleResource_.Reset();
        maxCount_ = 0;
        useSplit_ = false;
        aliveCount_ = 0;
        hasReadback_ = false;
    }

    bool ParticleEmitter::AddBatch(const ParticleSpawnBatch& batch) {
        if (batch.count == 0) {
            return true;
        }
        if (pendingBatches_.size() >= kMaxBatchesPerFrame) {
            return false;
        }
        pendingBatches_.push_back(batch);
        return true;
    }

    void ParticleEmitter::ReadAliveCount() {
        D3D12_RANGE readRange{};
        readRange.Begin = 0;
        readRange.End = sizeof(uint32_t);
        void* mapped = nullptr;
        if (SUCCEEDED(readbackResource_->Map(0, &readRange, &mapped)) && mapped) {
            aliveCount_ = *static_cast<const uint32_t*>(mapped);
            D3D12_RANGE writtenRange{}; // CPUからは何も書いていない
            readbackResource_->Unmap(0, &writtenRange);
        }
    }

    void ParticleEmitter::Update(const ParticleSimParams& simParams, const ParticleSpawnParams& spawnParams) {
        if (!IsInitialized()) {
            return;
        }

        // 前のフレームのGPU処理は終わっているので、そのとき数えた値を読める
        if (hasReadback_) {
            ReadAliveCount();
        }
        // Resize のコピーに使った古いバッファも、もう使い終わっている
        if (!copyPending_ && retiredParticleResource_) {
            retiredParticleResource_.Reset();
        }

        // 発生の命令に通し番号を振って詰める (合計が最大数を超えた分は切り捨て)
        uint32_t spawnCount = 0;
        uint32_t batchCount = 0;
        for (const ParticleSpawnBatch& pending : pendingBatches_) {
            if (batchCount >= kMaxBatchesPerFrame || spawnCount >= maxCount_) {
                break;
            }
            ParticleSpawnBatch batch = pending;
            batch.firstIndex = spawnCount;
            batch.count = (std::min)(batch.count, maxCount_ - spawnCount);
            batchData_[batchCount] = batch;
            spawnCount += batch.count;
            ++batchCount;
        }
        pendingBatches_.clear();

        *simParamsData_ = simParams;
        simParamsData_->spawnCount = spawnCount;
        simParamsData_->maxCount = maxCount_;
        simParamsData_->childReadSlot = childReadSlot_;
        simParamsData_->clear = clearRequested_ ? 1u : 0u;
        simParamsData_->batchCount = batchCount;
        clearRequested_ = false;

        *spawnParamsData_ = spawnParams;

        // 今フレームに書いた子は、次のフレームに読む
        childReadSlot_ = 1u - childReadSlot_;

        if (!ParticleCommon::IsReady()) {
            return;
        }
        ParticleCommon::SetDispatchCommands([this]() {
            InternalDispatch();
            });
        hasReadback_ = true;
    }

    void ParticleEmitter::Draw(const ParticleRenderParams& params, uint32_t textureHandle, ParticleBlend blend) {
        if (!IsInitialized() || !ParticleCommon::IsReady()) {
            return;
        }
        *renderParamsData_ = params;
        textureHandle_ = textureHandle;
        blend_ = blend;

        ParticleCommon::SetDrawCommands([this]() {
            InternalDraw();
            });
    }

    void ParticleEmitter::TransitionParticleBuffer(D3D12_RESOURCE_STATES newState) {
        if (particleState_ == newState) {
            return;
        }
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = particleResource_.Get();
        barrier.Transition.StateBefore = particleState_;
        barrier.Transition.StateAfter = newState;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        DirectXCommon::GetCommandList()->ResourceBarrier(1, &barrier);
        particleState_ = newState;
    }

    void ParticleEmitter::InternalDispatch() {
        auto commandList = DirectXCommon::GetCommandList();

        TransitionParticleBuffer(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        // 最大数を変えた直後なら、出ている粒を古いバッファから引き継ぐ
        if (copyPending_ && retiredParticleResource_) {
            CopyFromRetiredBuffer();
        }

        commandList->SetComputeRootSignature(ParticleCommon::GetComputeRootSignature());
        commandList->SetComputeRootUnorderedAccessView(0, particleResource_->GetGPUVirtualAddress());
        commandList->SetComputeRootUnorderedAccessView(1, counterResource_->GetGPUVirtualAddress());
        commandList->SetComputeRootShaderResourceView(2, batchResource_->GetGPUVirtualAddress());
        commandList->SetComputeRootConstantBufferView(3, simParamsResource_->GetGPUVirtualAddress());
        commandList->SetComputeRootUnorderedAccessView(4, childResource_->GetGPUVirtualAddress());
        commandList->SetComputeRootConstantBufferView(5, spawnParamsResource_->GetGPUVirtualAddress());

        // ① カウンターを0に戻す
        commandList->SetPipelineState(ParticleCommon::GetResetPipelineState());
        commandList->Dispatch(1, 1, 1);

        // リセットの書き込みが終わってから本体が読むように
        D3D12_RESOURCE_BARRIER uavBarrier{};
        uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        uavBarrier.UAV.pResource = counterResource_.Get();
        commandList->ResourceBarrier(1, &uavBarrier);

        // ② 本体 (発生・移動・生きている数を数える)
        commandList->SetPipelineState(ParticleCommon::GetSimulatePipelineState());
        commandList->Dispatch((maxCount_ + 255) / 256, 1, 1);

        // ③ 分裂の子のリストは次のフレームで読むので書き込みを確定させ、
        //    カウンターは「生きている数」をCPUで読めるようにコピーする
        D3D12_RESOURCE_BARRIER barriers[2] = {};
        barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barriers[0].UAV.pResource = childResource_.Get();
        barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers[1].Transition.pResource = counterResource_.Get();
        barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
        barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(2, barriers);

        commandList->CopyBufferRegion(readbackResource_.Get(), 0, counterResource_.Get(), sizeof(uint32_t) * 3, sizeof(uint32_t));

        D3D12_RESOURCE_BARRIER backToUAV{};
        backToUAV.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        backToUAV.Transition.pResource = counterResource_.Get();
        backToUAV.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
        backToUAV.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        backToUAV.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &backToUAV);
    }

    void ParticleEmitter::InternalDraw() {
        auto commandList = DirectXCommon::GetCommandList();

        // 頂点シェーダーが読めるように UAV → SRV へ
        TransitionParticleBuffer(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        commandList->SetGraphicsRootSignature(ParticleCommon::GetRenderRootSignature());
        commandList->SetPipelineState(ParticleCommon::GetRenderPipelineState(blend_));

        SrvManager::PreDraw(commandList);

        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->SetGraphicsRootShaderResourceView(0, particleResource_->GetGPUVirtualAddress());
        commandList->SetGraphicsRootConstantBufferView(1, renderParamsResource_->GetGPUVirtualAddress());
        commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetGPUHandle(textureHandle_));

        // 頂点バッファは使わず、6頂点(板1枚)を最大数ぶんインスタンス描画する。
        // 死んでいる粒は頂点シェーダーで面積0に潰される
        commandList->DrawInstanced(6, maxCount_, 0, 0);
    }
}

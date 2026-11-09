#include "ParticleFxEmitter.h"
#include "ParticleFxCommon.h"
#include <algorithm>
#include <cassert>

using namespace RyoEngine;

namespace App {

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

    void ParticleFxEmitter::Initialize(uint32_t maxCount, bool useSplit) {
        Finalize();

        maxCount_ = (maxCount < 1) ? 1 : maxCount;
        useSplit_ = useSplit;
        ID3D12Device* device = DirectXCommon::GetDevice();

        particleResource_ = CreateUAVBuffer(sizeof(ParticleFxGPUData) * maxCount_);
        particleState_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        counterResource_ = CreateUAVBuffer(sizeof(uint32_t) * 4);
        // 分裂を使わないときは、シェーダーが子のリストに触れないので最小限だけ確保する
        // (ルート引数にはアドレスが必要なので、空にはしない)
        const size_t childCount = useSplit_ ? static_cast<size_t>(maxCount_) * 2 : 2;
        childResource_ = CreateUAVBuffer(sizeof(ParticleFxGPUData) * childCount);
        readbackResource_ = CreateBuffer(sizeof(uint32_t), D3D12_HEAP_TYPE_READBACK,
            D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST);

        // 発生の命令 (1フレーム kMaxBatchesPerFrame 件まで)
        batchResource_ = DirectXCommon::CreateBufferResource(device, sizeof(ParticleFxSpawnBatch) * kMaxBatchesPerFrame);
        batchResource_->Map(0, nullptr, reinterpret_cast<void**>(&batchData_));
        pendingBatches_.reserve(kMaxBatchesPerFrame);

        simParamsResource_ = DirectXCommon::CreateBufferResource(device, AlignTo256(sizeof(ParticleFxSimParams)));
        simParamsResource_->Map(0, nullptr, reinterpret_cast<void**>(&simParamsData_));
        *simParamsData_ = ParticleFxSimParams{};

        spawnParamsResource_ = DirectXCommon::CreateBufferResource(device, AlignTo256(sizeof(ParticleFxSpawnParams)));
        spawnParamsResource_->Map(0, nullptr, reinterpret_cast<void**>(&spawnParamsData_));
        *spawnParamsData_ = ParticleFxSpawnParams{};

        renderParamsResource_ = DirectXCommon::CreateBufferResource(device, AlignTo256(sizeof(ParticleFxRenderParams)));
        renderParamsResource_->Map(0, nullptr, reinterpret_cast<void**>(&renderParamsData_));
        *renderParamsData_ = ParticleFxRenderParams{};

        textureHandle_ = TextureManager::GetWhiteTex();
        childReadSlot_ = 0;
        clearRequested_ = false;
        aliveCount_ = 0;
        hasReadback_ = false;
    }

    void ParticleFxEmitter::Finalize() {
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
        childResource_.Reset();
        counterResource_.Reset();
        particleResource_.Reset();
        maxCount_ = 0;
        useSplit_ = false;
        aliveCount_ = 0;
        hasReadback_ = false;
    }

    bool ParticleFxEmitter::AddBatch(const ParticleFxSpawnBatch& batch) {
        if (batch.count == 0) {
            return true;
        }
        if (pendingBatches_.size() >= kMaxBatchesPerFrame) {
            return false;
        }
        pendingBatches_.push_back(batch);
        return true;
    }

    void ParticleFxEmitter::ReadAliveCount() {
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

    void ParticleFxEmitter::Update(const ParticleFxSimParams& simParams, const ParticleFxSpawnParams& spawnParams) {
        if (!IsInitialized()) {
            return;
        }

        // 前のフレームのGPU処理は終わっているので、そのとき数えた値を読める
        if (hasReadback_) {
            ReadAliveCount();
        }

        // 発生の命令に通し番号を振って詰める (合計が最大数を超えた分は切り捨て)
        uint32_t spawnCount = 0;
        uint32_t batchCount = 0;
        for (const ParticleFxSpawnBatch& pending : pendingBatches_) {
            if (batchCount >= kMaxBatchesPerFrame || spawnCount >= maxCount_) {
                break;
            }
            ParticleFxSpawnBatch batch = pending;
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

        if (!ParticleFxCommon::IsReady()) {
            return;
        }
        GPUParticleCommon::SetDispatchCommands([this]() {
            InternalDispatch();
            });
        hasReadback_ = true;
    }

    void ParticleFxEmitter::Draw(const ParticleFxRenderParams& params, uint32_t textureHandle, ParticleFxBlend blend) {
        if (!IsInitialized() || !ParticleFxCommon::IsReady()) {
            return;
        }
        *renderParamsData_ = params;
        textureHandle_ = textureHandle;
        blend_ = blend;

        GPUParticleCommon::SetDrawCommands([this]() {
            InternalDraw();
            });
    }

    void ParticleFxEmitter::TransitionParticleBuffer(D3D12_RESOURCE_STATES newState) {
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

    void ParticleFxEmitter::InternalDispatch() {
        auto commandList = DirectXCommon::GetCommandList();

        TransitionParticleBuffer(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        commandList->SetComputeRootSignature(ParticleFxCommon::GetComputeRootSignature());
        commandList->SetComputeRootUnorderedAccessView(0, particleResource_->GetGPUVirtualAddress());
        commandList->SetComputeRootUnorderedAccessView(1, counterResource_->GetGPUVirtualAddress());
        commandList->SetComputeRootShaderResourceView(2, batchResource_->GetGPUVirtualAddress());
        commandList->SetComputeRootConstantBufferView(3, simParamsResource_->GetGPUVirtualAddress());
        commandList->SetComputeRootUnorderedAccessView(4, childResource_->GetGPUVirtualAddress());
        commandList->SetComputeRootConstantBufferView(5, spawnParamsResource_->GetGPUVirtualAddress());

        // ① カウンターを0に戻す
        commandList->SetPipelineState(ParticleFxCommon::GetResetPipelineState());
        commandList->Dispatch(1, 1, 1);

        // リセットの書き込みが終わってから本体が読むように
        D3D12_RESOURCE_BARRIER uavBarrier{};
        uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        uavBarrier.UAV.pResource = counterResource_.Get();
        commandList->ResourceBarrier(1, &uavBarrier);

        // ② 本体 (発生・移動・生きている数を数える)
        commandList->SetPipelineState(ParticleFxCommon::GetSimulatePipelineState());
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

    void ParticleFxEmitter::InternalDraw() {
        auto commandList = DirectXCommon::GetCommandList();

        // 頂点シェーダーが読めるように UAV → SRV へ
        TransitionParticleBuffer(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        commandList->SetGraphicsRootSignature(ParticleFxCommon::GetRenderRootSignature());
        commandList->SetPipelineState(ParticleFxCommon::GetRenderPipelineState(blend_));

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

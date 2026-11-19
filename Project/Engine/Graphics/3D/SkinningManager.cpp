#include "SkinningManager.h"
#include "ModelManager.h"
#include "../../Core/Base/DirectXCommon.h"
#include "../../Core/Base/Logger.h"
#include "../Shader/ShaderCompiler.h"
#include <cassert>

namespace RyoEngine {

    SkinningManager* SkinningManager::GetInstance() {
        static SkinningManager instance;
        return &instance;
    }

    void SkinningManager::Initialize() {
        SkinningManager* instance = GetInstance();
        Logger::Log("SkinningManager : Initializing...\n");
        instance->CreateRootSignature();
        instance->CreatePipelineState();
        Logger::LogSuccess("SkinningManager : Initialized\n");
    }

    void SkinningManager::Finalize() {
        SkinningManager* instance = GetInstance();
        Logger::Log("SkinningManager : Finalizing...\n");
        instance->requests_.clear();
        instance->retired_.clear();
        instance->instances_.clear();
        instance->freeInstances_.clear();
        instance->pipelineState_.Reset();
        instance->rootSignature_.Reset();
        Logger::LogSuccess("SkinningManager : Finalized\n");
    }

    void SkinningManager::NewFrame() {
        SkinningManager* instance = GetInstance();
        for (RetiredInstance& retired : instance->retired_) {
            --retired.framesLeft;
        }
        std::erase_if(instance->retired_, [](const RetiredInstance& r) { return r.framesLeft <= 0; });
    }

    // ------------------------------------------------------------------
    // 1体分のバッファ
    // ------------------------------------------------------------------
    uint32_t SkinningManager::CreateInstance(uint32_t modelIndex) {
        SkinningManager* self = GetInstance();
        const ModelManager::ModelAsset* asset = ModelManager::GetAsset(modelIndex);
        if (!asset || !asset->hasSkeleton || !self->pipelineState_) {
            return kInvalidInstance;
        }

        auto device = DirectXCommon::GetDevice();
        auto instance = std::make_unique<Instance>();
        instance->modelIndex = modelIndex;
        instance->meshes.resize(asset->meshes.size());

        for (size_t i = 0; i < asset->meshes.size(); ++i) {
            const ModelManager::MeshAsset& meshAsset = asset->meshes[i];
            if (!meshAsset.isSkinned) {
                continue;
            }
            MeshBuffers& buffers = instance->meshes[i];
            buffers.input = meshAsset.skinVertexResource;
            buffers.vertexCount = meshAsset.vertexCount;
            buffers.paletteCount = static_cast<uint32_t>(meshAsset.skinBones.size());

            // 行列パレット (アップロードヒープ。毎フレーム CPU から書く)
            buffers.palette = DirectXCommon::CreateBufferResource(device, sizeof(Well) * buffers.paletteCount);
            buffers.palette->Map(0, nullptr, reinterpret_cast<void**>(&buffers.paletteMapped));

            // スキニング後の頂点 (デフォルトヒープ。コンピュートシェーダーが書き、頂点バッファとして読む)
            const UINT64 outputBytes = static_cast<UINT64>(sizeof(VertexData)) * buffers.vertexCount;
            D3D12_HEAP_PROPERTIES heapProperties{};
            heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
            D3D12_RESOURCE_DESC desc{};
            desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            desc.Width = outputBytes;
            desc.Height = 1;
            desc.DepthOrArraySize = 1;
            desc.MipLevels = 1;
            desc.SampleDesc.Count = 1;
            desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            HRESULT hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&buffers.output));
            if (FAILED(hr)) {
                Logger::LogError("[SkinningManager] スキニング用のバッファを作れませんでした: {}\n", asset->filePath);
                return kInvalidInstance;
            }
            buffers.state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            buffers.vertexBufferView.BufferLocation = buffers.output->GetGPUVirtualAddress();
            buffers.vertexBufferView.SizeInBytes = static_cast<UINT>(outputBytes);
            buffers.vertexBufferView.StrideInBytes = sizeof(VertexData);
        }

        // 空いている番号を使い回す
        if (!self->freeInstances_.empty()) {
            const uint32_t id = self->freeInstances_.back();
            self->freeInstances_.pop_back();
            self->instances_[id - 1] = std::move(instance);
            return id;
        }
        self->instances_.push_back(std::move(instance));
        return static_cast<uint32_t>(self->instances_.size());
    }

    void SkinningManager::ReleaseInstance(uint32_t instance) {
        SkinningManager* self = GetInstance();
        if (instance == kInvalidInstance || instance > self->instances_.size() || !self->instances_[instance - 1]) {
            return; // Finalize の後に Model が消えた場合など
        }
        // 予約が残っていれば取り消す (このフレームの Draw の後に消された場合)
        std::erase_if(self->requests_, [instance](const RequestEntry& r) { return r.instance == instance; });

        // このフレームの描画でまだ使うかもしれないので、すぐには解放しない
        self->retired_.push_back({ std::move(self->instances_[instance - 1]), kRetireFrames });
        self->freeInstances_.push_back(instance);
    }

    SkinningManager::Instance* SkinningManager::Find(uint32_t instance) {
        if (instance == kInvalidInstance || instance > instances_.size()) {
            return nullptr;
        }
        return instances_[instance - 1].get();
    }

    // ------------------------------------------------------------------
    // 予約・実行
    // ------------------------------------------------------------------
    void SkinningManager::Request(uint32_t instance, uint32_t meshIndex, const std::vector<Matrix4x4>& jointMatrices) {
        SkinningManager* self = GetInstance();
        Instance* target = self->Find(instance);
        if (!target || meshIndex >= target->meshes.size()) {
            return;
        }
        MeshBuffers& buffers = target->meshes[meshIndex];
        if (!buffers.output) {
            return; // スキニングしないメッシュ
        }
        const uint64_t frame = ModelManager::GetFrameIndex();
        if (buffers.requestedFrame == frame && buffers.computed) {
            return; // このフレームはもう予約済み
        }

        const ModelManager::ModelAsset* asset = ModelManager::GetAsset(target->modelIndex);
        if (!asset || meshIndex >= asset->meshes.size()) {
            return;
        }
        // 行列パレット: 元の姿勢の逆行列 × 今のボーンの行列 (頂点をメッシュの座標 → ボーンから見た座標 → 今のボーンの位置へ)
        const std::vector<SkinBone>& bones = asset->meshes[meshIndex].skinBones;
        for (size_t b = 0; b < bones.size() && b < buffers.paletteCount; ++b) {
            const size_t joint = static_cast<size_t>(bones[b].jointIndex);
            const Matrix4x4 matrix = joint < jointMatrices.size()
                ? bones[b].inverseBindMatrix * jointMatrices[joint]
                : bones[b].inverseBindMatrix;
            buffers.paletteMapped[b].skeletonSpaceMatrix = matrix;
            buffers.paletteMapped[b].skeletonSpaceInverseTransposeMatrix = Transpose(Inverse(matrix));
        }

        buffers.requestedFrame = frame;
        buffers.computed = true; // Dispatch は描画より前に実行されるので、このフレームから使える
        self->requests_.push_back({ instance, meshIndex });
    }

    const D3D12_VERTEX_BUFFER_VIEW* SkinningManager::GetVertexBufferView(uint32_t instance, uint32_t meshIndex) {
        Instance* target = GetInstance()->Find(instance);
        if (!target || meshIndex >= target->meshes.size()) {
            return nullptr;
        }
        const MeshBuffers& buffers = target->meshes[meshIndex];
        return (buffers.output && buffers.computed) ? &buffers.vertexBufferView : nullptr;
    }

    void SkinningManager::Dispatch() {
        SkinningManager* self = GetInstance();
        if (self->requests_.empty()) {
            return;
        }
        auto commandList = DirectXCommon::GetCommandList();

        // 1. 書き込める状態 (UAV) にする
        std::vector<D3D12_RESOURCE_BARRIER> barriers;
        barriers.reserve(self->requests_.size());
        for (const RequestEntry& request : self->requests_) {
            MeshBuffers& buffers = self->instances_[request.instance - 1]->meshes[request.meshIndex];
            if (buffers.state != D3D12_RESOURCE_STATE_UNORDERED_ACCESS) {
                D3D12_RESOURCE_BARRIER barrier{};
                barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier.Transition.pResource = buffers.output.Get();
                barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                barrier.Transition.StateBefore = buffers.state;
                barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                barriers.push_back(barrier);
                buffers.state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            }
        }
        if (!barriers.empty()) {
            commandList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
        }

        // 2. スキニング
        commandList->SetComputeRootSignature(self->rootSignature_.Get());
        commandList->SetPipelineState(self->pipelineState_.Get());
        for (const RequestEntry& request : self->requests_) {
            const MeshBuffers& buffers = self->instances_[request.instance - 1]->meshes[request.meshIndex];
            // root 0: 行列パレット (t0) / 1: 入力の頂点 (t1) / 2: 出力の頂点 (u0) / 3: 頂点の数 (b0)
            commandList->SetComputeRootShaderResourceView(0, buffers.palette->GetGPUVirtualAddress());
            commandList->SetComputeRootShaderResourceView(1, buffers.input->GetGPUVirtualAddress());
            commandList->SetComputeRootUnorderedAccessView(2, buffers.output->GetGPUVirtualAddress());
            commandList->SetComputeRoot32BitConstants(3, 1, &buffers.vertexCount, 0);
            commandList->Dispatch((buffers.vertexCount + kThreadGroupSize - 1) / kThreadGroupSize, 1, 1);
        }

        // 3. 頂点バッファとして読める状態にする
        barriers.clear();
        for (const RequestEntry& request : self->requests_) {
            MeshBuffers& buffers = self->instances_[request.instance - 1]->meshes[request.meshIndex];
            D3D12_RESOURCE_BARRIER barrier{};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource = buffers.output.Get();
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
            barriers.push_back(barrier);
            buffers.state = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        }
        commandList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());

        self->requests_.clear();
    }

    // ------------------------------------------------------------------
    // 初期化
    // ------------------------------------------------------------------
    void SkinningManager::CreateRootSignature() {
        D3D12_ROOT_PARAMETER rootParameters[4] = {};
        // 0: 行列パレット (t0)
        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters[0].Descriptor.ShaderRegister = 0;
        // 1: 入力の頂点 (t1)
        rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters[1].Descriptor.ShaderRegister = 1;
        // 2: 出力の頂点 (u0)
        rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
        rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters[2].Descriptor.ShaderRegister = 0;
        // 3: 頂点の数 (b0)
        rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters[3].Constants.ShaderRegister = 0;
        rootParameters[3].Constants.Num32BitValues = 1;

        D3D12_ROOT_SIGNATURE_DESC desc{};
        desc.pParameters = rootParameters;
        desc.NumParameters = _countof(rootParameters);

        Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
        HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
        if (FAILED(hr)) {
            Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
            assert(false);
        }
        hr = DirectXCommon::GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
            IID_PPV_ARGS(&rootSignature_));
        assert(SUCCEEDED(hr));
    }

    void SkinningManager::CreatePipelineState() {
        Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob =
            ShaderCompiler::Compile(L"Resources/EngineResources/HLSL/Model/Skinning.CS.hlsl", L"cs_6_0");
        assert(shaderBlob);

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};
        desc.pRootSignature = rootSignature_.Get();
        desc.CS = { shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize() };
        HRESULT hr = DirectXCommon::GetDevice()->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pipelineState_));
        assert(SUCCEEDED(hr));
    }
}

#include "GPUParticleEmitter.h"
#include "../Base/DirectXCommon.h"
#include "../Base/Logger.h"
#include "../Graphics/TextureManager.h"
#include "../Camera/Camera.h"
#include <cassert>

namespace RyoEngine {

    namespace {
        // DEFAULT heap上に、UAVとして読み書き可能なバッファリソースを作る共通処理
        // (CBV用のDirectXCommon::CreateBufferResource()はUpload heap前提のため、これとは別に用意する)
        Microsoft::WRL::ComPtr<ID3D12Resource> CreateUAVBufferResource(ID3D12Device* device, size_t sizeInBytes) {
            D3D12_RESOURCE_DESC resourceDesc{};
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            resourceDesc.Width = sizeInBytes;
            resourceDesc.Height = 1;
            resourceDesc.DepthOrArraySize = 1;
            resourceDesc.MipLevels = 1;
            resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
            resourceDesc.SampleDesc.Count = 1;
            resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

            D3D12_HEAP_PROPERTIES heapProperties{};
            heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

            Microsoft::WRL::ComPtr<ID3D12Resource> resource;
            HRESULT hr = device->CreateCommittedResource(
                &heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&resource)
            );
            assert(SUCCEEDED(hr));
            return resource;
        }
    }

    void GPUParticleEmitter::Initialize(const std::string& meshFilePath, uint32_t maxParticleCount,
        bool billboard, GPUParticleCommon::BlendMode blendMode) {
        mesh_ = InstancedMesh::GetOrCreate(meshFilePath);
        maxParticleCount_ = maxParticleCount;
        billboard_ = billboard;
        blendMode_ = blendMode;

        auto device = DirectXCommon::GetInstance()->GetDevice();

        // パーティクル本体 (DEFAULT heap、UAV)
        // NOTE: D3D12のCommitted Resourceは生成時に0クリアされることが仕様で保証されているため、
        //       全スロットremainingLife=0(=死んでいる)の状態から始まる。明示的な初期化パスは設けていない。
        particleResource_ = CreateUAVBufferResource(device, sizeof(GPUParticleData) * maxParticleCount_);
        particleState_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

        // 発生依頼消化用カウンター (DEFAULT heap、UAV。中身はuint1個)
        claimCounterResource_ = CreateUAVBufferResource(device, sizeof(uint32_t));

        // 発生依頼バッファ (Upload heap、CPUがマップして毎フレーム書き込む)
        spawnRequestResource_ = DirectXCommon::CreateBufferResource(device, sizeof(GPUParticleData) * kMaxSpawnRequestsPerFrame);
        spawnRequestResource_->Map(0, nullptr, reinterpret_cast<void**>(&spawnRequestData_));

        // シミュレーションパラメータ用バッファ
        simParamsResource_ = DirectXCommon::CreateBufferResource(device, sizeof(GPUParticleSimParams));
        simParamsResource_->Map(0, nullptr, reinterpret_cast<void**>(&simParamsData_));

        // カメラ用バッファ
        cameraResource_ = DirectXCommon::CreateBufferResource(device, sizeof(GPUParticleCameraData));
        cameraResource_->Map(0, nullptr, reinterpret_cast<void**>(&cameraData_));
        cameraData_->viewProjection = MakeIdentity4x4();
        cameraData_->cameraRight = { 1.0f, 0.0f, 0.0f };
        cameraData_->cameraUp = { 0.0f, 1.0f, 0.0f };
    }

    void GPUParticleEmitter::Finalize() {
        cameraResource_.Reset();
        cameraData_ = nullptr;
        simParamsResource_.Reset();
        simParamsData_ = nullptr;
        spawnRequestResource_.Reset();
        spawnRequestData_ = nullptr;
        claimCounterResource_.Reset();
        particleResource_.Reset();
        pendingSpawnRequests_.clear();
        mesh_.reset();
    }

    void GPUParticleEmitter::Emit(const Vector3& position, const Vector3& velocity, const Vector4& color,
        float scale, const Vector3& rotation, float lifeTime) {
        if (pendingSpawnRequests_.size() >= kMaxSpawnRequestsPerFrame) {
            Logger::Log("[GPUParticleEmitter] Emit: 1フレームあたりの発生依頼上限に達しました\n");
            return;
        }

        GPUParticleData data{};
        data.position = position;
        data.scale = scale;
        data.velocity = velocity;
        data.totalLife = lifeTime;
        data.color = color;
        data.rotation = rotation;
        data.remainingLife = lifeTime;

        pendingSpawnRequests_.push_back(data);
    }

    void GPUParticleEmitter::Update(float deltaTime) {
        uint32_t requestCount = static_cast<uint32_t>(pendingSpawnRequests_.size());
        if (requestCount > kMaxSpawnRequestsPerFrame) {
            requestCount = kMaxSpawnRequestsPerFrame;
        }
        for (uint32_t i = 0; i < requestCount; ++i) {
            spawnRequestData_[i] = pendingSpawnRequests_[i];
        }

        simParamsData_->deltaTime = deltaTime;
        simParamsData_->spawnRequestCount = requestCount;
        simParamsData_->maxParticleCount = maxParticleCount_;
        simParamsData_->gravity = gravity_;

        // 依頼はもうGPU用バッファへ書き込み済みなので、CPU側のリストはここでクリアしてよい
        pendingSpawnRequests_.clear();

        GPUParticleCommon::GetInstance()->SetDispatchCommands([this]() {
            InternalDispatch();
        });
    }

    void GPUParticleEmitter::Draw(const Camera& camera) {
        if (!mesh_) {
            return;
        }

        // カメラの情報は今すぐ確定させ、マップ済みバッファに書き込んでおく
        // (実際の描画コマンド発行は後回しになるため、camera参照そのものをラムダに持ち越さない)
        cameraData_->viewProjection = camera.GetViewProjectionMatrix();
        cameraData_->cameraRight = camera.GetRight();
        cameraData_->cameraUp = camera.GetUp();

        GPUParticleCommon::GetInstance()->SetDrawCommands([this]() {
            InternalDraw();
        });
    }

    void GPUParticleEmitter::TransitionParticleBuffer(D3D12_RESOURCE_STATES newState) {
        if (particleState_ == newState) {
            return;
        }
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = particleResource_.Get();
        barrier.Transition.StateBefore = particleState_;
        barrier.Transition.StateAfter = newState;
        DirectXCommon::GetInstance()->GetCommandList()->ResourceBarrier(1, &barrier);
        particleState_ = newState;
    }

    void GPUParticleEmitter::InternalDispatch() {
        auto commandList = DirectXCommon::GetInstance()->GetCommandList();
        auto common = GPUParticleCommon::GetInstance();

        // コンピュートシェーダーが読み書きできる状態にしておく
        TransitionParticleBuffer(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        commandList->SetComputeRootSignature(common->GetComputeRootSignature());
        commandList->SetComputeRootUnorderedAccessView(0, particleResource_->GetGPUVirtualAddress());
        commandList->SetComputeRootUnorderedAccessView(1, claimCounterResource_->GetGPUVirtualAddress());
        commandList->SetComputeRootShaderResourceView(2, spawnRequestResource_->GetGPUVirtualAddress());
        commandList->SetComputeRootConstantBufferView(3, simParamsResource_->GetGPUVirtualAddress());

        // ① カウンターを0に戻す(1スレッドだけ)
        commandList->SetPipelineState(common->GetResetPipelineState());
        commandList->Dispatch(1, 1, 1);

        // リセットの書き込みが完全に終わってから本体パスが読めるように、UAVバリアを挟む
        D3D12_RESOURCE_BARRIER uavBarrier{};
        uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        uavBarrier.UAV.pResource = claimCounterResource_.Get();
        commandList->ResourceBarrier(1, &uavBarrier);

        // ② 本体：物理演算＋新規発生の消化
        commandList->SetPipelineState(common->GetSimulatePipelineState());
        uint32_t groupCount = (maxParticleCount_ + 255) / 256; // シェーダー側は[numthreads(256,1,1)]
        commandList->Dispatch(groupCount, 1, 1);
    }

    void GPUParticleEmitter::InternalDraw() {
        auto commandList = DirectXCommon::GetInstance()->GetCommandList();
        auto common = GPUParticleCommon::GetInstance();

        // 頂点シェーダー(Vertexステージ)がStructuredBufferとして読めるように、
        // UAV → NON_PIXEL_SHADER_RESOURCE(Pixel以外のステージ用SRV状態)へ遷移させる
        TransitionParticleBuffer(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        commandList->SetGraphicsRootSignature(common->GetRenderRootSignature());
        commandList->SetPipelineState(common->GetRenderPipelineState(billboard_, blendMode_));

        ID3D12DescriptorHeap* ppHeaps[] = { TextureManager::GetInstance()->GetDescriptorHeap() };
        commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        D3D12_VERTEX_BUFFER_VIEW vbv = mesh_->GetVertexBufferView();
        commandList->IASetVertexBuffers(0, 1, &vbv);

        commandList->SetGraphicsRootShaderResourceView(0, particleResource_->GetGPUVirtualAddress());
        commandList->SetGraphicsRootConstantBufferView(1, cameraResource_->GetGPUVirtualAddress());
        commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetGPUHandle(mesh_->GetTextureHandle()));

        // maxParticleCount_ぶん常に全部描く(簡易版)。死んでいるスロットは寿命フェードで
        // アルファ0になるため、見た目上は問題にならない。
        commandList->DrawInstanced(mesh_->GetVertexCount(), maxParticleCount_, 0, 0);
    }
}

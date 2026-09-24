#include "InstancedModel.h"
#include "../Base/DirectXCommon.h"
#include "../Base/Logger.h"
#include "../Graphics/TextureManager.h"
#include "../Camera/Camera.h"
#include "../Light/LightManager.h"
#include "InstancedModelCommon.h"
#include <cassert>

namespace RyoEngine {

    void InstancedModel::Initialize(const std::string& filePath, uint32_t maxInstanceCount) {
        mesh_ = InstancedMesh::GetOrCreate(filePath);
        maxInstanceCount_ = maxInstanceCount;

        auto device = DirectXCommon::GetInstance()->GetDevice();

        // インスタンス配列用バッファ (maxInstanceCount_件ぶん固定確保)
        instanceResource_ = DirectXCommon::CreateBufferResource(device, sizeof(InstanceData) * maxInstanceCount_);
        instanceResource_->Map(0, nullptr, reinterpret_cast<void**>(&instanceData_));

        // カメラView-Projection用バッファ
        cameraVPResource_ = DirectXCommon::CreateBufferResource(device, sizeof(Matrix4x4));
        cameraVPResource_->Map(0, nullptr, reinterpret_cast<void**>(&cameraVPData_));
        *cameraVPData_ = MakeIdentity4x4();

        // 共有マテリアル設定用バッファ
        materialResource_ = DirectXCommon::CreateBufferResource(device, sizeof(InstancedMaterialData));
        materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
        materialData_->enableLighting = enableLighting_ ? 1 : 0;
        materialData_->shadingMode = static_cast<int32_t>(shadingMode_);
    }

    void InstancedModel::Finalize() {
        materialResource_.Reset();
        materialData_ = nullptr;
        cameraVPResource_.Reset();
        cameraVPData_ = nullptr;
        instanceResource_.Reset();
        instanceData_ = nullptr;
        instances_.clear();
        instanceHandles_.clear();
        handleToIndex_.clear();
        mesh_.reset();
    }

    InstancedModel::Handle InstancedModel::AddInstance(const Vector3& translate, const Vector3& rotate, const Vector3& scale, const Vector4& color) {
        if (instances_.size() >= maxInstanceCount_) {
            Logger::Log("[InstancedModel] Cannot add instance, maxInstanceCount reached.\n");
            return kInvalidHandle;
        }
        Instance instance;
        instance.translate = translate;
        instance.rotate = rotate;
        instance.scale = scale;
        instance.color = color;

        Handle handle = nextHandle_++;
        instances_.push_back(instance);
        instanceHandles_.push_back(handle);
        handleToIndex_[handle] = static_cast<int>(instances_.size() - 1);
        return handle;
    }

    void InstancedModel::RemoveInstance(Handle handle) {
        auto it = handleToIndex_.find(handle);
        if (it == handleToIndex_.end()) {
            Logger::Log("[InstancedModel] RemoveInstance: invalid handle.\n");
            return;
        }

        int index = it->second;
        int lastIndex = static_cast<int>(instances_.size()) - 1;

        if (index != lastIndex) {
            // 末尾の要素を削除位置へ持ってくる(swap-and-pop)。
            // 順序は保証されなくなるが、他のインスタンスのHandleは一切変わらないままO(1)で消せる。
            instances_[index] = instances_[lastIndex];
            instanceHandles_[index] = instanceHandles_[lastIndex];
            handleToIndex_[instanceHandles_[index]] = index; // 移動した要素のHandleが指す先を更新
        }
        instances_.pop_back();
        instanceHandles_.pop_back();
        handleToIndex_.erase(it);
    }

    void InstancedModel::ClearInstances() {
        instances_.clear();
        instanceHandles_.clear();
        handleToIndex_.clear();
    }

    void InstancedModel::SetInstanceTransform(Handle handle, const Vector3& translate, const Vector3& rotate, const Vector3& scale) {
        auto it = handleToIndex_.find(handle);
        if (it == handleToIndex_.end()) {
            return;
        }
        Instance& instance = instances_[it->second];
        instance.translate = translate;
        instance.rotate = rotate;
        instance.scale = scale;
    }

    void InstancedModel::SetInstanceTransform(Handle handle, const Transform& transform) {
        auto it = handleToIndex_.find(handle);
        if (it == handleToIndex_.end()) {
            return;
        }
        Instance& instance = instances_[it->second];
        instance.translate = transform.translate;
        instance.rotate = transform.rotate;
        instance.scale = transform.scale;
    }

    void InstancedModel::SetInstanceColor(Handle handle, const Vector4& color) {
        auto it = handleToIndex_.find(handle);
        if (it == handleToIndex_.end()) {
            return;
        }
        instances_[it->second].color = color;
    }

    void InstancedModel::UpdateBuffer() {
        materialData_->enableLighting = enableLighting_ ? 1 : 0;
        materialData_->shadingMode = static_cast<int32_t>(shadingMode_);

        uint32_t count = static_cast<uint32_t>(instances_.size());
        if (count > maxInstanceCount_) {
            Logger::Log("[InstancedModel] Instance count exceeds maxInstanceCount, truncating.\n");
            count = maxInstanceCount_;
        }
        for (uint32_t i = 0; i < count; ++i) {
            const Instance& instance = instances_[i];
            instanceData_[i].world = MakeAffineMatrix(instance.scale, instance.rotate, instance.translate);
            instanceData_[i].color = instance.color;
        }
    }

    void InstancedModel::Draw(const Camera& camera) {
        if (!mesh_ || instances_.empty()) {
            return;
        }

        // カメラの行列は今すぐ確定させ、マップ済みバッファに書き込んでおく。
        // (実際の描画コマンド発行はInstancedModelCommonによって後回しにされるため、
        //  camera参照そのものをラムダに持ち越さないようにする)
        *cameraVPData_ = camera.GetViewProjectionMatrix();

        InstancedModelCommon::GetInstance()->SetDrawCommands([this]() {
            InternalDraw();
            });
    }

    void InstancedModel::InternalDraw() {
        auto commandList = DirectXCommon::GetInstance()->GetCommandList();
        auto common = InstancedModelCommon::GetInstance();
        auto lightManager = LightManager::GetInstance();

        commandList->SetGraphicsRootSignature(common->GetRootSignature());
        commandList->SetPipelineState(common->GetPipelineState());

        ID3D12DescriptorHeap* ppHeaps[] = { TextureManager::GetInstance()->GetDescriptorHeap() };
        commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        D3D12_VERTEX_BUFFER_VIEW vbv = mesh_->GetVertexBufferView();
        commandList->IASetVertexBuffers(0, 1, &vbv);

        commandList->SetGraphicsRootShaderResourceView(0, instanceResource_->GetGPUVirtualAddress());
        commandList->SetGraphicsRootConstantBufferView(1, cameraVPResource_->GetGPUVirtualAddress());
        commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetGPUHandle(mesh_->GetTextureHandle()));
        commandList->SetGraphicsRootConstantBufferView(3, materialResource_->GetGPUVirtualAddress());
        commandList->SetGraphicsRootShaderResourceView(4, lightManager->GetLightGPUVirtualAddress());
        commandList->SetGraphicsRootConstantBufferView(5, lightManager->GetLightCountGPUVirtualAddress());
        commandList->SetGraphicsRootConstantBufferView(6, lightManager->GetAmbientGPUVirtualAddress());

        // ここが核心：インスタンス数ぶんのDrawコールを個別に発行するのではなく、
        // 「頂点数」と「インスタンス数」を渡して1回だけ呼ぶ。GPU側がinstances_.size()回分、
        // 頂点シェーダーをSV_InstanceIDを変えながら自動的に実行してくれる。
        commandList->DrawInstanced(mesh_->GetVertexCount(), static_cast<UINT>(instances_.size()), 0, 0);
    }
}
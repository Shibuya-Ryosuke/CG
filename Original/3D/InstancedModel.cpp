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
        mesh_.reset();
    }

    int InstancedModel::AddInstance(const Vector3& translate, const Vector3& rotate, const Vector3& scale, const Vector4& color) {
        if (instances_.size() >= maxInstanceCount_) {
            Logger::LogSuccess("[InstancedModel] Cannot add instance, maxInstanceCount reached.\n");
            return -1;
        }
        Instance instance;
        instance.translate = translate;
        instance.rotate = rotate;
        instance.scale = scale;
        instance.color = color;
        instances_.push_back(instance);
        return static_cast<int>(instances_.size() - 1);
    }

    void InstancedModel::RemoveInstance(int index) {
        if (index < 0 || index >= static_cast<int>(instances_.size())) {
            Logger::LogError("[InstancedModel] RemoveInstance index out of range.\n");
            return;
        }
        instances_.erase(instances_.begin() + index);
    }

    void InstancedModel::ClearInstances() {
        instances_.clear();
    }

    void InstancedModel::SetInstanceTransform(int index, const Vector3& translate, const Vector3& rotate, const Vector3& scale) {
        if (index < 0 || index >= static_cast<int>(instances_.size())) {
            return;
        }
        instances_[index].translate = translate;
        instances_[index].rotate = rotate;
        instances_[index].scale = scale;
    }

    void InstancedModel::SetInstanceColor(int index, const Vector4& color) {
        if (index < 0 || index >= static_cast<int>(instances_.size())) {
            return;
        }
        instances_[index].color = color;
    }

    void InstancedModel::Update() {
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
#include "Model.h"
#include "../Base/DirectXCommon.h"
#include "../Graphics/TextureManager.h"
#include "../Reflect/ReflectCommon.h"
#include "../Reflect/ReflectModel.h"
#include "../Edit/AnimEdit.h"

namespace RyoEngine {

    void Model::Initialize() {
        // デフォルト設定などが必要ならここに書く
        // scale rotate translate
        transform_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
    }

    void Model::CreateModel(const std::string& filePath) {
        // モデルロード (複数メッシュ・複数マテリアルに対応)
        ModelLoader::ModelData modelData = ModelLoader::LoadObjFile(filePath);

        // リソース作成
        InternalInitialize(modelData);
        transform_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
    }

    void Model::CreateDirectionalLight() {
        auto device = DirectXCommon::GetInstance()->GetDevice();

        // DirectionalLightリソース作成
        lightResource_ = DirectXCommon::CreateBufferResource(device, sizeof(DirectionalLight));
        lightResource_->Map(0, nullptr, reinterpret_cast<void**>(&lightData_));

        // デフォルト値（白い光が斜め下に向いている状態）
        lightData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
        lightData_->direction = { 0.0f, -1.0f, 0.0f };
        lightData_->intensity = 1.0f;

        // 全メッシュのデフォルトシェーディングモードをHALF_LAMBERTにする
        for (auto& mesh : meshes_) {
            mesh.materialData->shadingMode = ShadingMode::HALF_LAMBERT;
        }
    }

    void Model::SetTex(uint32_t handle, int32_t meshIndex) {
        if (meshIndex < 0) {
            for (auto& mesh : meshes_) {
                mesh.textureHandle = handle;
            }
        } else {
            meshes_[meshIndex].textureHandle = handle;
        }
    }

    void Model::SetTex(const std::string& filePath, int32_t meshIndex) {
        uint32_t handle = TextureManager::GetInstance()->Load(filePath);
        SetTex(handle, meshIndex);
    }

    void Model::SetLambert(const ShadingMode lambertMode, int32_t meshIndex) {
        if (meshIndex < 0) {
            for (auto& mesh : meshes_) {
                mesh.materialData->shadingMode = lambertMode;
            }
        } else {
            meshes_[meshIndex].materialData->shadingMode = lambertMode;
        }
    }

    void Model::SetColor(const Vector4& color, int32_t meshIndex) {
        if (meshIndex < 0) {
            for (auto& mesh : meshes_) {
                mesh.materialData->color = color;
            }
        } else {
            meshes_[meshIndex].materialData->color = color;
        }
    }

    void Model::InternalInitialize(const ModelLoader::ModelData& modelData) {
        auto device = DirectXCommon::GetInstance()->GetDevice();

        meshes_.clear();
        meshes_.reserve(modelData.meshes.size());

        for (const auto& srcMesh : modelData.meshes) {
            MeshResource mesh;

            // 1. 頂点バッファ作成
            mesh.vertexResource = DirectXCommon::CreateBufferResource(device, sizeof(VertexData) * srcMesh.vertices.size());
            mesh.vertexBufferView.BufferLocation = mesh.vertexResource->GetGPUVirtualAddress();
            mesh.vertexBufferView.SizeInBytes = static_cast<UINT>(sizeof(VertexData) * srcMesh.vertices.size());
            mesh.vertexBufferView.StrideInBytes = sizeof(VertexData);

            VertexData* vertexData = nullptr;
            mesh.vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
            std::memcpy(vertexData, srcMesh.vertices.data(), sizeof(VertexData) * srcMesh.vertices.size());
            mesh.vertexCount = static_cast<uint32_t>(srcMesh.vertices.size());

            // 2. マテリアルバッファ作成 (メッシュごとに1つ)
            mesh.materialResource = DirectXCommon::CreateBufferResource(device, sizeof(Material));
            mesh.materialResource->Map(0, nullptr, reinterpret_cast<void**>(&mesh.materialData));
            mesh.materialData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
            mesh.materialData->enableLighting = 1;
            mesh.materialData->shadingMode = ShadingMode::LAMBERT;
            mesh.materialData->uvTransform = MakeIdentity4x4();

            // 3. テクスチャ (メッシュが参照するマテリアルのmap_Kdから読み込む。無ければ白テクスチャ)
            if (srcMesh.materialIndex < modelData.materials.size() &&
                !modelData.materials[srcMesh.materialIndex].textureFilePath.empty()) {
                mesh.textureHandle = TextureManager::GetInstance()->Load(modelData.materials[srcMesh.materialIndex].textureFilePath);
            } else {
                mesh.textureHandle = TextureManager::GetInstance()->GetWhiteTex();
            }

            meshes_.push_back(mesh);
        }

        // 4. WVPバッファ作成 (モデル全体で共有するので1つでよい)
        wvpResource_ = DirectXCommon::CreateBufferResource(device, sizeof(TransformationMatrix));
        wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));
        wvpData_->WVP = MakeIdentity4x4();
        wvpData_->World = MakeIdentity4x4();

        CreateDirectionalLight();
    }

    void Model::Update(const Camera& camera) {
        // ワールド行列の作成
        worldMatrix_ = MakeAffineMatrix(transform_.scale,transform_.rotate,transform_.translate);
        
        // WVP行列の計算 (World * ViewProjection)
        Matrix4x4 wvpMatrix = worldMatrix_ * camera.GetViewProjectionMatrix();

        wvpData_->World = worldMatrix_;
        wvpData_->WVP = wvpMatrix;
    }

    void Model::Update(const DebugCamera& debugCamera) {
        // ワールド行列の作成
        worldMatrix_ = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);

        // WVP行列の計算 (World * ViewProjection)
        Matrix4x4 wvpMatrix = worldMatrix_ * debugCamera.GetViewProjectionMatrix();

        wvpData_->World = worldMatrix_;
        wvpData_->WVP = wvpMatrix;
    }

    void Model::Draw() {
        auto commandList = DirectXCommon::GetInstance()->GetCommandList();

        // メッシュごとにテクスチャ・マテリアルを切り替えながらドローコールを発行する
        // (WVP・ライトはモデル全体で共有のため、メッシュ間で使い回す)
        for (const auto& mesh : meshes_) {
            commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetGPUHandle(mesh.textureHandle));

            commandList->IASetVertexBuffers(0, 1, &mesh.vertexBufferView);
            commandList->SetGraphicsRootConstantBufferView(0, mesh.materialResource->GetGPUVirtualAddress());
            commandList->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());

            // ライトの定数バッファをセット
            commandList->SetGraphicsRootConstantBufferView(3, lightResource_->GetGPUVirtualAddress());

            commandList->DrawInstanced(mesh.vertexCount, 1, 0, 0);
        }
    }

    Model* Model::Create(const std::string& filePath, bool registAnimEdit, const std::string& name) {
        Model* instance = new Model();
        instance->Initialize(); // 共通の初期化
        instance->CreateModel(filePath); // モデル読み込みとリソース作成 (複数メッシュに対応)

        if (registAnimEdit) {
            AnimEdit::SetTargetModel(instance, name);
        }

        return instance;
    }
}

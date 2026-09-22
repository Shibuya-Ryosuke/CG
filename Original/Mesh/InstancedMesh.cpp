#include "InstancedMesh.h"
#include "../Base/DirectXCommon.h"
#include "../Base/Logger.h"
#include "../Graphics/TextureManager.h"
#include "../Loader/ModelLoader.h"
#include <algorithm>

namespace RyoEngine {

    std::shared_ptr<InstancedMesh> InstancedMesh::GetOrCreate(const std::string& filePath) {
        auto it = cache_.find(filePath);
        if (it != cache_.end()) {
            return it->second;
        }

        // shared_ptrのコンストラクタはprivateなので、ここではnewを直接使う
        std::shared_ptr<InstancedMesh> mesh(new InstancedMesh());
        mesh->LoadFromFile(filePath);
        cache_[filePath] = mesh;
        return mesh;
    }

    void InstancedMesh::LoadFromFile(const std::string& filePath) {
        auto device = DirectXCommon::GetInstance()->GetDevice();

        ModelLoader::ModelData modelData = ModelLoader::LoadObjFile(filePath);

        if (modelData.meshes.empty()) {
            Logger::Log("[InstancedMesh] メッシュが1つも読み込めませんでした: " + filePath + "\n");
            return;
        }
        if (modelData.meshes.size() > 1) {
            Logger::LogWarning("[InstancedMesh] 複数メッシュのOBJです。GPUインスタンシングは単一メッシュのみ対応のため、先頭のメッシュだけを使用します: " + filePath + "\n");
        }

        const ModelLoader::MeshData& srcMesh = modelData.meshes[0];
        vertexCount_ = static_cast<uint32_t>(srcMesh.vertices.size());

        // 頂点バッファ生成 (Modelと同じく、インデックスバッファは使わない三角形展開済みの頂点列)
        vertexResource_ = DirectXCommon::CreateBufferResource(device, sizeof(VertexData) * srcMesh.vertices.size());
        vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
        vertexBufferView_.SizeInBytes = sizeof(VertexData) * static_cast<UINT>(srcMesh.vertices.size());
        vertexBufferView_.StrideInBytes = sizeof(VertexData);

        VertexData* mappedData = nullptr;
        vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));
        std::copy(srcMesh.vertices.begin(), srcMesh.vertices.end(), mappedData);
        vertexResource_->Unmap(0, nullptr);

        // テクスチャ (メッシュが参照するマテリアルのmap_Kdから読み込む。無ければ白テクスチャ)
        if (srcMesh.materialIndex < modelData.materials.size() &&
            !modelData.materials[srcMesh.materialIndex].textureFilePath.empty()) {
            textureHandle_ = TextureManager::GetInstance()->Load(modelData.materials[srcMesh.materialIndex].textureFilePath);
        } else {
            textureHandle_ = TextureManager::GetInstance()->GetWhiteTex();
        }
    }
}
#include "Model.h"
#include "../Base/DirectXCommon.h"
#include "../Graphics/TextureManager.h"
//#include "../Reflect/ReflectCommon.h"
//#include "../Reflect/ReflectModel.h"
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
        // NOTE: 以前はここでモデルごとのDirectionalLight用定数バッファを作成していたが、
        //       ライトはシーンで1つに共有する方針になったため、その生成処理は
        //       LightManager::Initialize() 側に移動した(エンジン起動時に一度だけ呼ばれる想定)。
        //       この関数名は互換性のため残しているが、実質的にやっているのは
        //       「全メッシュのデフォルトシェーディングモードをHALF_LAMBERTにする」ことだけ。

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

    void Model::SetEnableLighting(bool enableLighting, int32_t meshIndex) {
        if (meshIndex < 0) {
            for (auto& mesh : meshes_) {
                mesh.materialData->enableLighting = enableLighting;
            }
        } else {
            meshes_[meshIndex].materialData->enableLighting = enableLighting;
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

    void Model::SetHasUV(bool hasUV, int32_t meshIndex) {
        if (meshIndex < 0) {
            for (auto& mesh : meshes_) {
                mesh.hasUV = hasUV;
            }
        } else {
            meshes_[meshIndex].hasUV = hasUV;
        }
    }

    ShadingMode Model::GetLambertByName(const std::string& materialName) const {
        int32_t index = GetMeshIndexByName(materialName);
        if (index >= 0) {
            return GetLambert(index);
        }
        // 見つからない場合のデフォルト値やエラーハンドリング
        return {}; // または適切なデフォルトの ShadingMode
    }

    Vector4 Model::GetColorByName(const std::string& materialName) const {
        int32_t index = GetMeshIndexByName(materialName);
        if (index >= 0) {
            return GetColor(index);
        }
        return Vector4(0, 0, 0, 0); // デフォルトカラー
    }

    void Model::SetLambertByName(const ShadingMode lambertMode, const std::string& materialName) {
        int32_t index = GetMeshIndexByName(materialName);
        if (index >= 0) {
            SetLambert(lambertMode, index);
        }
    }

    void Model::SetTexByName(uint32_t handle, const std::string& materialName) {
        int32_t index = GetMeshIndexByName(materialName);
        if (index >= 0) {
            SetTex(handle, index);
        }
    }

    void Model::SetTexByName(const std::string& filePath, const std::string& materialName) {
        int32_t index = GetMeshIndexByName(materialName);
        if (index >= 0) {
            SetTex(filePath, index);
        }
    }

    void Model::SetColorByName(const Vector4& color, const std::string& materialName) {
        int32_t index = GetMeshIndexByName(materialName);
        if (index >= 0) {
            SetColor(color, index);
        }
    }

    void Model::UpdateUVTransform(MeshResource& mesh) {
        // UV用のSRT行列を作成してmaterialDataへ書き込む (materialDataはUpload Heapへ常時Mapされているので、
        // ここで代入した時点でGPU側の値もそのまま更新される)
        mesh.materialData->uvTransform = MakeAffineMatrix(
            { mesh.uvScale.x, mesh.uvScale.y, 1.0f },
            { 0.0f, 0.0f, mesh.uvRotate },
            { mesh.uvTranslate.x, mesh.uvTranslate.y, 0.0f }
        );
    }

    void Model::SetUVScale(const Vector2& scale, int32_t meshIndex) {
        if (meshIndex < 0) {
            for (auto& mesh : meshes_) {
                mesh.uvScale = scale;
                UpdateUVTransform(mesh);
            }
        } else {
            meshes_[meshIndex].uvScale = scale;
            UpdateUVTransform(meshes_[meshIndex]);
        }
    }

    void Model::SetUVRotate(float rotate, int32_t meshIndex) {
        if (meshIndex < 0) {
            for (auto& mesh : meshes_) {
                mesh.uvRotate = rotate;
                UpdateUVTransform(mesh);
            }
        } else {
            meshes_[meshIndex].uvRotate = rotate;
            UpdateUVTransform(meshes_[meshIndex]);
        }
    }

    void Model::SetUVTranslate(const Vector2& translate, int32_t meshIndex) {
        if (meshIndex < 0) {
            for (auto& mesh : meshes_) {
                mesh.uvTranslate = translate;
                UpdateUVTransform(mesh);
            }
        } else {
            meshes_[meshIndex].uvTranslate = translate;
            UpdateUVTransform(meshes_[meshIndex]);
        }
    }

    void Model::SetUVSRT(const Vector2& scale, float rotate, const Vector2& translate, int32_t meshIndex) {
        if (meshIndex < 0) {
            for (auto& mesh : meshes_) {
                mesh.uvScale = scale;
                mesh.uvRotate = rotate;
                mesh.uvTranslate = translate;
                UpdateUVTransform(mesh);
            }
        } else {
            meshes_[meshIndex].uvScale = scale;
            meshes_[meshIndex].uvRotate = rotate;
            meshes_[meshIndex].uvTranslate = translate;
            UpdateUVTransform(meshes_[meshIndex]);
        }
    }

    void Model::SetUVScaleByName(const Vector2& scale, const std::string& materialName) {
        int32_t index = GetMeshIndexByName(materialName);
        if (index >= 0) {
            SetUVScale(scale, index);
        }
    }

    void Model::SetUVRotateByName(float rotate, const std::string& materialName) {
        int32_t index = GetMeshIndexByName(materialName);
        if (index >= 0) {
            SetUVRotate(rotate, index);
        }
    }

    void Model::SetUVTranslateByName(const Vector2& translate, const std::string& materialName) {
        int32_t index = GetMeshIndexByName(materialName);
        if (index >= 0) {
            SetUVTranslate(translate, index);
        }
    }

    void Model::SetUVSRTByName(const Vector2& scale, float rotate, const Vector2& translate, const std::string& materialName) {
        int32_t index = GetMeshIndexByName(materialName);
        if (index >= 0) {
            SetUVSRT(scale, rotate, translate, index);
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
            UpdateUVTransform(mesh); // uvScale(1,1)/uvRotate(0)/uvTranslate(0,0)のデフォルト値から単位行列相当が入る

            // 3. テクスチャ (メッシュが参照するマテリアルのmap_Kdから読み込む。無ければ白テクスチャ)
            //    あわせて、あとで名前引きできるようにmtlのマテリアル名も控えておく
            if (srcMesh.materialIndex < modelData.materials.size()) {
                mesh.materialName = modelData.materials[srcMesh.materialIndex].name;
                if (!modelData.materials[srcMesh.materialIndex].textureFilePath.empty()) {
                    mesh.textureHandle = TextureManager::GetInstance()->Load(modelData.materials[srcMesh.materialIndex].textureFilePath);
                } else {
                    mesh.textureHandle = TextureManager::GetInstance()->GetWhiteTex();
                }
            } else {
                mesh.textureHandle = TextureManager::GetInstance()->GetWhiteTex();
            }

            // ModelLoaderが「objの面(f)にvt(UV)が無かったか」を判定した結果をそのまま反映する。
            // (Suzanneのようにvtを持たないobjは、ここで自動的にfalseになる)
            mesh.hasUV = srcMesh.hasUV;

            meshes_.push_back(mesh);
        }

        // 4. WVPバッファ作成 (モデル全体で共有するので1つでよい)
        wvpResource_ = DirectXCommon::CreateBufferResource(device, sizeof(TransformationMatrix));
        wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));
        wvpData_->WVP = MakeIdentity4x4();
        wvpData_->World = MakeIdentity4x4();

        CreateDirectionalLight();
    }

    void Model::InternalDraw(ModelCommon::DrawType drawType, D3D12_GPU_VIRTUAL_ADDRESS externalWVP) {
        auto commandList = DirectXCommon::GetInstance()->GetCommandList();
        auto lightManager = LightManager::GetInstance(); // ループの外で取得
        D3D12_GPU_VIRTUAL_ADDRESS lightGVA = lightManager->GetGPUVirtualAddress();

        // externalWVPが指定されていれば(パーティクル等の外部インスタンスバッファ)そちらを優先し、
        // 指定が無ければ従来通りモデル自身が持つ1個のwvpResource_を使う
        D3D12_GPU_VIRTUAL_ADDRESS wvpGVA = (externalWVP != 0) ? externalWVP : wvpResource_->GetGPUVirtualAddress();

        // 最後にセットしたPSOを保持しておき、変更時のみ切り替える（最適化）
        ID3D12PipelineState* lastPSO = nullptr;

        for (const auto& mesh : meshes_) {
            ModelCommon::DrawType actualDrawType = drawType;
            if (!mesh.hasUV) {
                actualDrawType = (drawType == ModelCommon::DrawType::REFLECT)
                    ? ModelCommon::DrawType::REFLECT_NO_UV
                    : ModelCommon::DrawType::NO_UV;
            }

            ID3D12PipelineState* currentPSO = ModelCommon::GetInstance()->GetPipelineState(actualDrawType);
            if (currentPSO != lastPSO) {
                commandList->SetPipelineState(currentPSO);
                lastPSO = currentPSO;
            }

            // SRV(テクスチャ)は DescriptorTable でセット
            commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetGPUHandle(mesh.textureHandle));

            commandList->IASetVertexBuffers(0, 1, &mesh.vertexBufferView);
            commandList->SetGraphicsRootConstantBufferView(0, mesh.materialResource->GetGPUVirtualAddress());
            commandList->SetGraphicsRootConstantBufferView(1, wvpGVA);
            commandList->SetGraphicsRootConstantBufferView(3, lightGVA);

            commandList->DrawInstanced(mesh.vertexCount, 1, 0, 0);
        }
    }

    void Model::DrawInstance(D3D12_GPU_VIRTUAL_ADDRESS externalWVP, ModelCommon::DrawType drawType) {
        ModelCommon::GetInstance()->SetDrawCommands([this, externalWVP, drawType]() {
            InternalDraw(drawType, externalWVP);
        });
    }

    void Model::Update(const Camera& camera) {
        // ワールド行列の作成
        worldMatrix_ = MakeAffineMatrix(transform_.scale,transform_.rotate,transform_.translate);
        
        // WVP行列の計算 (World * ViewProjection)
        Matrix4x4 wvpMatrix = worldMatrix_ * camera.GetViewProjectionMatrix();

        wvpData_->World = worldMatrix_;
        wvpData_->WVP = wvpMatrix;
    }

    void Model::Draw(ModelCommon::DrawType drawType) {
        ModelCommon::GetInstance()->SetDrawCommands([ =, this]() {
            InternalDraw(drawType);
        });
    }

    std::unique_ptr<Model> Model::Create(const std::string& filePath, bool registAnimEdit, const std::string& name) {
        std::unique_ptr<Model> instance = std::make_unique<Model>();
        instance->Initialize(); // 共通の初期化
        instance->CreateModel(filePath); // モデル読み込みとリソース作成 (複数メッシュに対応)

        if (registAnimEdit) {
            AnimEdit::SetTargetModel(instance.get(), name);
        }

        return instance;
    }
}

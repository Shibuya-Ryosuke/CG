#include "Model.h"
#include "../2D/TextureManager.h"
#include "../../Core/Math/Collision.h"
#include "../../Editor/ModelPicker.h"
#include <cassert>

namespace RyoEngine {

    namespace {
        constexpr float kDegToRad = 3.14159265358979323846f / 180.0f;

        Vector3 DegToRad(const Vector3& deg) {
            return { deg.x * kDegToRad, deg.y * kDegToRad, deg.z * kDegToRad };
        }
    }

    Model::~Model() {
        ModelPicker::OnModelDestroyed(this);
        if (layoutRegistered_) {
            ModelLayout::Unregister(this, layoutSlot_);
        }
    }

    void Model::SetName(const std::string& name) {
        if (!layoutRegistered_) return;
        layoutSlot_ = ModelLayout::Rename(this, layoutSlot_, name);
        layoutApplied_ = false; // 新しい名前で保存値を引き直す
    }

    void Model::ApplyEditedTransform(const Transform& transform) {
        layoutEdited_ = true;
        transform_ = transform;
        if (onTransformEdited_) {
            onTransformEdited_(transform);
        }
    }

    // ------------------------------------------------------------------
    // 作成
    // ------------------------------------------------------------------
    std::unique_ptr<Model> Model::Create(uint32_t modelIndex, uint32_t textureIndex) {
        std::unique_ptr<Model> instance = std::make_unique<Model>();
        instance->CreateModel(modelIndex, textureIndex);
        return instance;
    }

    std::unique_ptr<Model> Model::Create(const std::string& filePath, uint32_t textureIndex) {
        std::unique_ptr<Model> instance = std::make_unique<Model>();
        instance->CreateModel(filePath, textureIndex);
        return instance;
    }

    void Model::CreateModel(const std::string& filePath, uint32_t textureIndex) {
        CreateModel(ModelManager::GetInstance()->LoadObj(filePath), textureIndex);
    }

    void Model::CreateModel(uint32_t modelIndex, uint32_t textureIndex) {
        // 作り直しのときは、古い登録を外す
        if (layoutRegistered_) {
            ModelLayout::Unregister(this, layoutSlot_);
            layoutRegistered_ = false;
        }
        layoutApplied_ = false;
        layoutEdited_ = false;

        modelIndex_ = modelIndex;
        meshes_.clear();
        transform_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
        worldMatrix_ = MakeIdentity4x4();
        isVisible_ = true;

        const ModelManager::ModelAsset* asset = ModelManager::GetInstance()->GetAsset(modelIndex);
        if (!asset) {
            return; // 無効なindex。描画されないだけで落とさない
        }

        layoutSlot_ = ModelLayout::Register(this, asset->filePath);
        layoutRegistered_ = true;

        meshes_.resize(asset->meshes.size());
        for (size_t i = 0; i < meshes_.size(); ++i) {
            meshes_[i].textureHandle = asset->meshes[i].defaultTextureHandle;
        }
        if (textureIndex != 0) {
            SetTex(textureIndex);
        }
    }

    // ------------------------------------------------------------------
    // 毎フレーム
    // ------------------------------------------------------------------
    void Model::TransferMatrix(const Camera& camera, bool frustumCulling) {
        // 保存された Transform があれば、最初の1回だけ適用する (コードで入れた初期値より保存値が勝つ)
        if (!layoutApplied_ && layoutRegistered_) {
            layoutApplied_ = true;
            Transform saved{};
            if (ModelLayout::TryGetSaved(layoutSlot_.key, saved)) {
                transform_ = saved;
            }
        }

        worldMatrix_ = MakeAffineMatrix(transform_.scale, DegToRad(transform_.rotate), transform_.translate);

        // 3D描画全体で使うVPを渡す (最後に呼ばれたカメラが有効)
        ModelManager::GetInstance()->SetViewProjection(camera.GetViewProjectionMatrix());
        ModelPicker::SetCamera(camera);

        const ModelManager::ModelAsset* asset = ModelManager::GetInstance()->GetAsset(modelIndex_);
        if (!asset) {
            isVisible_ = false;
            return;
        }

        // フラスタムカリング。非一様スケールでも安全なよう一番大きい成分で球を拡大する
        float maxScale = transform_.scale.x;
        if (transform_.scale.y > maxScale) maxScale = transform_.scale.y;
        if (transform_.scale.z > maxScale) maxScale = transform_.scale.z;

        Sphere boundingSphere{};
        boundingSphere.center = { worldMatrix_.m[3][0], worldMatrix_.m[3][1], worldMatrix_.m[3][2] };
        boundingSphere.radius = asset->boundingRadius * maxScale;
        // frustumCulling=false の場合は常に描画する (球の計算はクリック選択のため常に行う)
        isVisible_ = frustumCulling ? IsCollision(boundingSphere, camera.GetFrustumPlanes()) : true;

        pickCenter_ = boundingSphere.center;
        pickRadius_ = boundingSphere.radius;
    }

    void Model::Draw() {
        if (!isVisible_) {
            return;
        }
        auto manager = ModelManager::GetInstance();
        const ModelManager::ModelAsset* asset = manager->GetAsset(modelIndex_);
        if (!asset) {
            return;
        }

        if (pickable_) {
            ModelPicker::Submit(this, pickCenter_, pickRadius_);
        }

        for (size_t i = 0; i < meshes_.size() && i < asset->meshes.size(); ++i) {
            const MeshState& state = meshes_[i];
            const ModelManager::MeshAsset& meshAsset = asset->meshes[i];

            ModelInstanceData data;
            data.world = worldMatrix_;
            data.color = state.color;
            data.emissive = { state.emissiveColor.x, state.emissiveColor.y, state.emissiveColor.z, state.emissiveIntensity };

            // UV変換 (2x2 + 平行移動だけGPUへ渡す)
            const Matrix4x4 uv = MakeAffineMatrix(
                { state.uvScale.x, state.uvScale.y, 1.0f },
                { 0.0f, 0.0f, state.uvRotate * kDegToRad },
                { state.uvTranslate.x, state.uvTranslate.y, 0.0f });
            data.uvMat = { uv.m[0][0], uv.m[0][1], uv.m[1][0], uv.m[1][1] };
            data.uvOffset = { uv.m[3][0], uv.m[3][1] };

            data.enableLighting = state.enableLighting ? 1 : 0;
            data.shadingMode = static_cast<int32_t>(state.shadingMode);

            // UVを持たないメッシュはテクスチャを引けないので、上書きされていても既定(white)を使う
            const uint32_t textureHandle = meshAsset.hasUV ? state.textureHandle : meshAsset.defaultTextureHandle;

            manager->Submit(modelIndex_, static_cast<uint32_t>(i), textureHandle, data);
        }
    }

    // ------------------------------------------------------------------
    // Getter
    // ------------------------------------------------------------------
    bool Model::GetHasUV(size_t meshIndex) const {
        const auto* asset = ModelManager::GetInstance()->GetAsset(modelIndex_);
        return asset && meshIndex < asset->meshes.size() ? asset->meshes[meshIndex].hasUV : true;
    }

    const std::string& Model::GetMaterialName(size_t meshIndex) const {
        static const std::string empty;
        const auto* asset = ModelManager::GetInstance()->GetAsset(modelIndex_);
        return asset && meshIndex < asset->meshes.size() ? asset->meshes[meshIndex].materialName : empty;
    }

    int32_t Model::GetMeshIndexByName(const std::string& materialName) const {
        const auto* asset = ModelManager::GetInstance()->GetAsset(modelIndex_);
        if (!asset) return -1;
        for (size_t i = 0; i < asset->meshes.size(); ++i) {
            if (asset->meshes[i].materialName == materialName) {
                return static_cast<int32_t>(i);
            }
        }
        return -1;
    }

    // ------------------------------------------------------------------
    // Setter
    // ------------------------------------------------------------------
    void Model::SetTex(uint32_t handle, int32_t meshIndex) {
        const auto* asset = ModelManager::GetInstance()->GetAsset(modelIndex_);
        if (!asset) return;
        ForEachMesh(meshIndex, [&](MeshState& mesh) {
            const size_t index = static_cast<size_t>(&mesh - meshes_.data());
            if (handle == 0) {
                mesh.textureHandle = asset->meshes[index].defaultTextureHandle; // 初期値へ戻す
                mesh.textureOverridden = false;
            } else {
                mesh.textureHandle = handle;
                mesh.textureOverridden = true;
            }
            });
    }

    void Model::SetTex(const std::string& filePath, int32_t meshIndex) {
        SetTex(TextureManager::GetInstance()->Load(filePath), meshIndex);
    }

    void Model::SetColor(const Vector4& color, int32_t meshIndex) {
        ForEachMesh(meshIndex, [&](MeshState& mesh) { mesh.color = color; });
    }

    void Model::SetEmissive(const Vector3& color, float intensity, int32_t meshIndex) {
        ForEachMesh(meshIndex, [&](MeshState& mesh) {
            mesh.emissiveColor = color;
            mesh.emissiveIntensity = intensity;
            });
    }

    void Model::SetEnableLighting(bool enable, int32_t meshIndex) {
        ForEachMesh(meshIndex, [&](MeshState& mesh) { mesh.enableLighting = enable; });
    }

    void Model::SetLambert(ShadingMode mode, int32_t meshIndex) {
        ForEachMesh(meshIndex, [&](MeshState& mesh) { mesh.shadingMode = mode; });
    }

    void Model::SetUVScale(const Vector2& scale, int32_t meshIndex) {
        ForEachMesh(meshIndex, [&](MeshState& mesh) { mesh.uvScale = scale; });
    }

    void Model::SetUVRotate(float rotateDeg, int32_t meshIndex) {
        ForEachMesh(meshIndex, [&](MeshState& mesh) { mesh.uvRotate = rotateDeg; });
    }

    void Model::SetUVTranslate(const Vector2& translate, int32_t meshIndex) {
        ForEachMesh(meshIndex, [&](MeshState& mesh) { mesh.uvTranslate = translate; });
    }

    void Model::SetUVSRT(const Vector2& scale, float rotateDeg, const Vector2& translate, int32_t meshIndex) {
        ForEachMesh(meshIndex, [&](MeshState& mesh) {
            mesh.uvScale = scale;
            mesh.uvRotate = rotateDeg;
            mesh.uvTranslate = translate;
            });
    }

    // ------------------------------------------------------------------
    // 名前指定版
    // ------------------------------------------------------------------
    void Model::SetTexByName(uint32_t handle, const std::string& materialName) {
        const int32_t index = GetMeshIndexByName(materialName);
        if (index >= 0) SetTex(handle, index);
    }
    void Model::SetTexByName(const std::string& filePath, const std::string& materialName) {
        const int32_t index = GetMeshIndexByName(materialName);
        if (index >= 0) SetTex(filePath, index);
    }
    void Model::SetColorByName(const Vector4& color, const std::string& materialName) {
        const int32_t index = GetMeshIndexByName(materialName);
        if (index >= 0) SetColor(color, index);
    }
    void Model::SetLambertByName(ShadingMode mode, const std::string& materialName) {
        const int32_t index = GetMeshIndexByName(materialName);
        if (index >= 0) SetLambert(mode, index);
    }
    void Model::SetUVScaleByName(const Vector2& scale, const std::string& materialName) {
        const int32_t index = GetMeshIndexByName(materialName);
        if (index >= 0) SetUVScale(scale, index);
    }
    void Model::SetUVRotateByName(float rotateDeg, const std::string& materialName) {
        const int32_t index = GetMeshIndexByName(materialName);
        if (index >= 0) SetUVRotate(rotateDeg, index);
    }
    void Model::SetUVTranslateByName(const Vector2& translate, const std::string& materialName) {
        const int32_t index = GetMeshIndexByName(materialName);
        if (index >= 0) SetUVTranslate(translate, index);
    }
    void Model::SetUVSRTByName(const Vector2& scale, float rotateDeg, const Vector2& translate, const std::string& materialName) {
        const int32_t index = GetMeshIndexByName(materialName);
        if (index >= 0) SetUVSRT(scale, rotateDeg, translate, index);
    }
}
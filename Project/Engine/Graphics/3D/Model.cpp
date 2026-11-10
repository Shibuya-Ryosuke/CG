#include "Model.h"
#include "../2D/TextureManager.h"
#include "../../Core/Math/Collision.h"
#include "../../Editor/ModelPicker.h"
#include "../../Camera/CameraManager.h"
#include "../../Core/Asset/AssetRegistry.h"
#include <cassert>

namespace RyoEngine {

    namespace {
        constexpr float kDegToRad = 3.14159265358979323846f / 180.0f;
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
        worldTransform_.SetLocal(transform);
        if (onTransformEdited_) {
            onTransformEdited_(transform);
        }
    }

    // ------------------------------------------------------------------
    // 作成
    // ------------------------------------------------------------------
    std::unique_ptr<Model> Model::Create(uint32_t modelIndex, uint32_t textureIndex) {
        std::unique_ptr<Model> instance = std::make_unique<Model>();
        instance->SetModel(modelIndex, textureIndex);
        return instance;
    }

    std::unique_ptr<Model> Model::Create(const std::string& filePath, uint32_t textureIndex) {
        std::unique_ptr<Model> instance = std::make_unique<Model>();
        instance->SetModel(filePath, textureIndex);
        return instance;
    }

    void Model::SetModel(const std::string& filePath, uint32_t textureIndex) {
        // パスでも名前でもよい。見つからなければ無効なindex (描画されないだけで落ちない)
        const std::string fullPath = AssetRegistry::Resolve(AssetRegistry::Type::Model, filePath);
        SetModel(fullPath.empty() ? ModelManager::kInvalidModelIndex : ModelManager::LoadObj(fullPath), textureIndex);
    }

    void Model::SetModel(uint32_t modelIndex, uint32_t textureIndex) {
        // 同じモデルなら何もしない (見た目をリセットしない)。ただしテクスチャ指定があれば反映する
        if (modelIndex != ModelManager::kInvalidModelIndex && modelIndex == modelIndex_) {
            if (textureIndex != 0) {
                SetTex(textureIndex);
            }
            return;
        }

        // 自動キー("objのパス#何番目")のときは、モデルが変わるので登録を外して付け直す。
        // SetNameで名前を付けている場合は、キーが変わらないのでそのまま。
        // NOTE: layoutApplied_ / layoutEdited_ / transform_ には触らない
        //       (差し替えでTransformが保存値に巻き戻らないように、また編集済みの印を残すため)
        if (layoutRegistered_ && !layoutSlot_.path.empty()) {
            ModelLayout::Unregister(this, layoutSlot_);
            layoutRegistered_ = false;
        }

        modelIndex_ = modelIndex;
        meshes_.clear();
        isVisible_ = true;

        const ModelManager::ModelAsset* asset = ModelManager::GetAsset(modelIndex);
        if (!asset) {
            return; // 無効なindex。描画されないだけで落とさない
        }

        if (!layoutRegistered_) {
            layoutSlot_ = ModelLayout::Register(this, asset->filePath);
            layoutRegistered_ = true;
        }

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
    void Model::TransferMatrix(bool frustumCulling) {
        // 視錐台判定はカメラ確定後のDraw()で行うので、ここでは指定だけ覚えておく
        frustumCulling_ = frustumCulling;
        TransferMatrix();
    }

    void Model::TransferMatrix() {
        // 保存された Transform があれば、最初の1回だけ適用する (コードで入れた初期値より保存値が勝つ)
        if (!layoutApplied_ && layoutRegistered_) {
            layoutApplied_ = true;
            Transform saved{};
            if (ModelLayout::TryGetSaved(layoutSlot_.key, saved)) {
                worldTransform_.SetLocal(saved);
            }
        }

        UpdateWorld();
    }

    void Model::UpdateWorld() {
        // 親がいれば「ローカル × 親のワールド行列」。親がこのフレームで未計算なら先に計算される
        worldTransform_.UpdateMatrix();

        const ModelManager::ModelAsset* asset = ModelManager::GetAsset(modelIndex_);
        if (!asset) {
            isVisible_ = false;
            return;
        }

        // フラスタムカリング用の球。非一様スケールでも安全なよう一番大きい成分で球を拡大する
        // (親のスケールも反映するため、ローカルのSRTではなくワールド行列から取り出したスケールを使う)
        const Vector3 worldScale = worldTransform_.GetWorldScale();
        float maxScale = worldScale.x;
        if (worldScale.y > maxScale) maxScale = worldScale.y;
        if (worldScale.z > maxScale) maxScale = worldScale.z;

        // 球の計算はクリック選択のため常に行う
        pickCenter_ = worldTransform_.GetWorldPosition();
        pickRadius_ = asset->boundingRadius * maxScale;
    }

    void Model::Draw() {
        const ModelManager::ModelAsset* asset = ModelManager::GetAsset(modelIndex_);
        if (!asset) {
            isVisible_ = false;
            return;
        }

        // 親がいる場合、TransferMatrix の後に親が動かされているかもしれないので、親の最新の行列で計算し直す
        // (Update で 子のTransferMatrix → 親の移動・TransferMatrix の順になっていても、1フレーム遅れないように)
        if (worldTransform_.GetParent()) {
            UpdateWorld();
        }

        // フラスタムカリング。CameraManager::UpdateAll()で確定済みの描画カメラで判定する
        // frustumCulling=false の場合は常に描画する
        if (frustumCulling_) {
            Sphere boundingSphere{};
            boundingSphere.center = pickCenter_;
            boundingSphere.radius = pickRadius_;
            isVisible_ = IsCollision(boundingSphere, CameraManager::GetActive().GetFrustumPlanes());
        } else {
            isVisible_ = true;
        }

        if (!isVisible_) {
            return;
        }

        if (pickable_) {
            ModelPicker::Submit(this, pickCenter_, pickRadius_);
        }

        for (size_t i = 0; i < meshes_.size() && i < asset->meshes.size(); ++i) {
            const MeshState& state = meshes_[i];
            const ModelManager::MeshAsset& meshAsset = asset->meshes[i];

            ModelInstanceData data;
            data.world = worldTransform_.GetWorldMatrix();
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

            data.params[0] = shaderParams_[0];
            data.params[1] = shaderParams_[1];

            ModelDrawOptions options;
            options.blendMode = state.blendMode;
            options.shader = state.shader;
            options.castShadow = castShadow_;
            options.isBackground = isBackground_;
            ModelManager::Submit(modelIndex_, static_cast<uint32_t>(i), textureHandle, options, data);
        }
    }

    // ------------------------------------------------------------------
    // Getter
    // ------------------------------------------------------------------
    bool Model::GetHasUV(size_t meshIndex) const {
        const auto* asset = ModelManager::GetAsset(modelIndex_);
        return asset && meshIndex < asset->meshes.size() ? asset->meshes[meshIndex].hasUV : true;
    }

    const std::string& Model::GetMaterialName(size_t meshIndex) const {
        static const std::string empty;
        const auto* asset = ModelManager::GetAsset(modelIndex_);
        return asset && meshIndex < asset->meshes.size() ? asset->meshes[meshIndex].materialName : empty;
    }

    int32_t Model::GetMeshIndexByName(const std::string& materialName) const {
        const auto* asset = ModelManager::GetAsset(modelIndex_);
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
        const auto* asset = ModelManager::GetAsset(modelIndex_);
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
        // パスでも名前でもよい。見つからなければ white1x1
        const std::string fullPath = AssetRegistry::Resolve(AssetRegistry::Type::Texture, filePath);
        SetTex(fullPath.empty() ? TextureManager::GetWhiteTex() : TextureManager::Load(fullPath), meshIndex);
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

    void Model::SetBlendMode(BlendMode blendMode, int32_t meshIndex) {
        ForEachMesh(meshIndex, [&](MeshState& mesh) { mesh.blendMode = blendMode; });
        PreparePipelines();
    }

    void Model::SetBackground(bool isBackground) {
        isBackground_ = isBackground;
        PreparePipelines();
    }

    void Model::SetShader(uint32_t shader, int32_t meshIndex) {
        ForEachMesh(meshIndex, [&](MeshState& mesh) { mesh.shader = shader; });
        PreparePipelines();
    }

    void Model::SetShaderParam(uint32_t index, float value) {
        if (index >= 8) return;
        Vector4& slot = shaderParams_[index / 4];
        switch (index % 4) {
        case 0: slot.x = value; break;
        case 1: slot.y = value; break;
        case 2: slot.z = value; break;
        default: slot.w = value; break;
        }
    }

    void Model::SetShaderParams(uint32_t slot, const Vector4& values) {
        if (slot >= 2) return;
        shaderParams_[slot] = values;
    }

    float Model::GetShaderParam(uint32_t index) const {
        if (index >= 8) return 0.0f;
        const Vector4& slot = shaderParams_[index / 4];
        switch (index % 4) {
        case 0: return slot.x;
        case 1: return slot.y;
        case 2: return slot.z;
        default: return slot.w;
        }
    }

    void Model::PreparePipelines() {
        for (const MeshState& mesh : meshes_) {
            ModelManager::PreparePipeline(mesh.shader, mesh.blendMode, isBackground_);
        }
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
    void Model::SetBlendModeByName(BlendMode blendMode, const std::string& materialName) {
        const int32_t index = GetMeshIndexByName(materialName);
        if (index >= 0) SetBlendMode(blendMode, index);
    }
    void Model::SetShaderByName(uint32_t shader, const std::string& materialName) {
        const int32_t index = GetMeshIndexByName(materialName);
        if (index >= 0) SetShader(shader, index);
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

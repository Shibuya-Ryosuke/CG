#include "Model.h"
#include "../2D/TextureManager.h"
#include "../../Core/Math/Collision.h"
#include "../../Core/Math/Color.h"
#include "../../Editor/ModelPicker.h"
#include "../../Camera/CameraManager.h"
#include "../../Core/Asset/AssetRegistry.h"
#include "../../Core/Base/Logger.h"
#include "../../Core/Base/TimeManager.h"
#include "../PrimitiveRenderer/PrimitiveRenderer.h"
#include "SkinningManager.h"
#include <cassert>

namespace RyoEngine {

    namespace {
        constexpr float kDegToRad = 3.14159265358979323846f / 180.0f;
    }

    Model::~Model() {
        // スキニングのバッファを返す (GPUが使い終わるまで SkinningManager が残しておく)
        SkinningManager::ReleaseInstance(skinInstance_);
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
        // アニメーションの状態も新しいモデルに合わせて作り直す
        ResetAnimation();

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

        // アニメーションを進める (1フレームに1回だけ)
        UpdateAnimation();
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
        // アニメーションで元の姿勢より外へ動くことがあるので、一番動いたボーンの分だけ球を広げる
        const float animationOffset = animator_ ? animator_->GetMaxJointOffset() : 0.0f;
        pickRadius_ = (asset->boundingRadius + animationOffset) * maxScale;
    }

    void Model::Draw() {
        const ModelManager::ModelAsset* asset = ModelManager::GetAsset(modelIndex_);
        if (!asset) {
            isVisible_ = false;
            return;
        }

        // TransferMatrix を呼んでいない場合も、アニメーションは進める (このフレームで進めていれば何もしない)
        UpdateAnimation();

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

        // 姿勢が変わっていたら、スキニングを予約する (実行は EndFrame で、描画より前にまとめて行われる)
        if (animator_ && skinDirty_) {
            for (size_t i = 0; i < asset->meshes.size(); ++i) {
                if (asset->meshes[i].isSkinned) {
                    SkinningManager::Request(skinInstance_, static_cast<uint32_t>(i), animator_->GetJointMatrices());
                }
            }
            skinDirty_ = false;
        }

#ifdef NRELEASE
        if (animator_ && ModelManager::IsShowSkeleton()) {
            DrawSkeleton();
        }
#endif

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

            // 鏡面反射: 色×強さ と 鋭さ をまとめて送る
            data.specular = {
                state.specularColor.x * state.specularIntensity,
                state.specularColor.y * state.specularIntensity,
                state.specularColor.z * state.specularIntensity,
                state.shininess };

            ModelDrawOptions options;
            options.blendMode = state.blendMode;
            options.shader = state.shader;
            options.castShadow = castShadow_;
            options.isBackground = isBackground_;
            // スキニング済みの頂点があれば、それで描く (まだ無ければ元の姿勢の頂点で描く)
            if (animator_ && meshAsset.isSkinned) {
                options.vertexBuffer = SkinningManager::GetVertexBufferView(skinInstance_, static_cast<uint32_t>(i));
            }
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

    void Model::SetColor(uint32_t rgbHex, int32_t meshIndex) {
        SetColor(ColorFromHex(rgbHex), meshIndex);
    }

    void Model::SetColor(std::string_view hex, int32_t meshIndex) {
        SetColor(ColorFromHex(hex), meshIndex);
    }

    void Model::SetEmissive(uint32_t rgbHex, float intensity, int32_t meshIndex) {
        SetEmissive(ColorFromHexRGB(rgbHex), intensity, meshIndex);
    }

    void Model::SetEmissive(std::string_view hex, float intensity, int32_t meshIndex) {
        SetEmissive(ColorFromHexRGB(hex), intensity, meshIndex);
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

    void Model::SetSpecular(const Vector3& color, float intensity, float shininess, int32_t meshIndex) {
        // shininess が0以下だと pow がおかしくなるので、最低1にしておく
        const float safeShininess = shininess < 1.0f ? 1.0f : shininess;
        ForEachMesh(meshIndex, [&](MeshState& mesh) {
            mesh.specularColor = color;
            mesh.specularIntensity = intensity;
            mesh.shininess = safeShininess;
            });
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
    void Model::SetColorByName(uint32_t rgbHex, const std::string& materialName) {
        SetColorByName(ColorFromHex(rgbHex), materialName);
    }
    void Model::SetColorByName(std::string_view hex, const std::string& materialName) {
        SetColorByName(ColorFromHex(hex), materialName);
    }
    void Model::SetLambertByName(ShadingMode mode, const std::string& materialName) {
        const int32_t index = GetMeshIndexByName(materialName);
        if (index >= 0) SetLambert(mode, index);
    }
    void Model::SetSpecularByName(const Vector3& color, float intensity, float shininess, const std::string& materialName) {
        const int32_t index = GetMeshIndexByName(materialName);
        if (index >= 0) SetSpecular(color, intensity, shininess, index);
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

    // ------------------------------------------------------------------
    // アニメーション
    // ------------------------------------------------------------------
    void Model::ResetAnimation() {
        // 前のモデルのボーンにくっついていたものは、親子が外れる (WorldTransform が消えるときに自動で外れる)
        jointAttachments_.clear();
        SkinningManager::ReleaseInstance(skinInstance_);
        skinInstance_ = SkinningManager::kInvalidInstance;
        animator_.reset();
        skinDirty_ = true;
        animatedFrame_ = 0;

        const ModelManager::ModelAsset* asset = ModelManager::GetAsset(modelIndex_);
        if (!asset || !asset->hasSkeleton) {
            return;
        }
        animator_ = std::make_unique<Animator>();
        animator_->Initialize(modelIndex_);
        skinInstance_ = SkinningManager::CreateInstance(modelIndex_);
    }

    void Model::UpdateAnimation() {
        if (!animator_) {
            return;
        }
        // 1フレームに1回だけ進める (TransferMatrix と Draw の両方から呼ばれるため)
        const uint64_t frame = ModelManager::GetFrameIndex();
        if (animatedFrame_ == frame) {
            return;
        }
        animatedFrame_ = frame;

        // ゲームの時間の速さ (スローなど) に合わせて進める
        if (animator_->Update(TimeManager::GetScaleTime())) {
            skinDirty_ = true;
            UpdateJointTransforms();
        }
    }

    void Model::UpdateJointTransforms() {
        if (!animator_) {
            return;
        }
        const std::vector<Matrix4x4>& joints = animator_->GetJointMatrices();
        for (auto& [name, attachment] : jointAttachments_) {
            if (attachment.jointIndex < 0 || static_cast<size_t>(attachment.jointIndex) >= joints.size()) {
                continue;
            }
            // ボーンの行列 (モデルの原点から見た座標) を、親(このモデル)から見た SRT に分ける
            const Matrix4x4& m = joints[static_cast<size_t>(attachment.jointIndex)];
            Transform local{};
            local.translate = { m.m[3][0], m.m[3][1], m.m[3][2] };
            Vector3 axisX = { m.m[0][0], m.m[0][1], m.m[0][2] };
            Vector3 axisY = { m.m[1][0], m.m[1][1], m.m[1][2] };
            Vector3 axisZ = { m.m[2][0], m.m[2][1], m.m[2][2] };
            local.scale = { Length(axisX), Length(axisY), Length(axisZ) };
            // 裏返っている (行列式が負) ときは、X のスケールを負にして回転を正しく取り出す
            if (Dot(Cross(axisX, axisY), axisZ) < 0.0f) {
                local.scale.x = -local.scale.x;
            }
            Matrix4x4 rotation = MakeIdentity4x4();
            const Vector3 axes[3] = { axisX, axisY, axisZ };
            const float scales[3] = { local.scale.x, local.scale.y, local.scale.z };
            for (int row = 0; row < 3; ++row) {
                const float s = scales[row] != 0.0f ? scales[row] : 1.0f;
                rotation.m[row][0] = axes[row].x / s;
                rotation.m[row][1] = axes[row].y / s;
                rotation.m[row][2] = axes[row].z / s;
            }
            local.rotate = QuaternionToEuler(MakeQuaternionFromRotateMatrix(rotation)); // 度数法
            attachment.transform->SetLocal(local);
        }
    }

    WorldTransform* Model::GetJointTransform(const std::string& jointName) {
        if (auto it = jointAttachments_.find(jointName); it != jointAttachments_.end()) {
            return it->second.transform.get();
        }
        const ModelManager::ModelAsset* asset = ModelManager::GetAsset(modelIndex_);
        const int32_t jointIndex = (asset && animator_) ? asset->skeleton.FindJoint(jointName) : -1;
        if (jointIndex < 0) {
            Logger::LogWarning("[Model] ボーンが見つかりません: \"{}\" ({})\n", jointName, asset ? asset->filePath : std::string());
            return nullptr;
        }

        JointAttachment attachment;
        attachment.jointIndex = jointIndex;
        attachment.transform = std::make_unique<WorldTransform>();
        attachment.transform->SetParent(&worldTransform_);
        WorldTransform* result = attachment.transform.get();
        jointAttachments_.emplace(jointName, std::move(attachment));
        // 今の姿勢をすぐに反映する
        UpdateJointTransforms();
        return result;
    }

    void Model::DrawSkeleton() const {
        const ModelManager::ModelAsset* asset = ModelManager::GetAsset(modelIndex_);
        if (!asset || !animator_) {
            return;
        }
        const std::vector<Matrix4x4>& joints = animator_->GetJointMatrices();
        const Matrix4x4& world = worldTransform_.GetWorldMatrix();
        const Vector4 lineColor = { 1.0f, 0.85f, 0.1f, 1.0f };
        for (size_t i = 0; i < joints.size() && i < asset->skeleton.joints.size(); ++i) {
            const int32_t parent = asset->skeleton.joints[i].parent;
            if (parent < 0) {
                continue;
            }
            // ボーンの位置 (モデルの原点から見た座標) を、ワールド座標にして親とつなぐ
            const Matrix4x4& child = joints[i];
            const Matrix4x4& parentMatrix = joints[static_cast<size_t>(parent)];
            const Vector3 start = TransformVector3({ parentMatrix.m[3][0], parentMatrix.m[3][1], parentMatrix.m[3][2] }, world);
            const Vector3 end = TransformVector3({ child.m[3][0], child.m[3][1], child.m[3][2] }, world);
            PrimitiveRenderer::DrawLine3D(start, end, lineColor);
        }
    }

    void Model::PlayAnimation(const std::string& name, bool loop, float blendSeconds, bool restart) {
        if (animator_) {
            animator_->Play(name, loop, blendSeconds, restart);
        }
    }

    void Model::StopAnimation() {
        if (animator_) animator_->Stop();
    }

    void Model::ResetPose() {
        if (animator_) animator_->ResetPose();
    }

    void Model::PauseAnimation() {
        if (animator_) animator_->Pause();
    }

    void Model::ResumeAnimation() {
        if (animator_) animator_->Resume();
    }

    const std::string& Model::GetCurrentAnimationName() const {
        static const std::string empty;
        return animator_ ? animator_->GetCurrentName() : empty;
    }

    bool Model::HasAnimation(const std::string& name) const {
        const ModelManager::ModelAsset* asset = ModelManager::GetAsset(modelIndex_);
        return asset && asset->FindAnimation(name) >= 0;
    }

    std::vector<std::string> Model::GetAnimationNames() const {
        std::vector<std::string> names;
        if (const ModelManager::ModelAsset* asset = ModelManager::GetAsset(modelIndex_)) {
            names.reserve(asset->animations.size());
            for (const SkeletalAnimation& animation : asset->animations) {
                names.push_back(animation.name);
            }
        }
        return names;
    }

    uint32_t Model::LoadAnimation(const std::string& filePath, const std::string& name) {
        // パスでも名前でもよい (Model::Create と同じ)
        const std::string fullPath = AssetRegistry::Resolve(AssetRegistry::Type::Model, filePath);
        if (fullPath.empty()) {
            Logger::LogWarning("[Model] アニメーションのファイルが見つかりません: {}\n", filePath);
            return 0;
        }
        return ModelManager::LoadAnimation(modelIndex_, fullPath, name);
    }
}

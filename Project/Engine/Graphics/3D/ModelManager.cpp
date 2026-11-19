#include "ModelManager.h"
#include "../../Core/Base/DirectXCommon.h"
#include "../../Core/Base/Logger.h"
#include "../../Core/Base/SrvManager.h"
#include "../2D/TextureManager.h"
#include "../Light/LightManager.h"
#include "../Shadow/ShadowMap.h"
#include "../Shader/ShaderManager.h"
#include "../../Core/Base/TimeManager.h"
#include "../../Core/Loader/ModelLoader.h"
#include "../../Editor/ModelPicker.h"
#include "../../Editor/ParamEditor.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>

namespace RyoEngine {

    namespace {
        // 作り直したバッファを、GPUが使い終わるまで生かしておくフレーム数
        constexpr int kRetireFrames = 3;
        // インスタンスバッファの初期容量 (体x メッシュ数)
        constexpr size_t kInitialInstanceCapacity = 1024;
    }

    ModelManager* ModelManager::GetInstance() {
        static ModelManager instance;
        return &instance;
    }

    void ModelManager::Initialize() {
        ModelManager* instance = GetInstance();

        Logger::Log("ModelManager : Initializing...\n");
        instance->viewProjection_ = MakeIdentity4x4();
        instance->CreateRootSignature();
        instance->CreatePipelineStates();
        instance->CreateBuffers();
        RegisterDebugParams();
        Logger::LogSuccess("ModelManager : Initialized\n");
    }

    void ModelManager::Finalize() {
        ModelManager* instance = GetInstance();

        Logger::Log("ModelManager : Finalizing...\n");
        instance->frameData_.clear();
        instance->entries_.clear();
        instance->backgroundGroups_.clear();
        instance->opaqueGroups_.clear();
        instance->sortedGroups_.clear();
        instance->commutativeGroups_.clear();
        instance->shadowGroups_.clear();
        instance->retired_.clear();

        instance->instanceResource_.Reset();
        instance->instanceMapped_ = nullptr;
        instance->instanceCapacity_ = 0;
        instance->cameraVPResource_.Reset();
        instance->cameraVPData_ = nullptr;

        instance->assets_.clear();
        instance->pathToIndex_.clear();

        instance->pipelineCache_.clear();
        instance->shadowPipelineCache_.clear();
        instance->retiredPipelines_.clear();
        instance->rootSignature_.Reset();
        Logger::LogSuccess("ModelManager : Finalized\n");
    }

    // ------------------------------------------------------------------
    // モデルの読み込み
    // ------------------------------------------------------------------
    uint32_t ModelManager::LoadObj(const std::string& filePath) {
        ModelManager* instance = GetInstance();

        auto cached = instance->pathToIndex_.find(filePath);
        if (cached != instance->pathToIndex_.end()) {
            return cached->second;
        }

        ModelLoader::ModelData modelData = ModelLoader::LoadModelFile(filePath);
        if (modelData.meshes.empty()) {
            Logger::LogWarning("[ModelManager] メッシュが1つも読み込めませんでした: " + filePath + "\n");
            return kInvalidModelIndex;
        }

        auto device = DirectXCommon::GetDevice();

        auto asset = std::make_unique<ModelAsset>();
        asset->filePath = filePath;
        asset->meshes.reserve(modelData.meshes.size());

        // 骨格を持つモデル: 元の姿勢での各ボーンの行列を先に求めておく (元の姿勢の頂点を作るのに使う)
        if (modelData.hasSkeleton) {
            asset->hasSkeleton = true;
            asset->skeleton = std::move(modelData.skeleton);
            asset->bindPoseMatrices = ComputeBindPoseMatrices(asset->skeleton);
        }

        for (const auto& src : modelData.meshes) {
            MeshAsset mesh;

            // 骨格を持つモデルの頂点は、ボーンから見た座標のこともあるので、元の姿勢でスキニングした頂点を通常の頂点にする
            // (スキニングの結果がまだ無いときや、カリングの球の計算に使う)
            std::vector<VertexData> bindPoseVertices;
            const std::vector<VertexData>* uploadVertices = &src.vertices;
            if (asset->hasSkeleton && !src.bones.empty() && src.influences.size() == src.vertices.size()) {
                mesh.isSkinned = true;
                mesh.skinBones = src.bones;

                // ボーンごとの行列 (元の姿勢) と、法線用の逆転置行列
                std::vector<Matrix4x4> palette(src.bones.size());
                std::vector<Matrix4x4> paletteNormal(src.bones.size());
                for (size_t b = 0; b < src.bones.size(); ++b) {
                    const SkinBone& bone = src.bones[b];
                    const Matrix4x4& jointMatrix = asset->bindPoseMatrices[static_cast<size_t>(bone.jointIndex)];
                    palette[b] = bone.inverseBindMatrix * jointMatrix;
                    paletteNormal[b] = Transpose(Inverse(palette[b]));
                }

                // スキニングの入力 (頂点 + ボーンの割合) と、元の姿勢でスキニングした頂点
                std::vector<SkinVertexData> skinVertices(src.vertices.size());
                bindPoseVertices.resize(src.vertices.size());
                for (size_t v = 0; v < src.vertices.size(); ++v) {
                    const VertexData& vertex = src.vertices[v];
                    const VertexInfluence& influence = src.influences[v];
                    SkinVertexData& skin = skinVertices[v];
                    skin.position = vertex.position;
                    skin.texcoord = vertex.texcoord;
                    skin.normal = vertex.normal;
                    skin.weight = { influence.weights[0], influence.weights[1], influence.weights[2], influence.weights[3] };
                    for (int k = 0; k < 4; ++k) {
                        skin.index[k] = influence.indices[k];
                    }

                    VertexData skinned = vertex;
                    const float totalWeight = skin.weight.x + skin.weight.y + skin.weight.z + skin.weight.w;
                    if (totalWeight > 0.0f) {
                        Vector3 position = { 0.0f, 0.0f, 0.0f };
                        Vector3 normal = { 0.0f, 0.0f, 0.0f };
                        const Vector3 p = { vertex.position.x, vertex.position.y, vertex.position.z };
                        for (int k = 0; k < 4; ++k) {
                            const float w = influence.weights[k];
                            if (w <= 0.0f) continue;
                            const Matrix4x4& m = palette[influence.indices[k]];
                            const Matrix4x4& n = paletteNormal[influence.indices[k]];
                            // 行ベクトル規約: 点は平行移動も入れる、法線は 3x3 だけ使う
                            position += Vector3{
                                p.x * m.m[0][0] + p.y * m.m[1][0] + p.z * m.m[2][0] + m.m[3][0],
                                p.x * m.m[0][1] + p.y * m.m[1][1] + p.z * m.m[2][1] + m.m[3][1],
                                p.x * m.m[0][2] + p.y * m.m[1][2] + p.z * m.m[2][2] + m.m[3][2] } * w;
                            normal += Vector3{
                                vertex.normal.x * n.m[0][0] + vertex.normal.y * n.m[1][0] + vertex.normal.z * n.m[2][0],
                                vertex.normal.x * n.m[0][1] + vertex.normal.y * n.m[1][1] + vertex.normal.z * n.m[2][1],
                                vertex.normal.x * n.m[0][2] + vertex.normal.y * n.m[1][2] + vertex.normal.z * n.m[2][2] } * w;
                        }
                        skinned.position = { position.x, position.y, position.z, 1.0f };
                        skinned.normal = Normalize(normal);
                    }
                    bindPoseVertices[v] = skinned;
                }
                uploadVertices = &bindPoseVertices;

                const size_t skinBytes = sizeof(SkinVertexData) * skinVertices.size();
                mesh.skinVertexResource = DirectXCommon::CreateBufferResource(device, skinBytes);
                SkinVertexData* skinMapped = nullptr;
                mesh.skinVertexResource->Map(0, nullptr, reinterpret_cast<void**>(&skinMapped));
                std::memcpy(skinMapped, skinVertices.data(), skinBytes);
                mesh.skinVertexResource->Unmap(0, nullptr);
            }

            // 頂点バッファ (インデックスバッファは使わない、三角形展開済みの頂点列)
            const size_t bytes = sizeof(VertexData) * uploadVertices->size();
            mesh.vertexResource = DirectXCommon::CreateBufferResource(device, bytes);
            mesh.vertexBufferView.BufferLocation = mesh.vertexResource->GetGPUVirtualAddress();
            mesh.vertexBufferView.SizeInBytes = static_cast<UINT>(bytes);
            mesh.vertexBufferView.StrideInBytes = sizeof(VertexData);

            VertexData* mapped = nullptr;
            mesh.vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
            std::memcpy(mapped, uploadVertices->data(), bytes);
            mesh.vertexResource->Unmap(0, nullptr);
            mesh.vertexCount = static_cast<uint32_t>(uploadVertices->size());

            // フラスタムカリング用: 原点から一番遠い頂点までの距離
            for (const auto& vertex : *uploadVertices) {
                const Vector3 position = { vertex.position.x, vertex.position.y, vertex.position.z };
                const float distance = Length(position);
                if (distance > asset->boundingRadius) {
                    asset->boundingRadius = distance;
                }
            }

            mesh.hasUV = src.hasUV;

            // マテリアル名とデフォルトテクスチャ
            std::string texturePath;
            const ModelLoader::MaterialData* material = nullptr;
            if (src.materialIndex < modelData.materials.size()) {
                material = &modelData.materials[src.materialIndex];
                mesh.materialName = modelData.materials[src.materialIndex].name;
                texturePath = modelData.materials[src.materialIndex].textureFilePath;
            }
            // UVが無いメッシュはテクスチャを引けない(texcoordは(0,0)固定)ので、必ず white にする
            // ファイルに埋め込まれた画像 (glb など) があれば、それを使う
            if (mesh.hasUV && material && !material->embeddedTextureData.empty()) {
                if (material->embeddedTextureHeight == 0) {
                    mesh.defaultTextureHandle = TextureManager::LoadFromMemory(material->embeddedTextureKey,
                        material->embeddedTextureData.data(), material->embeddedTextureData.size());
                } else {
                    mesh.defaultTextureHandle = TextureManager::LoadFromPixelsBGRA(material->embeddedTextureKey,
                        material->embeddedTextureData.data(), material->embeddedTextureWidth, material->embeddedTextureHeight);
                }
            } else if (mesh.hasUV && !texturePath.empty()) {
                mesh.defaultTextureHandle = TextureManager::Load(texturePath);
            } else {
                mesh.defaultTextureHandle = TextureManager::GetWhiteTex();
            }

            asset->meshes.push_back(std::move(mesh));
        }

        // 骨格を持つモデル: ファイルに入っているアニメーションを登録する
        if (asset->hasSkeleton) {
            SetupSkeletalAsset(*asset, std::move(modelData.animations));
        }

        instance->assets_.push_back(std::move(asset));
        const uint32_t modelIndex = static_cast<uint32_t>(instance->assets_.size()); // 1始まり
        instance->pathToIndex_[filePath] = modelIndex;
        return modelIndex;
    }

    const ModelManager::ModelAsset* ModelManager::GetAsset(uint32_t modelIndex) {
        ModelManager* instance = GetInstance();

        if (modelIndex == kInvalidModelIndex || modelIndex > instance->assets_.size()) {
            return nullptr;
        }
        return instance->assets_[modelIndex - 1].get();
    }

    // ------------------------------------------------------------------
    // フレーム処理
    // ------------------------------------------------------------------
    void ModelManager::NewFrame() {
        ModelManager* instance = GetInstance();

        // フレームの通し番号 (アニメーションを1フレームに1回だけ進めるため)
        ++instance->frameIndex_;

        ModelPicker::NewFrame();
        instance->frameData_.clear();
        instance->entries_.clear();
        instance->backgroundGroups_.clear();
        instance->opaqueGroups_.clear();
        instance->sortedGroups_.clear();
        instance->commutativeGroups_.clear();
        instance->shadowGroups_.clear();
        instance->prepared_ = false;

        // 作り直し前の古いバッファを、時間が経ったものから解放する
        for (auto& retired : instance->retired_) {
            --retired.framesLeft;
        }
        instance->retired_.erase(
            std::remove_if(instance->retired_.begin(), instance->retired_.end(),
                [](const RetiredBuffer& r) { return r.framesLeft <= 0; }),
            instance->retired_.end());

        // シェーダーが再読み込みされていたらPSOを作り直す。捨てたPSOも時間が経ったものから解放する
        instance->RetirePipelinesIfShaderReloaded();
        for (auto& retired : instance->retiredPipelines_) {
            --retired.framesLeft;
        }
        instance->retiredPipelines_.erase(
            std::remove_if(instance->retiredPipelines_.begin(), instance->retiredPipelines_.end(),
                [](const RetiredPipeline& r) { return r.framesLeft <= 0; }),
            instance->retiredPipelines_.end());

        // シェーダーに渡す経過時間 (前フレームのスケール済みdeltaTime)
        instance->time_ += TimeManager::GetScaleTime();
    }

    void ModelManager::RetirePipelinesIfShaderReloaded() {
        const uint32_t reloadCount = ShaderManager::GetReloadCount();
        if (reloadCount == shaderReloadCount_) {
            return;
        }
        shaderReloadCount_ = reloadCount;

        for (auto& [key, pipelineState] : pipelineCache_) {
            retiredPipelines_.push_back({ std::move(pipelineState), kRetireFrames });
        }
        for (auto& [key, pipelineState] : shadowPipelineCache_) {
            retiredPipelines_.push_back({ std::move(pipelineState), kRetireFrames });
        }
        pipelineCache_.clear();
        shadowPipelineCache_.clear();
        Logger::Log("[ModelManager] シェーダーが再読み込みされたので、PSOを作り直します\n");
    }

    void ModelManager::Submit(uint32_t modelIndex, uint32_t meshIndex, uint32_t textureHandle,
        const ModelDrawOptions& options, const ModelInstanceData& data) {
        ModelManager* instance = GetInstance();

        const ModelAsset* asset = GetAsset(modelIndex);
        if (!asset || meshIndex >= asset->meshes.size()) {
            return;
        }

        Entry entry;
        entry.key = (static_cast<uint64_t>(modelIndex) << 48) |
            (static_cast<uint64_t>(meshIndex & 0xFFFFu) << 32) |
            static_cast<uint64_t>(textureHandle);
        entry.dataIndex = static_cast<uint32_t>(instance->frameData_.size());
        entry.shader = options.shader;
        entry.blendMode = options.blendMode;
        entry.isBackground = options.isBackground;
        if (options.vertexBuffer) {
            entry.hasVertexBuffer = true;
            entry.vertexBuffer = *options.vertexBuffer;
        }
        // 加算・減算・スクリーンは光るエフェクト向けなので、影を落とさない。背景も影を落とさない
        entry.castShadow = options.castShadow && !options.isBackground &&
            (options.blendMode == BlendMode::None || options.blendMode == BlendMode::Normal || options.blendMode == BlendMode::Multiply);

        instance->frameData_.push_back(data);
        instance->entries_.push_back(entry);
    }

    void ModelManager::EnsureInstanceCapacity(size_t count) {
        if (instanceResource_ && count <= instanceCapacity_) {
            return;
        }

        size_t newCapacity = instanceCapacity_ * 2;
        if (newCapacity < count) newCapacity = count;
        if (newCapacity < kInitialInstanceCapacity) newCapacity = kInitialInstanceCapacity;

        // 古いバッファは、GPUがまだ読んでいるかもしれないので数フレーム生かしておく
        if (instanceResource_) {
            RetiredBuffer retired;
            retired.resource = std::move(instanceResource_);
            retired.framesLeft = kRetireFrames;
            retired_.push_back(std::move(retired));
        }

        auto device = DirectXCommon::GetDevice();
        instanceResource_ = DirectXCommon::CreateBufferResource(device, sizeof(ModelInstanceData) * newCapacity);
        instanceResource_->Map(0, nullptr, reinterpret_cast<void**>(&instanceMapped_));
        instanceCapacity_ = newCapacity;
    }

    void ModelManager::BuildFrame() {
        if (prepared_) {
            return;
        }
        prepared_ = true;
        backgroundGroups_.clear();
        opaqueGroups_.clear();
        sortedGroups_.clear();
        commutativeGroups_.clear();
        shadowGroups_.clear();

        // カメラのVPはここでGPUバッファへ書く (GPUが使う直前に書くため)
        cameraVPData_->viewProjection = viewProjection_;
        cameraVPData_->position = cameraPosition_;

        if (entries_.empty()) {
            return;
        }

        // ブレンドモードごとに振り分ける
        std::vector<const Entry*> background;
        std::vector<const Entry*> opaque;
        std::vector<const Entry*> sorted;
        std::vector<const Entry*> commutative;
        std::vector<const Entry*> shadow;
        for (Entry& entry : entries_) {
            const Matrix4x4& world = frameData_[entry.dataIndex].world;
            const Vector3 toCamera = Vector3{ world.m[3][0], world.m[3][1], world.m[3][2] } - cameraPosition_;
            entry.distanceSq = Dot(toCamera, toCamera);

            if (entry.isBackground) {
                background.push_back(&entry);
            } else if (entry.blendMode == BlendMode::None) {
                opaque.push_back(&entry);
            } else if (entry.blendMode == BlendMode::Normal) {
                sorted.push_back(&entry);
            } else {
                commutative.push_back(&entry);
            }
            if (entry.castShadow) {
                shadow.push_back(&entry);
            }
        }

        // シェーダー → (モデル, メッシュ, テクスチャ) の順。同じキーの中は提出順を保つ (dataIndexで比べる)
        // (シェーダーを一番上にすると、PSOの切り替えがシェーダーの種類数ぶんで済む)
        auto byKey = [](const Entry* a, const Entry* b) {
            if (a->shader != b->shader) return a->shader < b->shader;
            if (a->key != b->key) return a->key < b->key;
            return a->dataIndex < b->dataIndex;
        };
        // 不透明・影: シェーダー → (モデル, メッシュ, テクスチャ) の順に並べてまとめる
        std::sort(opaque.begin(), opaque.end(), byKey);
        std::sort(shadow.begin(), shadow.end(), byKey);
        // 半透明: 奥から手前へ描かないと、手前の半透明に隠れた奥のものが正しく透けない
        std::sort(sorted.begin(), sorted.end(), [&byKey](const Entry* a, const Entry* b) {
            if (a->distanceSq != b->distanceSq) return a->distanceSq > b->distanceSq;
            return byKey(a, b);
            });
        // 加算など: 順番は結果に関係ないので、ブレンドモード → キーで並べてまとめる
        auto byBlendThenKey = [&byKey](const Entry* a, const Entry* b) {
            if (a->blendMode != b->blendMode) return a->blendMode < b->blendMode;
            return byKey(a, b);
        };
        std::sort(commutative.begin(), commutative.end(), byBlendThenKey);
        // 背景: 深度を書かないので、ブレンドモードが違っても互いに隠し合わない。まとめやすい順に並べる
        std::sort(background.begin(), background.end(), byBlendThenKey);

        // インスタンス配列には 背景 → 不透明 → 半透明 → 加算など → 影 の順に詰める
        EnsureInstanceCapacity(background.size() + opaque.size() + sorted.size() + commutative.size() + shadow.size());
        size_t writeIndex = 0;
        AppendGroups(background, writeIndex, backgroundGroups_);
        AppendGroups(opaque, writeIndex, opaqueGroups_);
        AppendGroups(sorted, writeIndex, sortedGroups_);
        AppendGroups(commutative, writeIndex, commutativeGroups_);
        AppendGroups(shadow, writeIndex, shadowGroups_);
    }

    void ModelManager::AppendGroups(const std::vector<const Entry*>& sorted, size_t& writeIndex, std::vector<Group>& outGroups) {
        const Entry* previous = nullptr;
        for (const Entry* entry : sorted) {
            instanceMapped_[writeIndex] = frameData_[entry->dataIndex];

            // キー・シェーダー・ブレンドモードのどれかが変わったら新しいグループ
            // スキニング済みの頂点バッファを持つものは、体ごとに頂点が違うので、ほかとまとめない
            if (!previous || previous->key != entry->key || previous->shader != entry->shader || previous->blendMode != entry->blendMode ||
                previous->hasVertexBuffer || entry->hasVertexBuffer) {
                Group group;
                group.modelIndex = static_cast<uint32_t>(entry->key >> 48);
                group.meshIndex = static_cast<uint32_t>((entry->key >> 32) & 0xFFFFu);
                group.textureHandle = static_cast<uint32_t>(entry->key & 0xFFFFFFFFu);
                group.shader = entry->shader;
                group.blendMode = entry->blendMode;
                group.firstInstance = static_cast<uint32_t>(writeIndex);
                group.instanceCount = 0;
                group.hasVertexBuffer = entry->hasVertexBuffer;
                group.vertexBuffer = entry->vertexBuffer;
                outGroups.push_back(group);
            }
            outGroups.back().instanceCount++;
            previous = entry;
            ++writeIndex;
        }
    }

    // ------------------------------------------------------------------
    // 描画
    // ------------------------------------------------------------------
    void ModelManager::BindCommonState() {
        auto commandList = DirectXCommon::GetCommandList();

        commandList->SetGraphicsRootSignature(rootSignature_.Get());
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

       
        SrvManager::PreDraw(commandList);

        // root 0: カメラVP / 3: ライト配列 / 4: ライト数 / 5: アンビエント / 6: ライトVP / 7: シャドウマップ
        commandList->SetGraphicsRootConstantBufferView(0, cameraVPResource_->GetGPUVirtualAddress());
        commandList->SetGraphicsRootShaderResourceView(3, LightManager::GetLightGPUVirtualAddress());
        commandList->SetGraphicsRootConstantBufferView(4, LightManager::GetLightCountGPUVirtualAddress());
        commandList->SetGraphicsRootConstantBufferView(5, LightManager::GetAmbientGPUVirtualAddress());
        commandList->SetGraphicsRootConstantBufferView(6, ShadowMap::GetLightViewProjGPUVirtualAddress());
        commandList->SetGraphicsRootDescriptorTable(7, TextureManager::GetGPUHandle(ShadowMap::GetShadowMapTextureHandle()));
    }

    void ModelManager::SetDrawParams(BlendMode blendMode) {
        auto commandList = DirectXCommon::GetCommandList();
        // root 8: b4 { uint blendMode; float nearFadeStart; float nearFadeEnd; float time; } (ModelSurface.hlsli の DrawConstants)
        struct DrawParams {
            uint32_t blendMode;
            float nearFadeStart;
            float nearFadeEnd;
            float time;
        } params{ static_cast<uint32_t>(blendMode), nearFadeStart_, nearFadeEnd_, time_ };
        commandList->SetGraphicsRoot32BitConstants(8, 4, &params, 0);
    }

    void ModelManager::BindShaderTextures(uint32_t shader) {
        auto commandList = DirectXCommon::GetCommandList();
        // root 9: t4 / root 10: t5 (設定していなければ white1x1)
        commandList->SetGraphicsRootDescriptorTable(9, TextureManager::GetGPUHandle(ShaderManager::GetTexture(shader, 0)));
        commandList->SetGraphicsRootDescriptorTable(10, TextureManager::GetGPUHandle(ShaderManager::GetTexture(shader, 1)));
    }

    void ModelManager::DrawGroups(const std::vector<Group>& groups, bool isBackground, bool isShadow) {
        auto commandList = DirectXCommon::GetCommandList();
        const D3D12_GPU_VIRTUAL_ADDRESS base = instanceResource_->GetGPUVirtualAddress();

        const Group* previous = nullptr;
        for (const Group& group : groups) {
            // シェーダーかブレンドモードが変わったところだけ、PSO・定数・追加テクスチャを切り替える
            if (!previous || previous->shader != group.shader || previous->blendMode != group.blendMode) {
                commandList->SetPipelineState(isShadow
                    ? GetShadowPipelineState(group.shader)
                    : GetPipelineState(group.shader, group.blendMode, isBackground));
                SetDrawParams(group.blendMode);
                BindShaderTextures(group.shader);
            }
            previous = &group;

            const MeshAsset& mesh = assets_[group.modelIndex - 1]->meshes[group.meshIndex];

            // 影のパスでも結ぶ (アプリの Vertex() がテクスチャを読むことがあるため)
            commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetGPUHandle(group.textureHandle));
            // スキニング済みの頂点バッファがあれば、そちらを使う (頂点の数は同じ)
            commandList->IASetVertexBuffers(0, 1, group.hasVertexBuffer ? &group.vertexBuffer : &mesh.vertexBufferView);

            // グループの先頭インスタンスへずらしたアドレスを渡す。
            // (StartInstanceLocationはSV_InstanceIDに反映されないため、アドレス側でずらす)
            commandList->SetGraphicsRootShaderResourceView(1, base + sizeof(ModelInstanceData) * group.firstInstance);

            commandList->DrawInstanced(mesh.vertexCount, group.instanceCount, 0, 0);
        }
    }

    void ModelManager::Draw() {
        ModelManager* instance = GetInstance();

        instance->BuildFrame();
        if (instance->backgroundGroups_.empty() && instance->opaqueGroups_.empty() &&
            instance->sortedGroups_.empty() && instance->commutativeGroups_.empty()) {
            return;
        }

        instance->BindCommonState();

        // 0. 背景 (深度を書かないので、後から描くものは必ず背景より手前に見える)
        instance->DrawGroups(instance->backgroundGroups_, true, false);
        // 1. 不透明 (深度を書く)
        instance->DrawGroups(instance->opaqueGroups_, false, false);
        // 2. 半透明 (遠い順に並んでいる。自分の奥の面が手前に出ないよう深度は書く)
        instance->DrawGroups(instance->sortedGroups_, false, false);
        // 3. 加算・減算・乗算・スクリーン (ブレンドモード → シェーダーの順に並んでいるので、変わったところだけPSOを切り替える)
        instance->DrawGroups(instance->commutativeGroups_, false, false);
    }

    void ModelManager::DrawShadow() {
        ModelManager* instance = GetInstance();

        instance->BuildFrame();
        if (instance->shadowGroups_.empty()) {
            return;
        }

        auto commandList = DirectXCommon::GetCommandList();
        // シャドウパス中はシャドウマップ自体をSRVとして結ばない (深度書き込み中のため)
        commandList->SetGraphicsRootSignature(instance->rootSignature_.Get());
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        // テクスチャ(ディスクリプタテーブル)を結ぶため、SRVのヒープを設定しておく
        SrvManager::PreDraw(commandList);
        commandList->SetGraphicsRootConstantBufferView(6, ShadowMap::GetLightViewProjGPUVirtualAddress());
        instance->DrawGroups(instance->shadowGroups_, false, true);
    }

    void ModelManager::PreparePipeline(uint32_t shader, BlendMode blendMode, bool isBackground) {
        ModelManager* instance = GetInstance();
        instance->GetPipelineState(shader, blendMode, isBackground);
        // 影を落とすブレンドモードなら、影用も作っておく
        if (!isBackground && (blendMode == BlendMode::None || blendMode == BlendMode::Normal || blendMode == BlendMode::Multiply)) {
            instance->GetShadowPipelineState(shader);
        }
    }

    // ------------------------------------------------------------------
    // 初期化 (ルートシグネチャ / PSO / バッファ)
    // ------------------------------------------------------------------
    void ModelManager::CreateBuffers() {
        auto device = DirectXCommon::GetDevice();

        cameraVPResource_ = DirectXCommon::CreateBufferResource(device, sizeof(ModelCameraData));
        cameraVPResource_->Map(0, nullptr, reinterpret_cast<void**>(&cameraVPData_));
        cameraVPData_->viewProjection = MakeIdentity4x4();
        cameraVPData_->position = { 0.0f, 0.0f, 0.0f };
        cameraVPData_->padding = 0.0f;

        EnsureInstanceCapacity(kInitialInstanceCapacity);
    }

    void ModelManager::CreateRootSignature() {
        auto device = DirectXCommon::GetDevice();
        HRESULT hr = S_OK;

        // テクスチャ (t0, Pixel)
        D3D12_DESCRIPTOR_RANGE textureRange[1] = {};
        textureRange[0].BaseShaderRegister = 0;
        textureRange[0].NumDescriptors = 1;
        textureRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        textureRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        // シャドウマップ (t2, Pixel)
        D3D12_DESCRIPTOR_RANGE shadowRange[1] = {};
        shadowRange[0].BaseShaderRegister = 2;
        shadowRange[0].NumDescriptors = 1;
        shadowRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        shadowRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        // シェーダーごとの追加テクスチャ (t4 / t5, Vertex/Pixel共通)
        D3D12_DESCRIPTOR_RANGE shaderTexture0Range[1] = {};
        shaderTexture0Range[0].BaseShaderRegister = 4;
        shaderTexture0Range[0].NumDescriptors = 1;
        shaderTexture0Range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        shaderTexture0Range[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
        D3D12_DESCRIPTOR_RANGE shaderTexture1Range[1] = {};
        shaderTexture1Range[0].BaseShaderRegister = 5;
        shaderTexture1Range[0].NumDescriptors = 1;
        shaderTexture1Range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        shaderTexture1Range[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_PARAMETER rootParameters[11] = {};
        // 0: カメラのView-Projection と位置 (CBV b0, VertexとPixelの両方。Pixelは鏡面反射で位置を使う)
        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters[0].Descriptor.ShaderRegister = 0;
        // 1: インスタンス配列 (Root SRV t3, VertexとPixelの両方から読む)
        rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters[1].Descriptor.ShaderRegister = 3;
        // 2: テクスチャ (Table t0, Vertex/Pixel共通。アプリの Vertex() からも読めるように)
        rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters[2].DescriptorTable.pDescriptorRanges = textureRange;
        rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(textureRange);
        // 3: Light配列 (Root SRV t1, Pixel)
        rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[3].Descriptor.ShaderRegister = 1;
        // 4: 有効ライト数 (CBV b1, Pixel)
        rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[4].Descriptor.ShaderRegister = 1;
        // 5: アンビエント (CBV b2, Pixel)
        rootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[5].Descriptor.ShaderRegister = 2;
        // 6: ライトのView-Projection (CBV b3, Vertex/Pixel共通)
        rootParameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters[6].Descriptor.ShaderRegister = 3;
        // 7: シャドウマップ (Table t2, Pixel)
        rootParameters[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[7].DescriptorTable.pDescriptorRanges = shadowRange;
        rootParameters[7].DescriptorTable.NumDescriptorRanges = _countof(shadowRange);
        // 8: 描画パスごとの値 (Root Constants b4, Vertex/Pixel共通) { blendMode, nearFadeStart, nearFadeEnd, time }
        rootParameters[8].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        rootParameters[8].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters[8].Constants.ShaderRegister = 4;
        rootParameters[8].Constants.RegisterSpace = 0;
        rootParameters[8].Constants.Num32BitValues = 4;
        // 9 / 10: シェーダーごとの追加テクスチャ (Table t4 / t5, Vertex/Pixel共通)
        rootParameters[9].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[9].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters[9].DescriptorTable.pDescriptorRanges = shaderTexture0Range;
        rootParameters[9].DescriptorTable.NumDescriptorRanges = _countof(shaderTexture0Range);
        rootParameters[10].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[10].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters[10].DescriptorTable.pDescriptorRanges = shaderTexture1Range;
        rootParameters[10].DescriptorTable.NumDescriptorRanges = _countof(shaderTexture1Range);

        D3D12_STATIC_SAMPLER_DESC staticSamplers[2] = {};
        // s0: 通常のテクスチャ用
        staticSamplers[0].Filter = D3D12_FILTER_ANISOTROPIC;  // 異方性フィルタ(斜めから見た地面などをくっきりさせる。ミップマップ前提)
        staticSamplers[0].MaxAnisotropy = 16;  // 1〜16。static samplerなので実行中には変更できない
        staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
        staticSamplers[0].ShaderRegister = 0;
        staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // アプリの Vertex() からも使えるように
        // s1: シャドウマップ比較用 (ハードウェアPCF)
        staticSamplers[1].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        staticSamplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        staticSamplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        staticSamplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        staticSamplers[1].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
        staticSamplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        staticSamplers[1].MaxLOD = D3D12_FLOAT32_MAX;
        staticSamplers[1].ShaderRegister = 1;
        staticSamplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC desc{};
        desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        desc.pParameters = rootParameters;
        desc.NumParameters = _countof(rootParameters);
        desc.pStaticSamplers = staticSamplers;
        desc.NumStaticSamplers = _countof(staticSamplers);

        Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
        hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
        if (FAILED(hr)) {
            Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
            assert(false);
        }
        hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
        assert(SUCCEEDED(hr));
    }

    void ModelManager::CreatePipelineStates() {
        // シェーダーのコンパイルは ShaderManager が行う (エンジン標準 = ShaderManager::kDefaultShader)。
        // PSOは GetPipelineState() / GetShadowPipelineState() で、初めて使われた組み合わせの分だけ作る。
        // ここでは一番よく使う「エンジン標準の不透明」「背景(不透明)」「影」だけ先に作っておく
        PreparePipeline(ShaderManager::kDefaultShader, BlendMode::None, false);
        PreparePipeline(ShaderManager::kDefaultShader, BlendMode::None, true);
    }

    ID3D12PipelineState* ModelManager::GetPipelineState(uint32_t shader, BlendMode blendMode, bool isBackground) {
        const uint64_t key = (static_cast<uint64_t>(shader) << 16) | (isBackground ? 0x100u : 0u) | static_cast<uint64_t>(blendMode);
        if (auto it = pipelineCache_.find(key); it != pipelineCache_.end()) {
            return it->second.Get();
        }

        auto device = DirectXCommon::GetDevice();
        const ShaderManager::ModelProgram& program = ShaderManager::GetModelProgram(shader);

        D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
        inputElementDescs[0].SemanticName = "POSITION";
        inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        inputElementDescs[0].AlignedByteOffset = static_cast<UINT>(offsetof(VertexData, position));
        inputElementDescs[1].SemanticName = "TEXCOORD";
        inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
        inputElementDescs[1].AlignedByteOffset = static_cast<UINT>(offsetof(VertexData, texcoord));
        inputElementDescs[2].SemanticName = "NORMAL";
        inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
        inputElementDescs[2].AlignedByteOffset = static_cast<UINT>(offsetof(VertexData, normal));

        D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
        inputLayoutDesc.pInputElementDescs = inputElementDescs;
        inputLayoutDesc.NumElements = _countof(inputElementDescs);

        D3D12_RASTERIZER_DESC rasterizerDesc{};
        rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
        rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

        // 深度テストは全部で行う。深度を書くのは不透明と半透明(Normal)だけ
        // (Normalは書かないと、立体的なモデルの奥の面が手前に出てしまうため。遠い順に描くので他の半透明を隠すことはほぼ無い)
        // (加算などが深度を書くと、後ろに描くエフェクトが隠れてしまうため書かない)
        // 背景は何も隠さないよう、ブレンドモードに関係なく書かない
        const bool depthWrite = !isBackground && (blendMode == BlendMode::None || blendMode == BlendMode::Normal);
        D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
        depthStencilDesc.DepthEnable = true;
        depthStencilDesc.DepthWriteMask = depthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
        depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
        psoDesc.pRootSignature = rootSignature_.Get();
        psoDesc.InputLayout = inputLayoutDesc;
        psoDesc.VS = { program.vertexShader.data(), program.vertexShader.size() };
        psoDesc.PS = { program.pixelShader.data(), program.pixelShader.size() };
        psoDesc.BlendState = CreateBlendDesc(blendMode);
        psoDesc.RasterizerState = rasterizerDesc;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.SampleDesc.Count = 1;
        psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
        psoDesc.DepthStencilState = depthStencilDesc;
        psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

        Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
        HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
        assert(SUCCEEDED(hr));

        ID3D12PipelineState* result = pipelineState.Get();
        pipelineCache_[key] = std::move(pipelineState);
        return result;
    }

    ID3D12PipelineState* ModelManager::GetShadowPipelineState(uint32_t shader) {
        if (auto it = shadowPipelineCache_.find(shader); it != shadowPipelineCache_.end()) {
            return it->second.Get();
        }

        auto device = DirectXCommon::GetDevice();
        const ShaderManager::ModelProgram& program = ShaderManager::GetModelProgram(shader);

        // アプリの Vertex() が UV や法線を使えるよう、位置以外も渡す
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
        inputElementDescs[0].SemanticName = "POSITION";
        inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        inputElementDescs[0].AlignedByteOffset = static_cast<UINT>(offsetof(VertexData, position));
        inputElementDescs[1].SemanticName = "TEXCOORD";
        inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
        inputElementDescs[1].AlignedByteOffset = static_cast<UINT>(offsetof(VertexData, texcoord));
        inputElementDescs[2].SemanticName = "NORMAL";
        inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
        inputElementDescs[2].AlignedByteOffset = static_cast<UINT>(offsetof(VertexData, normal));

        D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
        inputLayoutDesc.pInputElementDescs = inputElementDescs;
        inputLayoutDesc.NumElements = _countof(inputElementDescs);

        D3D12_RASTERIZER_DESC rasterizerDesc{};
        rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
        rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
        // シャドウアクネ対策の深度バイアス (ModelCommonのシャドウPSOと同じ値)
        rasterizerDesc.DepthBias = 50;
        rasterizerDesc.DepthBiasClamp = 0.0f;
        rasterizerDesc.SlopeScaledDepthBias = 1.5f;

        D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
        depthStencilDesc.DepthEnable = true;
        depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
        psoDesc.pRootSignature = rootSignature_.Get();
        psoDesc.InputLayout = inputLayoutDesc;
        psoDesc.VS = { program.shadowVertexShader.data(), program.shadowVertexShader.size() };
        psoDesc.RasterizerState = rasterizerDesc;
        psoDesc.NumRenderTargets = 0;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.SampleDesc.Count = 1;
        psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
        psoDesc.DepthStencilState = depthStencilDesc;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

        Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
        HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
        assert(SUCCEEDED(hr));

        ID3D12PipelineState* result = pipelineState.Get();
        shadowPipelineCache_[shader] = std::move(pipelineState);
        return result;
    }

    D3D12_BLEND_DESC ModelManager::CreateBlendDesc(BlendMode blendMode) const {
        D3D12_BLEND_DESC blendDesc{};
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

        switch (blendMode) {
        case BlendMode::None:
            blendDesc.RenderTarget[0].BlendEnable = FALSE;
            blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
            blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
            blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
            break;
        case BlendMode::Normal:
            blendDesc.RenderTarget[0].BlendEnable = TRUE;
            blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
            blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
            blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
            break;
        case BlendMode::Add:
            blendDesc.RenderTarget[0].BlendEnable = TRUE;
            blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
            blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
            blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
            break;
        case BlendMode::Subtract:
            blendDesc.RenderTarget[0].BlendEnable = TRUE;
            blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
            blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
            blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
            break;
        case BlendMode::Multiply:
            blendDesc.RenderTarget[0].BlendEnable = TRUE;
            blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ZERO;
            blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR;
            blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
            break;
        case BlendMode::Screen:
            blendDesc.RenderTarget[0].BlendEnable = TRUE;
            blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
            blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
            blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
            break;
        }
        return blendDesc;
    }

    // ------------------------------------------------------------------
    // アニメーション
    // ------------------------------------------------------------------
    void ModelManager::RegisterDebugParams() {
        ModelManager* instance = GetInstance();
        ParamEditor::BeginGroup("Model");
        ParamEditor::RegisterFlag("骨格を表示", &instance->showSkeleton_);
        ParamEditor::EndGroup();
    }

    void ModelManager::SetupSkeletalAsset(ModelAsset& asset, std::vector<AnimationData>&& animations) {
        for (size_t i = 0; i < animations.size(); ++i) {
            // 名前が無いアニメーションは "Animation" (重複すると _1, _2 ... が付く)
            const std::string name = animations[i].name.empty() ? std::string("Animation") : animations[i].name;
            AddAnimation(asset, std::move(animations[i]), name, nullptr, asset.filePath);
        }
    }

    uint32_t ModelManager::LoadAnimation(uint32_t modelIndex, const std::string& filePath, const std::string& name) {
        ModelManager* instance = GetInstance();
        if (modelIndex == kInvalidModelIndex || modelIndex > instance->assets_.size()) {
            Logger::LogWarning("[ModelManager] アニメーションを足す先のモデルがありません: {}\n", filePath);
            return 0;
        }
        ModelAsset& asset = *instance->assets_[modelIndex - 1];
        if (!asset.hasSkeleton) {
            Logger::LogWarning("[ModelManager] 骨格の無いモデルには、アニメーションを足せません: {} ← {}\n", asset.filePath, filePath);
            return 0;
        }

        // 同じファイルは1回だけ読む (同じモデルを使う全員の初期化で呼ばれても、登録が増えないように)
        if (!asset.loadedAnimationFiles.insert(filePath).second) {
            return 0;
        }

        SkeletonData sourceSkeleton;
        std::vector<AnimationData> animations = ModelLoader::LoadAnimationFile(filePath, sourceSkeleton);
        if (animations.empty()) {
            Logger::LogWarning("[ModelManager] ファイルにアニメーションが入っていません: {}\n", filePath);
            return 0;
        }

        // ファイル名 (フォルダと拡張子を除いたもの)
        std::string stem = filePath;
        if (const size_t slash = stem.find_last_of("/\\"); slash != std::string::npos) {
            stem = stem.substr(slash + 1);
        }
        if (const size_t dot = stem.find_last_of('.'); dot != std::string::npos) {
            stem = stem.substr(0, dot);
        }

        for (size_t i = 0; i < animations.size(); ++i) {
            std::string animationName;
            if (animations.size() == 1) {
                // 1つだけなら、指定した名前 (無ければファイル名)
                animationName = name.empty() ? stem : name;
            } else {
                // 複数なら、それぞれの名前 (指定した名前があれば "指定した名前/アニメーションの名前")
                const std::string ownName = animations[i].name.empty() ? stem + "_" + std::to_string(i) : animations[i].name;
                animationName = name.empty() ? ownName : name + "/" + ownName;
            }
            AddAnimation(asset, std::move(animations[i]), animationName, &sourceSkeleton, filePath);
        }
        return static_cast<uint32_t>(animations.size());
    }

    void ModelManager::AddAnimation(ModelAsset& asset, AnimationData&& animation, const std::string& name,
        const SkeletonData* sourceSkeleton, const std::string& filePath) {
        SkeletalAnimation result;
        result.data = std::make_unique<AnimationData>(std::move(animation));
        result.duration = result.data->duration;

        // 名前が重複したら _1, _2 ... を付ける
        result.name = name;
        for (int suffix = 1; asset.animationIndex.count(result.name) != 0; ++suffix) {
            result.name = name + "_" + std::to_string(suffix);
        }

        const SkeletonData& skeleton = asset.skeleton;

        // 別ファイルのアニメーションが、骨の長さの違うモデル用かどうか
        // (対応するボーンの元の位置が全部ほぼ同じなら、同じ骨格とみなして全部の動きを使う)
        bool retarget = false;
        if (sourceSkeleton && !sourceSkeleton->joints.empty()) {
            for (const auto& [nodeName, nodeAnimation] : result.data->nodeAnimations) {
                const int32_t joint = skeleton.FindJoint(nodeName);
                const int32_t sourceJoint = sourceSkeleton->FindJoint(nodeName);
                if (joint < 0 || sourceJoint < 0) continue;
                const Vector3& own = skeleton.joints[joint].translate;
                const Vector3 diff = own - sourceSkeleton->joints[sourceJoint].translate;
                const float tolerance = 1e-3f * (1.0f + Length(own));
                if (Length(diff) > tolerance) {
                    retarget = true;
                    break;
                }
            }
        }

        // ボーンの深さ (一番上の動くボーン = 腰 を見つけるため)
        auto depthOf = [&skeleton](int32_t joint) {
            int depth = 0;
            for (int32_t j = skeleton.joints[joint].parent; j >= 0; j = skeleton.joints[j].parent) {
                ++depth;
            }
            return depth;
        };

        std::vector<std::string> missing;
        int rootDepth = 0;
        size_t rootChannel = 0;
        for (const auto& [nodeName, nodeAnimation] : result.data->nodeAnimations) {
            const int32_t joint = skeleton.FindJoint(nodeName);
            if (joint < 0) {
                missing.push_back(nodeName);
                continue;
            }
            AnimationChannelBinding binding;
            binding.jointIndex = joint;
            binding.animation = &nodeAnimation;
            const int depth = depthOf(joint);
            if (result.channels.empty() || depth < rootDepth) {
                rootDepth = depth;
                rootChannel = result.channels.size();
            }
            result.channels.push_back(binding);
        }

        if (retarget && !result.channels.empty()) {
            // 骨の長さの違うモデル用: 腰以外は回転だけ使い、骨の長さはこのモデルのままにする。
            // 腰の移動量は、元のモデルと腰の高さ(親からの距離)の比で縮める
            for (size_t c = 0; c < result.channels.size(); ++c) {
                AnimationChannelBinding& binding = result.channels[c];
                binding.useScale = false;
                binding.useTranslate = (c == rootChannel);
            }
            AnimationChannelBinding& root = result.channels[rootChannel];
            const std::string& rootName = skeleton.joints[root.jointIndex].name;
            const int32_t sourceRoot = sourceSkeleton->FindJoint(rootName);
            if (sourceRoot >= 0) {
                const float sourceLength = Length(sourceSkeleton->joints[sourceRoot].translate);
                const float ownLength = Length(skeleton.joints[root.jointIndex].translate);
                if (sourceLength > 1e-6f) {
                    root.translateScale = ownLength / sourceLength;
                }
            }
            Logger::Log("[ModelManager] \"{}\" は骨の長さが違うモデル用なので、回転だけを使います (腰: {}、移動量 x{:.2f})\n",
                result.name, rootName, root.translateScale);
        }

        if (!missing.empty()) {
            Logger::LogWarning("[ModelManager] アニメーション \"{}\" の {} 本の動きが、モデルのボーンに見つかりませんでした (例: {}): {}\n",
                result.name, missing.size(), missing.front(), filePath);
        }
        Logger::Log("[ModelManager] アニメーション \"{}\" ({:.2f}秒) を登録しました: {}\n", result.name, result.duration, asset.filePath);

        asset.animationIndex[result.name] = static_cast<uint32_t>(asset.animations.size());
        asset.animations.push_back(std::move(result));
    }
}

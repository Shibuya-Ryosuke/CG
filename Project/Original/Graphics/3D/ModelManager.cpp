#include "ModelManager.h"
#include "../../Core/Base/DirectXCommon.h"
#include "../../Core/Base/Logger.h"
#include "../../Core/Base/ShaderCompiler.h"
#include "../../Core/Base/SrvManager.h"
#include "../2D/TextureManager.h"
#include "../Light/LightManager.h"
#include "../Shadow/ShadowMap.h"
#include "../../Core/Loader/ModelLoader.h"
#include "../../Editor/ModelPicker.h"
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

        instance->shadowPipelineState_.Reset();
        instance->pipelineCache_.clear();
        instance->vertexShaderBytecode_.clear();
        instance->pixelShaderBytecode_.clear();
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

        for (const auto& src : modelData.meshes) {
            MeshAsset mesh;

            // 頂点バッファ (インデックスバッファは使わない、三角形展開済みの頂点列)
            const size_t bytes = sizeof(VertexData) * src.vertices.size();
            mesh.vertexResource = DirectXCommon::CreateBufferResource(device, bytes);
            mesh.vertexBufferView.BufferLocation = mesh.vertexResource->GetGPUVirtualAddress();
            mesh.vertexBufferView.SizeInBytes = static_cast<UINT>(bytes);
            mesh.vertexBufferView.StrideInBytes = sizeof(VertexData);

            VertexData* mapped = nullptr;
            mesh.vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
            std::memcpy(mapped, src.vertices.data(), bytes);
            mesh.vertexResource->Unmap(0, nullptr);
            mesh.vertexCount = static_cast<uint32_t>(src.vertices.size());

            // フラスタムカリング用: 原点から一番遠い頂点までの距離
            for (const auto& vertex : src.vertices) {
                const Vector3 position = { vertex.position.x, vertex.position.y, vertex.position.z };
                const float distance = Length(position);
                if (distance > asset->boundingRadius) {
                    asset->boundingRadius = distance;
                }
            }

            mesh.hasUV = src.hasUV;

            // マテリアル名とデフォルトテクスチャ
            std::string texturePath;
            if (src.materialIndex < modelData.materials.size()) {
                mesh.materialName = modelData.materials[src.materialIndex].name;
                texturePath = modelData.materials[src.materialIndex].textureFilePath;
            }
            // UVが無いメッシュはテクスチャを引けない(texcoordは(0,0)固定)ので、必ず white にする
            if (mesh.hasUV && !texturePath.empty()) {
                mesh.defaultTextureHandle = TextureManager::Load(texturePath);
            } else {
                mesh.defaultTextureHandle = TextureManager::GetWhiteTex();
            }

            asset->meshes.push_back(std::move(mesh));
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
    }

    void ModelManager::Submit(uint32_t modelIndex, uint32_t meshIndex, uint32_t textureHandle,
        BlendMode blendMode, bool castShadow, bool isBackground, const ModelInstanceData& data) {
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
        entry.blendMode = blendMode;
        entry.isBackground = isBackground;
        // 加算・減算・スクリーンは光るエフェクト向けなので、影を落とさない。背景も影を落とさない
        entry.castShadow = castShadow && !isBackground &&
            (blendMode == BlendMode::None || blendMode == BlendMode::Normal || blendMode == BlendMode::Multiply);

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
        *cameraVPData_ = viewProjection_;

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

        // 同じキーの中は提出順を保つ (dataIndexで比べる)
        auto byKey = [](const Entry* a, const Entry* b) {
            if (a->key != b->key) return a->key < b->key;
            return a->dataIndex < b->dataIndex;
        };
        // 不透明・影: (モデル, メッシュ, テクスチャ) の順に並べてまとめる
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

            // キーかブレンドモードが変わったら新しいグループ
            if (!previous || previous->key != entry->key || previous->blendMode != entry->blendMode) {
                Group group;
                group.modelIndex = static_cast<uint32_t>(entry->key >> 48);
                group.meshIndex = static_cast<uint32_t>((entry->key >> 32) & 0xFFFFu);
                group.textureHandle = static_cast<uint32_t>(entry->key & 0xFFFFFFFFu);
                group.blendMode = entry->blendMode;
                group.firstInstance = static_cast<uint32_t>(writeIndex);
                group.instanceCount = 0;
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
        // root 8: b4 { uint blendMode; float nearFadeStart; float nearFadeEnd; }
        struct DrawParams {
            uint32_t blendMode;
            float nearFadeStart;
            float nearFadeEnd;
        } params{ static_cast<uint32_t>(blendMode), nearFadeStart_, nearFadeEnd_ };
        commandList->SetGraphicsRoot32BitConstants(8, 3, &params, 0);
    }

    void ModelManager::DrawGroups(const std::vector<Group>& groups, bool bindTexture) {
        auto commandList = DirectXCommon::GetCommandList();
        const D3D12_GPU_VIRTUAL_ADDRESS base = instanceResource_->GetGPUVirtualAddress();

        for (const Group& group : groups) {
            const MeshAsset& mesh = assets_[group.modelIndex - 1]->meshes[group.meshIndex];

            if (bindTexture) {
                commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetGPUHandle(group.textureHandle));
            }
            commandList->IASetVertexBuffers(0, 1, &mesh.vertexBufferView);

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

        auto commandList = DirectXCommon::GetCommandList();
        instance->BindCommonState();

        // 0. 背景 (深度を書かないので、後から描くものは必ず背景より手前に見える)
        {
            const std::vector<Group>& groups = instance->backgroundGroups_;
            size_t begin = 0;
            while (begin < groups.size()) {
                const BlendMode blendMode = groups[begin].blendMode;
                size_t end = begin;
                while (end < groups.size() && groups[end].blendMode == blendMode) {
                    ++end;
                }
                commandList->SetPipelineState(instance->GetPipelineState(blendMode, true));
                instance->SetDrawParams(blendMode);
                instance->DrawGroups(std::vector<Group>(groups.begin() + begin, groups.begin() + end), true);
                begin = end;
            }
        }

        // 1. 不透明 (深度を書く)
        if (!instance->opaqueGroups_.empty()) {
            commandList->SetPipelineState(instance->GetPipelineState(BlendMode::None));
            instance->SetDrawParams(BlendMode::None);
            instance->DrawGroups(instance->opaqueGroups_, true);
        }

        // 2. 半透明 (遠い順に並んでいる。自分の奥の面が手前に出ないよう深度は書く)
        if (!instance->sortedGroups_.empty()) {
            commandList->SetPipelineState(instance->GetPipelineState(BlendMode::Normal));
            instance->SetDrawParams(BlendMode::Normal);
            instance->DrawGroups(instance->sortedGroups_, true);
        }

        // 3. 加算・減算・乗算・スクリーン (ブレンドモードごとに並んでいるので、変わったところだけPSOを切り替える)
        const std::vector<Group>& groups = instance->commutativeGroups_;
        size_t begin = 0;
        while (begin < groups.size()) {
            const BlendMode blendMode = groups[begin].blendMode;
            size_t end = begin;
            while (end < groups.size() && groups[end].blendMode == blendMode) {
                ++end;
            }
            commandList->SetPipelineState(instance->GetPipelineState(blendMode));
            instance->SetDrawParams(blendMode);
            instance->DrawGroups(std::vector<Group>(groups.begin() + begin, groups.begin() + end), true);
            begin = end;
        }
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
        commandList->SetGraphicsRootConstantBufferView(6, ShadowMap::GetLightViewProjGPUVirtualAddress());
        commandList->SetPipelineState(instance->shadowPipelineState_.Get());
        instance->DrawGroups(instance->shadowGroups_, false);
    }

    // ------------------------------------------------------------------
    // 初期化 (ルートシグネチャ / PSO / バッファ)
    // ------------------------------------------------------------------
    void ModelManager::CreateBuffers() {
        auto device = DirectXCommon::GetDevice();

        cameraVPResource_ = DirectXCommon::CreateBufferResource(device, sizeof(Matrix4x4));
        cameraVPResource_->Map(0, nullptr, reinterpret_cast<void**>(&cameraVPData_));
        *cameraVPData_ = MakeIdentity4x4();

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

        D3D12_ROOT_PARAMETER rootParameters[9] = {};
        // 0: カメラのView-Projection (CBV b0, Vertex)
        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        rootParameters[0].Descriptor.ShaderRegister = 0;
        // 1: インスタンス配列 (Root SRV t3, VertexとPixelの両方から読む)
        rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters[1].Descriptor.ShaderRegister = 3;
        // 2: テクスチャ (Table t0, Pixel)
        rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
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
        // 8: 描画パスごとの値 (Root Constants b4, Pixel) { blendMode, nearFadeStart, nearFadeEnd }
        rootParameters[8].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        rootParameters[8].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[8].Constants.ShaderRegister = 4;
        rootParameters[8].Constants.RegisterSpace = 0;
        rootParameters[8].Constants.Num32BitValues = 3;

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
        staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
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
        auto device = DirectXCommon::GetDevice();
        HRESULT hr = S_OK;

        D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
        depthStencilDesc.DepthEnable = true;
        depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

        // ---------- 通常描画 ----------
        // PSOは GetPipelineState() で、初めて使われたブレンドモードの分だけ作る。ここではシェーダーのコンパイルだけ行う
        {
            Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = ShaderCompiler::Compile(L"Resources/EngineResources/HLSL/Model/ModelBatch.VS.hlsl", L"vs_6_0");
            Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = ShaderCompiler::Compile(L"Resources/EngineResources/HLSL/Model/ModelBatch.PS.hlsl", L"ps_6_0");
            assert(vertexShaderBlob != nullptr);
            assert(pixelShaderBlob != nullptr);

            const uint8_t* vs = static_cast<const uint8_t*>(vertexShaderBlob->GetBufferPointer());
            vertexShaderBytecode_.assign(vs, vs + vertexShaderBlob->GetBufferSize());
            const uint8_t* ps = static_cast<const uint8_t*>(pixelShaderBlob->GetBufferPointer());
            pixelShaderBytecode_.assign(ps, ps + pixelShaderBlob->GetBufferSize());

            // 一番使う不透明だけは最初に作っておく
            GetPipelineState(BlendMode::None);
        }

        // ---------- シャドウ (深度のみ) ----------
        {
            D3D12_INPUT_ELEMENT_DESC inputElementDescs[1] = {};
            inputElementDescs[0].SemanticName = "POSITION";
            inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            inputElementDescs[0].AlignedByteOffset = static_cast<UINT>(offsetof(VertexData, position));

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

            Microsoft::WRL::ComPtr<IDxcBlob> shadowVSBlob = ShaderCompiler::Compile(L"Resources/EngineResources/HLSL/Model/ModelBatchShadow.VS.hlsl", L"vs_6_0");
            assert(shadowVSBlob != nullptr);

            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
            psoDesc.pRootSignature = rootSignature_.Get();
            psoDesc.InputLayout = inputLayoutDesc;
            psoDesc.VS = { shadowVSBlob->GetBufferPointer(), shadowVSBlob->GetBufferSize() };
            psoDesc.RasterizerState = rasterizerDesc;
            psoDesc.NumRenderTargets = 0;
            psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            psoDesc.SampleDesc.Count = 1;
            psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
            psoDesc.DepthStencilState = depthStencilDesc;
            psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

            hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&shadowPipelineState_));
            assert(SUCCEEDED(hr));
        }
    }

    ID3D12PipelineState* ModelManager::GetPipelineState(BlendMode blendMode, bool isBackground) {
        const uint32_t key = static_cast<uint32_t>(blendMode) | (isBackground ? 0x100u : 0u);
        if (auto it = pipelineCache_.find(key); it != pipelineCache_.end()) {
            return it->second.Get();
        }

        auto device = DirectXCommon::GetDevice();

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
        psoDesc.VS = { vertexShaderBytecode_.data(), vertexShaderBytecode_.size() };
        psoDesc.PS = { pixelShaderBytecode_.data(), pixelShaderBytecode_.size() };
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
}

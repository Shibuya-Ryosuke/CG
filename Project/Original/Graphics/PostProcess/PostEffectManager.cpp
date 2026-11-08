#include "PostEffectManager.h"
#include "../../Core/Base/DirectXCommon.h"
#include "../../Core/Base/Logger.h"
#include "../../Core/Base/SrvManager.h"
#include "../../Core/Base/TimeManager.h"
#include "../2D/TextureManager.h"
#include "../Shader/ShaderCompiler.h"
#include "../Shader/ShaderManager.h"
#include <algorithm>
#include <cassert>

namespace RyoEngine {

    PostEffectManager* PostEffectManager::GetInstance() {
        static PostEffectManager instance;
        return &instance;
    }

    void PostEffectManager::Initialize() {
        PostEffectManager* instance = GetInstance();

        Logger::Log("PostEffectManager : Initializing...\n");

        // 頂点シェーダーは PostProcess と同じ「画面全体を覆う三角形」を使う
        Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = ShaderCompiler::Compile(L"Resources/EngineResources/HLSL/PostProcess/Composite.VS.hlsl", L"vs_6_0");
        assert(vertexShaderBlob != nullptr);
        const uint8_t* vs = static_cast<const uint8_t*>(vertexShaderBlob->GetBufferPointer());
        instance->vertexShaderBytecode_.assign(vs, vs + vertexShaderBlob->GetBufferSize());

        instance->CreateRootSignature();
        instance->CreateWorkTextures();
        instance->shaderReloadCount_ = ShaderManager::GetReloadCount();

        Logger::LogSuccess("PostEffectManager : Initialized\n");
    }

    void PostEffectManager::Finalize() {
        PostEffectManager* instance = GetInstance();

        Logger::Log("PostEffectManager : Finalizing...\n");
        instance->effects_.clear();
        instance->pipelineCache_.clear();
        instance->retiredPipelines_.clear();
        for (WorkTexture& texture : instance->workTextures_) {
            texture.resource.Reset();
        }
        instance->rtvHeap_.Reset();
        instance->rootSignature_.Reset();
        instance->vertexShaderBytecode_.clear();
        Logger::LogSuccess("PostEffectManager : Finalized\n");
    }

    // ------------------------------------------------------------------
    // 登録・設定
    // ------------------------------------------------------------------
    uint32_t PostEffectManager::Add(uint32_t shader) {
        PostEffectManager* instance = GetInstance();

        Effect effect;
        effect.id = instance->nextId_++;
        effect.shader = shader;
        instance->effects_.push_back(effect);

        // 描く瞬間に作るとカクつくことがあるので、PSOは登録したときに作っておく
        instance->GetPipelineState(shader);
        return effect.id;
    }

    void PostEffectManager::Remove(uint32_t effect) {
        PostEffectManager* instance = GetInstance();
        instance->effects_.erase(
            std::remove_if(instance->effects_.begin(), instance->effects_.end(),
                [effect](const Effect& e) { return e.id == effect; }),
            instance->effects_.end());
    }

    void PostEffectManager::Clear() {
        GetInstance()->effects_.clear();
    }

    PostEffectManager::Effect* PostEffectManager::Find(uint32_t effect) {
        for (Effect& e : effects_) {
            if (e.id == effect) {
                return &e;
            }
        }
        Logger::LogWarning("[PostEffectManager] 登録されていない効果です: {}\n", effect);
        return nullptr;
    }

    void PostEffectManager::SetEnabled(uint32_t effect, bool enabled) {
        if (Effect* e = GetInstance()->Find(effect)) {
            e->enabled = enabled;
        }
    }

    bool PostEffectManager::IsEnabled(uint32_t effect) {
        const Effect* e = GetInstance()->Find(effect);
        return e && e->enabled;
    }

    void PostEffectManager::SetParam(uint32_t effect, uint32_t index, float value) {
        Effect* e = GetInstance()->Find(effect);
        if (!e || index >= 8) {
            return;
        }
        Vector4& slot = e->params[index / 4];
        switch (index % 4) {
        case 0: slot.x = value; break;
        case 1: slot.y = value; break;
        case 2: slot.z = value; break;
        default: slot.w = value; break;
        }
    }

    void PostEffectManager::SetParams(uint32_t effect, uint32_t slot, const Vector4& values) {
        Effect* e = GetInstance()->Find(effect);
        if (!e || slot >= 2) {
            return;
        }
        e->params[slot] = values;
    }

    float PostEffectManager::GetParam(uint32_t effect, uint32_t index) {
        const Effect* e = GetInstance()->Find(effect);
        if (!e || index >= 8) {
            return 0.0f;
        }
        const Vector4& slot = e->params[index / 4];
        switch (index % 4) {
        case 0: return slot.x;
        case 1: return slot.y;
        case 2: return slot.z;
        default: return slot.w;
        }
    }

    // ------------------------------------------------------------------
    // 毎フレーム・描画
    // ------------------------------------------------------------------
    void PostEffectManager::NewFrame() {
        PostEffectManager* instance = GetInstance();

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

    void PostEffectManager::CollectActiveEffects(std::vector<const Effect*>& outEffects) {
        outEffects.clear();
        for (const Effect& effect : effects_) {
            // オフのもの・シェーダーが使えないもの(読み込み失敗・コンパイル失敗)は飛ばす
            if (effect.enabled && GetPipelineState(effect.shader)) {
                outEffects.push_back(&effect);
            }
        }
    }

    D3D12_CPU_DESCRIPTOR_HANDLE PostEffectManager::BeginComposite() {
        PostEffectManager* instance = GetInstance();

        std::vector<const Effect*> active;
        instance->CollectActiveEffects(active);
        instance->compositeToWork_ = !active.empty();
        if (!instance->compositeToWork_) {
            // 効果が無ければ、今まで通りゲーム画面へ直接書く
            return DirectXCommon::GetGameRenderTargetRTVHandle();
        }

        // 効果があれば、作業用テクスチャ(0番)へ書いてもらう
        instance->Transition(instance->workTextures_[0], D3D12_RESOURCE_STATE_RENDER_TARGET);
        return instance->workTextures_[0].rtv;
    }

    void PostEffectManager::Draw() {
        PostEffectManager* instance = GetInstance();
        if (!instance->compositeToWork_) {
            return;
        }
        instance->compositeToWork_ = false;

        std::vector<const Effect*> active;
        instance->CollectActiveEffects(active);
        // BeginComposite の後で効果が無くなることは無いはずだが、念のため
        assert(!active.empty());
        if (active.empty()) {
            return;
        }

        auto commandList = DirectXCommon::GetCommandList();
        const float width = static_cast<float>(DirectXCommon::GetGameWidth());
        const float height = static_cast<float>(DirectXCommon::GetGameHeight());

        commandList->SetGraphicsRootSignature(instance->rootSignature_.Get());
        SrvManager::PreDraw(commandList);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        D3D12_VIEWPORT viewport{};
        viewport.Width = width;
        viewport.Height = height;
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        commandList->RSSetViewports(1, &viewport);
        D3D12_RECT scissorRect{};
        scissorRect.right = static_cast<LONG>(width);
        scissorRect.bottom = static_cast<LONG>(height);
        commandList->RSSetScissorRects(1, &scissorRect);

        // 作業用テクスチャ 0 → 1 → 0 … と交互に読み書きし、最後の効果だけゲーム画面へ書く
        size_t source = 0;
        for (size_t i = 0; i < active.size(); ++i) {
            const Effect& effect = *active[i];
            const bool isLast = (i + 1 == active.size());

            WorkTexture& input = instance->workTextures_[source];
            instance->Transition(input, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

            D3D12_CPU_DESCRIPTOR_HANDLE outputRtv = DirectXCommon::GetGameRenderTargetRTVHandle();
            if (!isLast) {
                WorkTexture& output = instance->workTextures_[1 - source];
                instance->Transition(output, D3D12_RESOURCE_STATE_RENDER_TARGET);
                outputRtv = output.rtv;
            }
            commandList->OMSetRenderTargets(1, &outputRtv, false, nullptr);

            commandList->SetPipelineState(instance->GetPipelineState(effect.shader));
            // root 0: 1つ前までの画面 / 1, 2: シェーダーごとの追加テクスチャ
            commandList->SetGraphicsRootDescriptorTable(0, TextureManager::GetGPUHandle(input.textureHandle));
            commandList->SetGraphicsRootDescriptorTable(1, TextureManager::GetGPUHandle(ShaderManager::GetTexture(effect.shader, 0)));
            commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetGPUHandle(ShaderManager::GetTexture(effect.shader, 1)));
            // root 3: b0 { float4 params[2]; float2 screenSize; float time; float padding; } (PostEffectCommon.hlsli と合わせる)
            struct Constants {
                Vector4 params[2];
                float screenWidth;
                float screenHeight;
                float time;
                float padding;
            } constants{ { effect.params[0], effect.params[1] }, width, height, instance->time_, 0.0f };
            static_assert(sizeof(Constants) == 48, "PostEffectConstants は12個の32bit値であること");
            commandList->SetGraphicsRoot32BitConstants(3, 12, &constants, 0);

            commandList->DrawInstanced(3, 1, 0, 0);
            source = 1 - source;
        }
        // ここでゲーム画面がレンダーターゲットのままなので、このあと2Dがそのまま上に描かれる
    }

    // ------------------------------------------------------------------
    // PSO
    // ------------------------------------------------------------------
    ID3D12PipelineState* PostEffectManager::GetPipelineState(uint32_t shader) {
        if (auto it = pipelineCache_.find(shader); it != pipelineCache_.end()) {
            return it->second.Get();
        }

        const std::vector<uint8_t>* pixelShader = ShaderManager::GetPostEffectPixelShader(shader);
        if (!pixelShader) {
            return nullptr; // ポストエフェクト用でない・コンパイル失敗 (その効果は飛ばす)
        }

        D3D12_BLEND_DESC blendDesc{};
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        D3D12_RASTERIZER_DESC rasterizerDesc{};
        rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
        rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
        psoDesc.pRootSignature = rootSignature_.Get();
        psoDesc.InputLayout = { nullptr, 0 };
        psoDesc.VS = { vertexShaderBytecode_.data(), vertexShaderBytecode_.size() };
        psoDesc.PS = { pixelShader->data(), pixelShader->size() };
        psoDesc.BlendState = blendDesc;
        psoDesc.RasterizerState = rasterizerDesc;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DirectXCommon::GetBackBufferFormat(); // ゲーム画面と同じフォーマット
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.SampleDesc.Count = 1;
        psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
        psoDesc.DepthStencilState.DepthEnable = false;
        psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;

        Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
        HRESULT hr = DirectXCommon::GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
        assert(SUCCEEDED(hr));

        ID3D12PipelineState* result = pipelineState.Get();
        pipelineCache_[shader] = std::move(pipelineState);
        return result;
    }

    void PostEffectManager::RetirePipelinesIfShaderReloaded() {
        const uint32_t reloadCount = ShaderManager::GetReloadCount();
        if (reloadCount == shaderReloadCount_) {
            return;
        }
        shaderReloadCount_ = reloadCount;

        for (auto& [shader, pipelineState] : pipelineCache_) {
            retiredPipelines_.push_back({ std::move(pipelineState), retireFrames_ });
        }
        pipelineCache_.clear();
    }

    // ------------------------------------------------------------------
    // 初期化
    // ------------------------------------------------------------------
    void PostEffectManager::CreateRootSignature() {
        auto device = DirectXCommon::GetDevice();

        // t0: 1つ前までの画面 / t1, t2: シェーダーごとの追加テクスチャ
        D3D12_DESCRIPTOR_RANGE ranges[3] = {};
        for (UINT i = 0; i < 3; ++i) {
            ranges[i].BaseShaderRegister = i;
            ranges[i].NumDescriptors = 1;
            ranges[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            ranges[i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
        }

        D3D12_ROOT_PARAMETER rootParameters[4] = {};
        for (UINT i = 0; i < 3; ++i) {
            rootParameters[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            rootParameters[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
            rootParameters[i].DescriptorTable.pDescriptorRanges = &ranges[i];
            rootParameters[i].DescriptorTable.NumDescriptorRanges = 1;
        }
        // 3: b0 { float4 params[2]; float2 screenSize; float time; float padding; } (Root Constants 12個)
        rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[3].Constants.ShaderRegister = 0;
        rootParameters[3].Constants.RegisterSpace = 0;
        rootParameters[3].Constants.Num32BitValues = 12;

        D3D12_STATIC_SAMPLER_DESC samplers[2] = {};
        // s0: 画面用 (端で折り返さない)
        samplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        samplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        samplers[0].MaxLOD = D3D12_FLOAT32_MAX;
        samplers[0].ShaderRegister = 0;
        samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        // s1: 追加テクスチャ用 (タイル状に繰り返す)
        samplers[1] = samplers[0];
        samplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplers[1].ShaderRegister = 1;

        D3D12_ROOT_SIGNATURE_DESC desc{};
        desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE; // 頂点バッファ不要
        desc.pParameters = rootParameters;
        desc.NumParameters = _countof(rootParameters);
        desc.pStaticSamplers = samplers;
        desc.NumStaticSamplers = _countof(samplers);

        Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
        HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
        if (FAILED(hr)) {
            Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
            assert(false);
        }
        hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
        assert(SUCCEEDED(hr));
    }

    void PostEffectManager::CreateWorkTextures() {
        auto device = DirectXCommon::GetDevice();

        // ゲーム画面と同じ大きさ・フォーマット
        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Width = static_cast<UINT>(DirectXCommon::GetGameWidth());
        resourceDesc.Height = static_cast<UINT>(DirectXCommon::GetGameHeight());
        resourceDesc.MipLevels = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.Format = DirectXCommon::GetBackBufferFormat();
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        D3D12_HEAP_PROPERTIES heapProperties{};
        heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_CLEAR_VALUE clearValue{};
        clearValue.Format = resourceDesc.Format;

        rtvHeap_ = DirectXCommon::CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, static_cast<UINT>(workTextures_.size()), false);
        const UINT rtvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        for (size_t i = 0; i < workTextures_.size(); ++i) {
            WorkTexture& texture = workTextures_[i];
            HRESULT hr = device->CreateCommittedResource(
                &heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue, IID_PPV_ARGS(&texture.resource));
            assert(SUCCEEDED(hr));
            texture.state = D3D12_RESOURCE_STATE_RENDER_TARGET;

            texture.rtv = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
            texture.rtv.ptr += rtvSize * i;
            device->CreateRenderTargetView(texture.resource.Get(), nullptr, texture.rtv);

            texture.textureHandle = TextureManager::RegisterResource(texture.resource);
        }
    }

    void PostEffectManager::Transition(WorkTexture& texture, D3D12_RESOURCE_STATES newState) {
        if (texture.state == newState) {
            return;
        }
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = texture.resource.Get();
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = texture.state;
        barrier.Transition.StateAfter = newState;
        DirectXCommon::GetCommandList()->ResourceBarrier(1, &barrier);
        texture.state = newState;
    }
}

#include "PostProcess.h"
#include "../../Core/Base/DirectXCommon.h"
#include "../../Core/Base/Logger.h"
#include "../../Core/Base/ShaderCompiler.h"
#include "../../Core/Base/SrvManager.h"
#include "../../Graphics/2D/TextureManager.h"
#include "../../Externals/imgui/imgui.h"
#include <cassert>
#include <algorithm>

namespace RyoEngine {

    PostProcess* PostProcess::GetInstance() {
        static PostProcess instance;
        return &instance;
    }

    void PostProcess::Initialize() {
        PostProcess* instance = GetInstance();

        Logger::Log("PostProcess : Initializing...\n");

        instance->CreateSceneColorResource();
        instance->CreateRootSignatureAndPSO();
        instance->CreateBloomLevels();
        instance->CreateBloomPipelineStates();

        auto device = DirectXCommon::GetDevice();
        instance->compositeParamsResource_ = DirectXCommon::CreateBufferResource(device, sizeof(PostProcessParams));
        instance->compositeParamsResource_->Map(0, nullptr, reinterpret_cast<void**>(&instance->compositeParamsData_));
        instance->compositeParamsData_->acesEnabled = instance->acesEnabled_ ? 1 : 0;
        instance->compositeParamsData_->bloomEnabled = instance->bloomEnabled_ ? 1 : 0;
        instance->compositeParamsData_->exposure = instance->exposure_;
        instance->compositeParamsData_->threshold = instance->bloomThreshold_;
        instance->compositeParamsData_->bloomIntensity = instance->bloomIntensity_;
        instance->compositeParamsData_->distortionEnabled = instance->distortionEnabled_ ? 1 : 0;
        instance->compositeParamsData_->distortionStrength = instance->distortionStrength_;
        instance->compositeParamsData_->glitchEnabled = instance->glitchEnabled_ ? 1 : 0;
        instance->compositeParamsData_->glitchIntensity = instance->glitchIntensity_;
        instance->compositeParamsData_->chromaticAberrationEnabled = instance->chromaticAberrationEnabled_ ? 1 : 0;
        instance->compositeParamsData_->chromaticAberrationStrength = instance->chromaticAberrationStrength_;
        instance->compositeParamsData_->blurEnabled = instance->blurEnabled_ ? 1 : 0;
        instance->compositeParamsData_->blurStrength = instance->blurStrength_;
        instance->compositeParamsData_->grayscaleEnabled = instance->grayscaleEnabled_ ? 1 : 0;
        instance->compositeParamsData_->grayscaleIntensity = instance->grayscaleIntensity_;
        instance->compositeParamsData_->noiseEnabled = instance->noiseEnabled_ ? 1 : 0;
        instance->compositeParamsData_->noiseIntensity = instance->noiseIntensity_;
        instance->compositeParamsData_->time = 0.0f;

        // ノイズ/グリッチのアニメーション用に、経過時間の起点をここで記録しておく
        instance->startTime_ = std::chrono::steady_clock::now();

        Logger::LogSuccess("PostProcess : Initialized\n");
    }

    void PostProcess::Finalize() {
        PostProcess* instance = GetInstance();

        Logger::Log("PostProcess : Finalizing...\n");
        instance->compositeParamsResource_.Reset();
        instance->compositeParamsData_ = nullptr;

        instance->bloomUpsamplePipelineState_.Reset();
        instance->bloomDownsamplePipelineState_.Reset();
        instance->bloomThresholdPipelineState_.Reset();
        instance->bloomLevels_.clear();

        instance->compositePipelineState_.Reset();
        instance->compositeRootSignature_.Reset();
        instance->sceneColorRtvHeap_.Reset();
        instance->sceneColorResource_.Reset();
        Logger::LogSuccess("PostProcess : Finalized\n");
    }

    void PostProcess::CreateSceneColorResource() {
        auto device = DirectXCommon::GetDevice();

        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Width = static_cast<UINT>(DirectXCommon::GetBackBufferWidth());
        resourceDesc.Height = static_cast<UINT>(DirectXCommon::GetBackBufferHeight());
        resourceDesc.MipLevels = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        D3D12_HEAP_PROPERTIES heapProperties{};
        heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_CLEAR_VALUE clearValue{};
        clearValue.Format = resourceDesc.Format;
        clearValue.Color[0] = 0.1f;
        clearValue.Color[1] = 0.25f;
        clearValue.Color[2] = 0.5f;
        clearValue.Color[3] = 1.0f;

        HRESULT hr = device->CreateCommittedResource(
            &heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
            D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue, IID_PPV_ARGS(&sceneColorResource_)
        );
        assert(SUCCEEDED(hr));
        sceneColorState_ = D3D12_RESOURCE_STATE_RENDER_TARGET;

        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.NumDescriptors = 1;
        hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&sceneColorRtvHeap_));
        assert(SUCCEEDED(hr));

        device->CreateRenderTargetView(sceneColorResource_.Get(), nullptr, sceneColorRtvHeap_->GetCPUDescriptorHandleForHeapStart());

        sceneColorTextureHandle_ = TextureManager::RegisterResource(sceneColorResource_);
    }

    void PostProcess::CreateRootSignatureAndPSO() {
        auto device = DirectXCommon::GetDevice();
        HRESULT hr = S_OK;

        // t0 : メインの入力テクスチャ (シーンカラー、またはブルームの各レベル)
        D3D12_DESCRIPTOR_RANGE rangeT0[1] = {};
        rangeT0[0].BaseShaderRegister = 0;
        rangeT0[0].NumDescriptors = 1;
        rangeT0[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        rangeT0[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        // t1 : 副次的な入力テクスチャ (Compositeパスでのみブルーム結果を渡すのに使う)
        D3D12_DESCRIPTOR_RANGE rangeT1[1] = {};
        rangeT1[0].BaseShaderRegister = 1;
        rangeT1[0].NumDescriptors = 1;
        rangeT1[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        rangeT1[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_PARAMETER rootParameters[3] = {};
        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[0].DescriptorTable.pDescriptorRanges = rangeT0;
        rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(rangeT0);

        rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[1].DescriptorTable.pDescriptorRanges = rangeT1;
        rootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(rangeT1);

        rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[2].Descriptor.ShaderRegister = 0; // b0

        D3D12_STATIC_SAMPLER_DESC staticSampler{};
        staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
        staticSampler.ShaderRegister = 0; // s0
        staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
        rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE; // 頂点バッファ不要
        rootSignatureDesc.pParameters = rootParameters;
        rootSignatureDesc.NumParameters = _countof(rootParameters);
        rootSignatureDesc.pStaticSamplers = &staticSampler;
        rootSignatureDesc.NumStaticSamplers = 1;

        Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
        hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
        if (FAILED(hr)) {
            Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
            assert(false);
        }
        hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&compositeRootSignature_));
        assert(SUCCEEDED(hr));

        Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = ShaderCompiler::Compile(L"Resources/EngineResources/HLSL/PostProcess/Composite.VS.hlsl", L"vs_6_0");
        assert(vertexShaderBlob != nullptr);
        Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = ShaderCompiler::Compile(L"Resources/EngineResources/HLSL/PostProcess/Composite.PS.hlsl", L"ps_6_0");
        assert(pixelShaderBlob != nullptr);

        D3D12_BLEND_DESC blendDesc{};
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        D3D12_RASTERIZER_DESC rasterizerDesc{};
        rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
        rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
        psoDesc.pRootSignature = compositeRootSignature_.Get();
        psoDesc.InputLayout = { nullptr, 0 };
        psoDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
        psoDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
        psoDesc.BlendState = blendDesc;
        psoDesc.RasterizerState = rasterizerDesc;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DirectXCommon::GetBackBufferFormat(); // gameRenderTargetResource_と同じフォーマット
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.SampleDesc.Count = 1;
        psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
        psoDesc.DepthStencilState.DepthEnable = false;
        psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;

        hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&compositePipelineState_));
        assert(SUCCEEDED(hr));
    }

    void PostProcess::CreateBloomLevels() {
        auto device = DirectXCommon::GetDevice();

        uint32_t width = static_cast<uint32_t>(DirectXCommon::GetBackBufferWidth());
        uint32_t height = static_cast<uint32_t>(DirectXCommon::GetBackBufferHeight());

        bloomLevels_.clear();
        bloomLevels_.reserve(kBloomLevelCount);

        for (uint32_t i = 0; i < kBloomLevelCount; ++i) {
            width = std::max<uint32_t>(1, width / 2);
            height = std::max<uint32_t>(1, height / 2);

            BloomLevel level{};
            level.width = width;
            level.height = height;

            D3D12_RESOURCE_DESC resourceDesc{};
            resourceDesc.Width = width;
            resourceDesc.Height = height;
            resourceDesc.MipLevels = 1;
            resourceDesc.DepthOrArraySize = 1;
            resourceDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            resourceDesc.SampleDesc.Count = 1;
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

            D3D12_HEAP_PROPERTIES heapProperties{};
            heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

            D3D12_CLEAR_VALUE clearValue{};
            clearValue.Format = resourceDesc.Format;
            clearValue.Color[0] = clearValue.Color[1] = clearValue.Color[2] = 0.0f;
            clearValue.Color[3] = 1.0f;

            HRESULT hr = device->CreateCommittedResource(
                &heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue, IID_PPV_ARGS(&level.resource)
            );
            assert(SUCCEEDED(hr));
            level.state = D3D12_RESOURCE_STATE_RENDER_TARGET;

            D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
            rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            rtvHeapDesc.NumDescriptors = 1;
            hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&level.rtvHeap));
            assert(SUCCEEDED(hr));
            device->CreateRenderTargetView(level.resource.Get(), nullptr, level.rtvHeap->GetCPUDescriptorHandleForHeapStart());

            level.textureHandle = TextureManager::RegisterResource(level.resource);

            bloomLevels_.push_back(level);
        }
    }

    void PostProcess::CreateBloomPipelineStates() {
        auto device = DirectXCommon::GetDevice();
        HRESULT hr = S_OK;

        Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = ShaderCompiler::Compile(L"Resources/EngineResources/HLSL/PostProcess/Composite.VS.hlsl", L"vs_6_0");
        assert(vertexShaderBlob != nullptr);

        D3D12_RASTERIZER_DESC rasterizerDesc{};
        rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
        rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC baseDesc{};
        baseDesc.pRootSignature = compositeRootSignature_.Get();
        baseDesc.InputLayout = { nullptr, 0 };
        baseDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
        baseDesc.RasterizerState = rasterizerDesc;
        baseDesc.NumRenderTargets = 1;
        baseDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT; // ブルーム系のバッファは全部HDRフォーマット
        baseDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        baseDesc.SampleDesc.Count = 1;
        baseDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
        baseDesc.DepthStencilState.DepthEnable = false;
        baseDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;

        // --- 閾値抽出パス (上書き) ---
        {
            Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = ShaderCompiler::Compile(L"Resources/EngineResources/HLSL/PostProcess/BloomThreshold.PS.hlsl", L"ps_6_0");
            assert(pixelShaderBlob != nullptr);

            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = baseDesc;
            psoDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
            D3D12_BLEND_DESC blendDesc{};
            blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
            psoDesc.BlendState = blendDesc;

            hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&bloomThresholdPipelineState_));
            assert(SUCCEEDED(hr));
        }

        // ダウンサンプルは13点の重み付き平均、アップサンプルは3x3のテントフィルタで行う
        // (1回のバイリニアだけだと、低解像度の段を引き伸ばしたときにブロック状のムラが出るため)
        // 上書き/加算の違いは、これまで通りPSOのBlendStateで決める
        Microsoft::WRL::ComPtr<IDxcBlob> downsampleShaderBlob = ShaderCompiler::Compile(L"Resources/EngineResources/HLSL/PostProcess/BloomDownsample.PS.hlsl", L"ps_6_0");
        assert(downsampleShaderBlob != nullptr);
        Microsoft::WRL::ComPtr<IDxcBlob> upsampleShaderBlob = ShaderCompiler::Compile(L"Resources/EngineResources/HLSL/PostProcess/BloomUpsample.PS.hlsl", L"ps_6_0");
        assert(upsampleShaderBlob != nullptr);

        // --- ダウンサンプルパス (上書き) ---
        {
            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = baseDesc;
            psoDesc.PS = { downsampleShaderBlob->GetBufferPointer(), downsampleShaderBlob->GetBufferSize() };
            D3D12_BLEND_DESC blendDesc{};
            blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
            psoDesc.BlendState = blendDesc;

            hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&bloomDownsamplePipelineState_));
            assert(SUCCEEDED(hr));
        }

        // --- アップサンプル+加算合成パス (加算ブレンド：SrcColor*1 + DestColor*1) ---
        {
            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = baseDesc;
            psoDesc.PS = { upsampleShaderBlob->GetBufferPointer(), upsampleShaderBlob->GetBufferSize() };

            D3D12_BLEND_DESC blendDesc{};
            blendDesc.RenderTarget[0].BlendEnable = true;
            blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
            blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
            blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
            blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
            blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
            blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
            blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
            psoDesc.BlendState = blendDesc;

            hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&bloomUpsamplePipelineState_));
            assert(SUCCEEDED(hr));
        }
    }

    void PostProcess::TransitionSceneColor(D3D12_RESOURCE_STATES newState) {
        if (sceneColorState_ == newState) {
            return;
        }
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = sceneColorResource_.Get();
        barrier.Transition.StateBefore = sceneColorState_;
        barrier.Transition.StateAfter = newState;
        DirectXCommon::GetCommandList()->ResourceBarrier(1, &barrier);
        sceneColorState_ = newState;
    }

    void PostProcess::TransitionBloomLevel(size_t index, D3D12_RESOURCE_STATES newState) {
        BloomLevel& level = bloomLevels_[index];
        if (level.state == newState) {
            return;
        }
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = level.resource.Get();
        barrier.Transition.StateBefore = level.state;
        barrier.Transition.StateAfter = newState;
        DirectXCommon::GetCommandList()->ResourceBarrier(1, &barrier);
        level.state = newState;
    }

    void PostProcess::DrawFullscreenTriangle(uint32_t width, uint32_t height) {
        auto commandList = DirectXCommon::GetCommandList();

        D3D12_VIEWPORT viewport{};
        viewport.Width = static_cast<float>(width);
        viewport.Height = static_cast<float>(height);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        commandList->RSSetViewports(1, &viewport);

        D3D12_RECT scissorRect{};
        scissorRect.right = static_cast<LONG>(width);
        scissorRect.bottom = static_cast<LONG>(height);
        commandList->RSSetScissorRects(1, &scissorRect);

        commandList->DrawInstanced(3, 1, 0, 0);
    }

    void PostProcess::BeginScenePass() {
        PostProcess* instance = GetInstance();

        auto commandList = DirectXCommon::GetCommandList();

        instance->TransitionSceneColor(D3D12_RESOURCE_STATE_RENDER_TARGET);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = instance->sceneColorRtvHeap_->GetCPUDescriptorHandleForHeapStart();
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = DirectXCommon::GetDSVHandle();

        commandList->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

        float clearColor[4] = { 0.1f, 0.25f, 0.5f, 1.0f };
        commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    }

    void PostProcess::EndScenePass() {
        PostProcess* instance = GetInstance();

        instance->TransitionSceneColor(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

    void PostProcess::RenderBloom() {
        if (bloomLevels_.empty()) {
            return;
        }
        auto commandList = DirectXCommon::GetCommandList();
        size_t levelCount = bloomLevels_.size();

        // --- 閾値抽出：シーンカラー(フル解像度) → bloomLevels_[0](半解像度) ---
        commandList->SetPipelineState(bloomThresholdPipelineState_.Get());
        TransitionBloomLevel(0, D3D12_RESOURCE_STATE_RENDER_TARGET);
        {
            D3D12_CPU_DESCRIPTOR_HANDLE rtv = bloomLevels_[0].rtvHeap->GetCPUDescriptorHandleForHeapStart();
            commandList->OMSetRenderTargets(1, &rtv, false, nullptr);
        }
        commandList->SetGraphicsRootDescriptorTable(0, TextureManager::GetGPUHandle(sceneColorTextureHandle_));
        DrawFullscreenTriangle(bloomLevels_[0].width, bloomLevels_[0].height);
        TransitionBloomLevel(0, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        // --- ダウンサンプルチェーン：bloomLevels_[i] → bloomLevels_[i+1] ---
        commandList->SetPipelineState(bloomDownsamplePipelineState_.Get());
        for (size_t i = 0; i + 1 < levelCount; ++i) {
            TransitionBloomLevel(i + 1, D3D12_RESOURCE_STATE_RENDER_TARGET);
            {
                D3D12_CPU_DESCRIPTOR_HANDLE rtv = bloomLevels_[i + 1].rtvHeap->GetCPUDescriptorHandleForHeapStart();
                commandList->OMSetRenderTargets(1, &rtv, false, nullptr);
            }
            commandList->SetGraphicsRootDescriptorTable(0, TextureManager::GetGPUHandle(bloomLevels_[i].textureHandle));
            DrawFullscreenTriangle(bloomLevels_[i + 1].width, bloomLevels_[i + 1].height);
            TransitionBloomLevel(i + 1, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        }

        // --- アップサンプル+加算合成チェーン：bloomLevels_[i] → bloomLevels_[i-1]へ加算 ---
        commandList->SetPipelineState(bloomUpsamplePipelineState_.Get());
        for (size_t i = levelCount - 1; i > 0; --i) {
            TransitionBloomLevel(i - 1, D3D12_RESOURCE_STATE_RENDER_TARGET);
            {
                D3D12_CPU_DESCRIPTOR_HANDLE rtv = bloomLevels_[i - 1].rtvHeap->GetCPUDescriptorHandleForHeapStart();
                commandList->OMSetRenderTargets(1, &rtv, false, nullptr);
            }
            commandList->SetGraphicsRootDescriptorTable(0, TextureManager::GetGPUHandle(bloomLevels_[i].textureHandle));
            DrawFullscreenTriangle(bloomLevels_[i - 1].width, bloomLevels_[i - 1].height);
            TransitionBloomLevel(i - 1, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        }

        // ここまで終わると、bloomLevels_[0]に全スケール分がブレンドされた最終的なブルーム結果が入っている
    }

    void PostProcess::Composite() {
        PostProcess* instance = GetInstance();

        auto commandList = DirectXCommon::GetCommandList();

        commandList->SetGraphicsRootSignature(instance->compositeRootSignature_.Get());
        
        SrvManager::PreDraw(commandList);
        // NOTE: root param1(t1)はRootSignature上、常に何か有効な値がバインドされている必要があるため、
        //       ブルームを使わない/まだ計算していない段階ではひとまずシーンカラー自体を仮に入れておく
        //       (Threshold/Downsample/Upsampleの各シェーダーはt1を一切参照しないので実害無し)
        commandList->SetGraphicsRootDescriptorTable(1, TextureManager::GetGPUHandle(instance->sceneColorTextureHandle_));
        commandList->SetGraphicsRootConstantBufferView(2, instance->compositeParamsResource_->GetGPUVirtualAddress());
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // ImGuiでの切り替え内容をシェーダー用バッファへ反映
        instance->compositeParamsData_->acesEnabled = instance->acesEnabled_ ? 1 : 0;
        instance->compositeParamsData_->bloomEnabled = instance->bloomEnabled_ ? 1 : 0;
        instance->compositeParamsData_->exposure = instance->exposure_;
        instance->compositeParamsData_->threshold = instance->bloomThreshold_;
        instance->compositeParamsData_->bloomIntensity = instance->bloomIntensity_;
        instance->compositeParamsData_->distortionEnabled = instance->distortionEnabled_ ? 1 : 0;
        instance->compositeParamsData_->distortionStrength = instance->distortionStrength_;
        instance->compositeParamsData_->glitchEnabled = instance->glitchEnabled_ ? 1 : 0;
        instance->compositeParamsData_->glitchIntensity = instance->glitchIntensity_;
        instance->compositeParamsData_->chromaticAberrationEnabled = instance->chromaticAberrationEnabled_ ? 1 : 0;
        instance->compositeParamsData_->chromaticAberrationStrength = instance->chromaticAberrationStrength_;
        instance->compositeParamsData_->blurEnabled = instance->blurEnabled_ ? 1 : 0;
        instance->compositeParamsData_->blurStrength = instance->blurStrength_;
        instance->compositeParamsData_->grayscaleEnabled = instance->grayscaleEnabled_ ? 1 : 0;
        instance->compositeParamsData_->grayscaleIntensity = instance->grayscaleIntensity_;
        instance->compositeParamsData_->noiseEnabled = instance->noiseEnabled_ ? 1 : 0;
        instance->compositeParamsData_->noiseIntensity = instance->noiseIntensity_;

        // ノイズ/グリッチのアニメーション用の経過時間(秒)
        std::chrono::duration<float> elapsed = std::chrono::steady_clock::now() - instance->startTime_;
        instance->compositeParamsData_->time = elapsed.count();

        if (instance->bloomEnabled_) {
            instance->RenderBloom();
            commandList->SetGraphicsRootDescriptorTable(1, TextureManager::GetGPUHandle(instance->bloomLevels_[0].textureHandle));
        }

        // 描画先をgameRenderTargetResource_へ戻す
        D3D12_CPU_DESCRIPTOR_HANDLE gameRtvHandle = DirectXCommon::GetGameRenderTargetRTVHandle();
        commandList->OMSetRenderTargets(1, &gameRtvHandle, false, nullptr);

        // NOTE: RenderBloom()内でビューポート/シザーをブルーム用の縮小解像度に変更しているため、
        //       ここでバックバッファサイズへ明示的に戻す必要がある
        D3D12_VIEWPORT viewport{};
        viewport.Width = static_cast<float>(DirectXCommon::GetBackBufferWidth());
        viewport.Height = static_cast<float>(DirectXCommon::GetBackBufferHeight());
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        commandList->RSSetViewports(1, &viewport);

        D3D12_RECT scissorRect{};
        scissorRect.right = static_cast<LONG>(DirectXCommon::GetBackBufferWidth());
        scissorRect.bottom = static_cast<LONG>(DirectXCommon::GetBackBufferHeight());
        commandList->RSSetScissorRects(1, &scissorRect);

        commandList->SetPipelineState(instance->compositePipelineState_.Get());
        commandList->SetGraphicsRootDescriptorTable(0, TextureManager::GetGPUHandle(instance->sceneColorTextureHandle_));

        commandList->DrawInstanced(3, 1, 0, 0);
    }

    void PostProcess::DrawImGui() {
        PostProcess* instance = GetInstance();

#ifdef _DEBUG
        ImGui::Begin("PostProcess");
        ImGui::SliderFloat("露出", &instance->exposure_, 0.0f, 5.0f);
        ImGui::Checkbox("ACES トーンマッピング", &instance->acesEnabled_);
        if (!instance->acesEnabled_) {
            ImGui::TextDisabled("(OFF時は露出後の値を単純にクリップします)");
        }
        ImGui::Separator();
        ImGui::Checkbox("Bloom", &instance->bloomEnabled_);
        if (instance->bloomEnabled_) {
            ImGui::SliderFloat("Bloom対象となる境界値", &instance->bloomThreshold_, 0.0f, 10.0f);
            ImGui::SliderFloat("Bloomの強さ", &instance->bloomIntensity_, 0.0f, 10.0f);
        }
        ImGui::Separator();
        ImGui::Checkbox("色収差(Chromatic Aberration)", &instance->chromaticAberrationEnabled_);
        if (instance->chromaticAberrationEnabled_) {
            ImGui::SliderFloat("色収差の強さ", &instance->chromaticAberrationStrength_, 0.0f, 0.05f);
        }
        ImGui::Separator();
        ImGui::Checkbox("ブラー(簡易版)", &instance->blurEnabled_);
        if (instance->blurEnabled_) {
            ImGui::SliderFloat("ブラーの強さ", &instance->blurStrength_, 0.0f, 0.02f);
            ImGui::TextDisabled("(3x3の簡易ボックスブラー。強くかけるとバンディングが出やすい)");
        }
        ImGui::Separator();
        ImGui::Checkbox("ノイズ(グレイン)", &instance->noiseEnabled_);
        if (instance->noiseEnabled_) {
            ImGui::SliderFloat("ノイズの強さ", &instance->noiseIntensity_, 0.0f, 0.5f);
        }
        ImGui::Separator();
        ImGui::Checkbox("グリッチ", &instance->glitchEnabled_);
        if (instance->glitchEnabled_) {
            ImGui::SliderFloat("グリッチの強さ", &instance->glitchIntensity_, 0.0f, 0.3f);
        }
        ImGui::Separator();
        ImGui::Checkbox("ゆがみ(Distortion)", &instance->distortionEnabled_);
        if (instance->distortionEnabled_) {
            ImGui::SliderFloat("ゆがみの強さ", &instance->distortionStrength_, 0.0f, 0.05f);
        }
        ImGui::Separator();
        ImGui::Checkbox("白黒化(Grayscale)", &instance->grayscaleEnabled_);
        if (instance->grayscaleEnabled_) {
            ImGui::SliderFloat("白黒の度合い", &instance->grayscaleIntensity_, 0.0f, 1.0f);
        }
        ImGui::End();
#endif
    }
}
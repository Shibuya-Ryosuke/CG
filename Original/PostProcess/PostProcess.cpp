#include "PostProcess.h"
#include "../Base/DirectXCommon.h"
#include "../Base/Logger.h"
#include "../Base/ShaderCompiler.h"
#include "../Graphics/TextureManager.h"
#include "../Externals/imgui/imgui.h"
#include <cassert>
#include <algorithm>

namespace RyoEngine {

    PostProcess* PostProcess::GetInstance() {
        static PostProcess instance;
        return &instance;
    }

    void PostProcess::Initialize() {
        Logger::Log("PostProcess : Initializing...\n");

        CreateSceneColorResource();
        CreateRootSignatureAndPSO();
        CreateBloomLevels();
        CreateBloomPipelineStates();

        auto device = DirectXCommon::GetInstance()->GetDevice();
        compositeParamsResource_ = DirectXCommon::CreateBufferResource(device, sizeof(PostProcessParams));
        compositeParamsResource_->Map(0, nullptr, reinterpret_cast<void**>(&compositeParamsData_));
        compositeParamsData_->acesEnabled = acesEnabled_ ? 1 : 0;
        compositeParamsData_->bloomEnabled = bloomEnabled_ ? 1 : 0;
        compositeParamsData_->exposure = exposure_;
        compositeParamsData_->threshold = bloomThreshold_;
        compositeParamsData_->bloomIntensity = bloomIntensity_;
        compositeParamsData_->distortionEnabled = distortionEnabled_ ? 1 : 0;
        compositeParamsData_->distortionStrength = distortionStrength_;
        compositeParamsData_->glitchEnabled = glitchEnabled_ ? 1 : 0;
        compositeParamsData_->glitchIntensity = glitchIntensity_;
        compositeParamsData_->chromaticAberrationEnabled = chromaticAberrationEnabled_ ? 1 : 0;
        compositeParamsData_->chromaticAberrationStrength = chromaticAberrationStrength_;
        compositeParamsData_->blurEnabled = blurEnabled_ ? 1 : 0;
        compositeParamsData_->blurStrength = blurStrength_;
        compositeParamsData_->grayscaleEnabled = grayscaleEnabled_ ? 1 : 0;
        compositeParamsData_->grayscaleIntensity = grayscaleIntensity_;
        compositeParamsData_->noiseEnabled = noiseEnabled_ ? 1 : 0;
        compositeParamsData_->noiseIntensity = noiseIntensity_;
        compositeParamsData_->time = 0.0f;

        // ノイズ/グリッチのアニメーション用に、経過時間の起点をここで記録しておく
        startTime_ = std::chrono::steady_clock::now();

        Logger::LogSuccess("PostProcess : Initialized\n");
    }

    void PostProcess::Finalize() {
        Logger::Log("PostProcess : Finalizing...\n");
        compositeParamsResource_.Reset();
        compositeParamsData_ = nullptr;

        bloomUpsamplePipelineState_.Reset();
        bloomDownsamplePipelineState_.Reset();
        bloomThresholdPipelineState_.Reset();
        bloomLevels_.clear();

        compositePipelineState_.Reset();
        compositeRootSignature_.Reset();
        sceneColorRtvHeap_.Reset();
        sceneColorResource_.Reset();
        Logger::LogSuccess("PostProcess : Finalized\n");
    }

    void PostProcess::CreateSceneColorResource() {
        auto dxCommon = DirectXCommon::GetInstance();
        auto device = dxCommon->GetDevice();

        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Width = static_cast<UINT>(dxCommon->GetBackBufferWidth());
        resourceDesc.Height = static_cast<UINT>(dxCommon->GetBackBufferHeight());
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

        sceneColorTextureHandle_ = TextureManager::GetInstance()->RegisterResource(sceneColorResource_);
    }

    void PostProcess::CreateRootSignatureAndPSO() {
        auto device = DirectXCommon::GetInstance()->GetDevice();
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

        Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = ShaderCompiler::GetInstance()->Compile(L"HLSL/PostProcess/Composite.VS.hlsl", L"vs_6_0");
        assert(vertexShaderBlob != nullptr);
        Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = ShaderCompiler::GetInstance()->Compile(L"HLSL/PostProcess/Composite.PS.hlsl", L"ps_6_0");
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
        psoDesc.RTVFormats[0] = DirectXCommon::GetInstance()->GetBackBufferFormat(); // gameRenderTargetResource_と同じフォーマット
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.SampleDesc.Count = 1;
        psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
        psoDesc.DepthStencilState.DepthEnable = false;
        psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;

        hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&compositePipelineState_));
        assert(SUCCEEDED(hr));
    }

    void PostProcess::CreateBloomLevels() {
        auto dxCommon = DirectXCommon::GetInstance();
        auto device = dxCommon->GetDevice();

        uint32_t width = static_cast<uint32_t>(dxCommon->GetBackBufferWidth());
        uint32_t height = static_cast<uint32_t>(dxCommon->GetBackBufferHeight());

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

            level.textureHandle = TextureManager::GetInstance()->RegisterResource(level.resource);

            bloomLevels_.push_back(level);
        }
    }

    void PostProcess::CreateBloomPipelineStates() {
        auto device = DirectXCommon::GetInstance()->GetDevice();
        HRESULT hr = S_OK;

        Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = ShaderCompiler::GetInstance()->Compile(L"HLSL/PostProcess/Composite.VS.hlsl", L"vs_6_0");
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
            Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = ShaderCompiler::GetInstance()->Compile(L"HLSL/PostProcess/BloomThreshold.PS.hlsl", L"ps_6_0");
            assert(pixelShaderBlob != nullptr);

            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = baseDesc;
            psoDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
            D3D12_BLEND_DESC blendDesc{};
            blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
            psoDesc.BlendState = blendDesc;

            hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&bloomThresholdPipelineState_));
            assert(SUCCEEDED(hr));
        }

        // ダウンサンプル・アップサンプルは全く同じシェーダー(単純なサンプル→出力)を使い回し、
        // PSOのBlendStateの違い(上書き/加算)だけで役割を変える
        Microsoft::WRL::ComPtr<IDxcBlob> sampleShaderBlob = ShaderCompiler::GetInstance()->Compile(L"HLSL/PostProcess/BloomSample.PS.hlsl", L"ps_6_0");
        assert(sampleShaderBlob != nullptr);

        // --- ダウンサンプルパス (上書き) ---
        {
            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = baseDesc;
            psoDesc.PS = { sampleShaderBlob->GetBufferPointer(), sampleShaderBlob->GetBufferSize() };
            D3D12_BLEND_DESC blendDesc{};
            blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
            psoDesc.BlendState = blendDesc;

            hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&bloomDownsamplePipelineState_));
            assert(SUCCEEDED(hr));
        }

        // --- アップサンプル+加算合成パス (加算ブレンド：SrcColor*1 + DestColor*1) ---
        {
            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = baseDesc;
            psoDesc.PS = { sampleShaderBlob->GetBufferPointer(), sampleShaderBlob->GetBufferSize() };

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
        DirectXCommon::GetInstance()->GetCommandList()->ResourceBarrier(1, &barrier);
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
        DirectXCommon::GetInstance()->GetCommandList()->ResourceBarrier(1, &barrier);
        level.state = newState;
    }

    void PostProcess::DrawFullscreenTriangle(uint32_t width, uint32_t height) {
        auto commandList = DirectXCommon::GetInstance()->GetCommandList();

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
        auto commandList = DirectXCommon::GetInstance()->GetCommandList();

        TransitionSceneColor(D3D12_RESOURCE_STATE_RENDER_TARGET);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = sceneColorRtvHeap_->GetCPUDescriptorHandleForHeapStart();
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = DirectXCommon::GetInstance()->GetDSVHandle();

        commandList->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

        float clearColor[4] = { 0.1f, 0.25f, 0.5f, 1.0f };
        commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    }

    void PostProcess::EndScenePass() {
        TransitionSceneColor(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

    void PostProcess::RenderBloom() {
        if (bloomLevels_.empty()) {
            return;
        }
        auto commandList = DirectXCommon::GetInstance()->GetCommandList();
        size_t levelCount = bloomLevels_.size();

        // --- 閾値抽出：シーンカラー(フル解像度) → bloomLevels_[0](半解像度) ---
        commandList->SetPipelineState(bloomThresholdPipelineState_.Get());
        TransitionBloomLevel(0, D3D12_RESOURCE_STATE_RENDER_TARGET);
        {
            D3D12_CPU_DESCRIPTOR_HANDLE rtv = bloomLevels_[0].rtvHeap->GetCPUDescriptorHandleForHeapStart();
            commandList->OMSetRenderTargets(1, &rtv, false, nullptr);
        }
        commandList->SetGraphicsRootDescriptorTable(0, TextureManager::GetInstance()->GetGPUHandle(sceneColorTextureHandle_));
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
            commandList->SetGraphicsRootDescriptorTable(0, TextureManager::GetInstance()->GetGPUHandle(bloomLevels_[i].textureHandle));
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
            commandList->SetGraphicsRootDescriptorTable(0, TextureManager::GetInstance()->GetGPUHandle(bloomLevels_[i].textureHandle));
            DrawFullscreenTriangle(bloomLevels_[i - 1].width, bloomLevels_[i - 1].height);
            TransitionBloomLevel(i - 1, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        }

        // ここまで終わると、bloomLevels_[0]に全スケール分がブレンドされた最終的なブルーム結果が入っている
    }

    void PostProcess::Composite() {
        auto commandList = DirectXCommon::GetInstance()->GetCommandList();

        commandList->SetGraphicsRootSignature(compositeRootSignature_.Get());
        ID3D12DescriptorHeap* ppHeaps[] = { TextureManager::GetInstance()->GetDescriptorHeap() };
        commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
        // NOTE: root param1(t1)はRootSignature上、常に何か有効な値がバインドされている必要があるため、
        //       ブルームを使わない/まだ計算していない段階ではひとまずシーンカラー自体を仮に入れておく
        //       (Threshold/Downsample/Upsampleの各シェーダーはt1を一切参照しないので実害無し)
        commandList->SetGraphicsRootDescriptorTable(1, TextureManager::GetInstance()->GetGPUHandle(sceneColorTextureHandle_));
        commandList->SetGraphicsRootConstantBufferView(2, compositeParamsResource_->GetGPUVirtualAddress());
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // ImGuiでの切り替え内容をシェーダー用バッファへ反映
        compositeParamsData_->acesEnabled = acesEnabled_ ? 1 : 0;
        compositeParamsData_->bloomEnabled = bloomEnabled_ ? 1 : 0;
        compositeParamsData_->exposure = exposure_;
        compositeParamsData_->threshold = bloomThreshold_;
        compositeParamsData_->bloomIntensity = bloomIntensity_;
        compositeParamsData_->distortionEnabled = distortionEnabled_ ? 1 : 0;
        compositeParamsData_->distortionStrength = distortionStrength_;
        compositeParamsData_->glitchEnabled = glitchEnabled_ ? 1 : 0;
        compositeParamsData_->glitchIntensity = glitchIntensity_;
        compositeParamsData_->chromaticAberrationEnabled = chromaticAberrationEnabled_ ? 1 : 0;
        compositeParamsData_->chromaticAberrationStrength = chromaticAberrationStrength_;
        compositeParamsData_->blurEnabled = blurEnabled_ ? 1 : 0;
        compositeParamsData_->blurStrength = blurStrength_;
        compositeParamsData_->grayscaleEnabled = grayscaleEnabled_ ? 1 : 0;
        compositeParamsData_->grayscaleIntensity = grayscaleIntensity_;
        compositeParamsData_->noiseEnabled = noiseEnabled_ ? 1 : 0;
        compositeParamsData_->noiseIntensity = noiseIntensity_;

        // ノイズ/グリッチのアニメーション用の経過時間(秒)
        std::chrono::duration<float> elapsed = std::chrono::steady_clock::now() - startTime_;
        compositeParamsData_->time = elapsed.count();

        if (bloomEnabled_) {
            RenderBloom();
            commandList->SetGraphicsRootDescriptorTable(1, TextureManager::GetInstance()->GetGPUHandle(bloomLevels_[0].textureHandle));
        }

        // 描画先をgameRenderTargetResource_へ戻す
        D3D12_CPU_DESCRIPTOR_HANDLE gameRtvHandle = DirectXCommon::GetInstance()->GetGameRenderTargetRTVHandle();
        commandList->OMSetRenderTargets(1, &gameRtvHandle, false, nullptr);

        // NOTE: RenderBloom()内でビューポート/シザーをブルーム用の縮小解像度に変更しているため、
        //       ここでバックバッファサイズへ明示的に戻す必要がある
        auto dxCommon = DirectXCommon::GetInstance();
        D3D12_VIEWPORT viewport{};
        viewport.Width = static_cast<float>(dxCommon->GetBackBufferWidth());
        viewport.Height = static_cast<float>(dxCommon->GetBackBufferHeight());
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        commandList->RSSetViewports(1, &viewport);

        D3D12_RECT scissorRect{};
        scissorRect.right = static_cast<LONG>(dxCommon->GetBackBufferWidth());
        scissorRect.bottom = static_cast<LONG>(dxCommon->GetBackBufferHeight());
        commandList->RSSetScissorRects(1, &scissorRect);

        commandList->SetPipelineState(compositePipelineState_.Get());
        commandList->SetGraphicsRootDescriptorTable(0, TextureManager::GetInstance()->GetGPUHandle(sceneColorTextureHandle_));

        commandList->DrawInstanced(3, 1, 0, 0);
    }

    void PostProcess::DrawImGui() {
#ifdef _DEBUG
        ImGui::Begin("PostProcess");
        ImGui::SliderFloat("露出", &exposure_, 0.0f, 5.0f);
        ImGui::Checkbox("ACES トーンマッピング", &acesEnabled_);
        if (!acesEnabled_) {
            ImGui::TextDisabled("(OFF時は露出後の値を単純にクリップします)");
        }
        ImGui::Separator();
        ImGui::Checkbox("Bloom", &bloomEnabled_);
        if (bloomEnabled_) {
            ImGui::SliderFloat("Bloom対象となる境界値", &bloomThreshold_, 0.0f, 10.0f);
            ImGui::SliderFloat("Bloomの強さ", &bloomIntensity_, 0.0f, 10.0f);
        }
        ImGui::Separator();
        ImGui::Checkbox("色収差(Chromatic Aberration)", &chromaticAberrationEnabled_);
        if (chromaticAberrationEnabled_) {
            ImGui::SliderFloat("色収差の強さ", &chromaticAberrationStrength_, 0.0f, 0.05f);
        }
        ImGui::Separator();
        ImGui::Checkbox("ブラー(簡易版)", &blurEnabled_);
        if (blurEnabled_) {
            ImGui::SliderFloat("ブラーの強さ", &blurStrength_, 0.0f, 0.02f);
            ImGui::TextDisabled("(3x3の簡易ボックスブラー。強くかけるとバンディングが出やすい)");
        }
        ImGui::Separator();
        ImGui::Checkbox("ノイズ(グレイン)", &noiseEnabled_);
        if (noiseEnabled_) {
            ImGui::SliderFloat("ノイズの強さ", &noiseIntensity_, 0.0f, 0.5f);
        }
        ImGui::Separator();
        ImGui::Checkbox("グリッチ", &glitchEnabled_);
        if (glitchEnabled_) {
            ImGui::SliderFloat("グリッチの強さ", &glitchIntensity_, 0.0f, 0.3f);
        }
        ImGui::Separator();
        ImGui::Checkbox("ゆがみ(Distortion)", &distortionEnabled_);
        if (distortionEnabled_) {
            ImGui::SliderFloat("ゆがみの強さ", &distortionStrength_, 0.0f, 0.05f);
        }
        ImGui::Separator();
        ImGui::Checkbox("白黒化(Grayscale)", &grayscaleEnabled_);
        if (grayscaleEnabled_) {
            ImGui::SliderFloat("白黒の度合い", &grayscaleIntensity_, 0.0f, 1.0f);
        }
        ImGui::End();
#endif
    }
}
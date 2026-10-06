#include "GPUParticleCommon.h"
#include "../../Core/Base/DirectXCommon.h"
#include "../../Core/Base/Logger.h"
#include "../../Core/Base/ShaderCompiler.h"
#include "../../Core/Math/Geometry.h" // VertexDataのoffsetof用
#include <cassert>
#include <cstddef>

namespace RyoEngine {

    GPUParticleCommon* GPUParticleCommon::GetInstance() {
        static GPUParticleCommon instance;
        return &instance;
    }

    void GPUParticleCommon::Initialize() {
        Logger::Log("GPUParticleCommon : Initializing...\n");
        CreateComputeRootSignature();
        CreateComputePipelineStates();
        CreateRenderRootSignature();
        CreateRenderPipelineStates();
        Logger::LogSuccess("GPUParticleCommon : Initialized\n");
    }

    void GPUParticleCommon::Finalize() {
        Logger::Log("GPUParticleCommon : Finalizing...\n");
        for (auto& pso : renderPipelineStates_) {
            pso.Reset();
        }
        renderRootSignature_.Reset();
        simulatePipelineState_.Reset();
        resetPipelineState_.Reset();
        computeRootSignature_.Reset();
        Logger::LogSuccess("GPUParticleCommon : Finalized\n");
    }

    void GPUParticleCommon::Dispatch() {
        for (const auto& command : dispatchCommands_) {
            command();
        }
    }

    void GPUParticleCommon::Draw() {
        for (const auto& command : drawCommands_) {
            command();
        }
    }

    void GPUParticleCommon::CreateComputeRootSignature() {
        auto device = DirectXCommon::GetInstance()->GetDevice();
        HRESULT hr = S_OK;

        D3D12_ROOT_PARAMETER rootParameters[4] = {};
        // 0: パーティクル本体 (RWStructuredBuffer。Root Descriptor UAV)
        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters[0].Descriptor.ShaderRegister = 0; // u0
        // 1: 発生依頼消化用のカウンター (Root Descriptor UAV)
        rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
        rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters[1].Descriptor.ShaderRegister = 1; // u1
        // 2: 発生依頼リスト (StructuredBuffer。Root Descriptor SRV。読み取り専用)
        rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters[2].Descriptor.ShaderRegister = 0; // t0
        // 3: シミュレーション用パラメータ (deltaTime, 発生依頼数, 最大数, 重力等)
        rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters[3].Descriptor.ShaderRegister = 0; // b0

        D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
        rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE; // 頂点入力レイアウト不要(コンピュート専用)
        rootSignatureDesc.pParameters = rootParameters;
        rootSignatureDesc.NumParameters = _countof(rootParameters);
        rootSignatureDesc.pStaticSamplers = nullptr;
        rootSignatureDesc.NumStaticSamplers = 0;

        Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
        hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
        if (FAILED(hr)) {
            Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
            assert(false);
        }
        hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&computeRootSignature_));
        assert(SUCCEEDED(hr));
    }

    void GPUParticleCommon::CreateComputePipelineStates() {
        auto device = DirectXCommon::GetInstance()->GetDevice();
        HRESULT hr = S_OK;

        // --- リセットパス：カウンターを0に戻すだけの、1スレッドだけの軽いシェーダー ---
        {
            Microsoft::WRL::ComPtr<IDxcBlob> csBlob = ShaderCompiler::GetInstance()->Compile(L"Resources/EngineResources/HLSL/GPUParticle/GPUParticleReset.CS.hlsl", L"cs_6_0");
            assert(csBlob != nullptr);

            D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
            psoDesc.pRootSignature = computeRootSignature_.Get();
            psoDesc.CS = { csBlob->GetBufferPointer(), csBlob->GetBufferSize() };

            hr = device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&resetPipelineState_));
            assert(SUCCEEDED(hr));
        }

        // --- シミュレーション本体パス：物理演算＋新規発生の消化 ---
        {
            Microsoft::WRL::ComPtr<IDxcBlob> csBlob = ShaderCompiler::GetInstance()->Compile(L"Resources/EngineResources/HLSL/GPUParticle/GPUParticleSimulate.CS.hlsl", L"cs_6_0");
            assert(csBlob != nullptr);

            D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
            psoDesc.pRootSignature = computeRootSignature_.Get();
            psoDesc.CS = { csBlob->GetBufferPointer(), csBlob->GetBufferSize() };

            hr = device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&simulatePipelineState_));
            assert(SUCCEEDED(hr));
        }
    }

    void GPUParticleCommon::CreateRenderRootSignature() {
        auto device = DirectXCommon::GetInstance()->GetDevice();
        HRESULT hr = S_OK;

        // テクスチャ用DescriptorRange (t0, Pixel)
        D3D12_DESCRIPTOR_RANGE textureRange[1] = {};
        textureRange[0].BaseShaderRegister = 0;
        textureRange[0].NumDescriptors = 1;
        textureRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        textureRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_PARAMETER rootParameters[3] = {};
        // 0: パーティクル本体 (StructuredBuffer。Root Descriptor SRV、Vertexで読む)
        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        rootParameters[0].Descriptor.ShaderRegister = 0; // t0 (Vertex)
        // 1: カメラ関連(View-Projection行列＋ビルボード計算用のカメラ右・上ベクトル)
        rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        rootParameters[1].Descriptor.ShaderRegister = 0; // b0 (Vertex)
        // 2: テクスチャ (DescriptorTable SRV、Pixel)
        rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[2].DescriptorTable.pDescriptorRanges = textureRange;
        rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(textureRange);

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
        rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
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
        hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&renderRootSignature_));
        assert(SUCCEEDED(hr));
    }

    void GPUParticleCommon::CreateRenderPipelineStates() {
        auto device = DirectXCommon::GetInstance()->GetDevice();
        HRESULT hr = S_OK;

        // 頂点フォーマットはModel/InstancedModelと共通のVertexData(position/texcoord/normal)を使う
        // (normalは今回のシェーダーでは使わないが、InstancedMesh経由で読み込む形状と型を揃えるため残す)
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
        inputElementDescs[0].SemanticName = "POSITION";
        inputElementDescs[0].SemanticIndex = 0;
        inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        inputElementDescs[0].AlignedByteOffset = static_cast<UINT>(offsetof(VertexData, position));
        inputElementDescs[1].SemanticName = "TEXCOORD";
        inputElementDescs[1].SemanticIndex = 0;
        inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
        inputElementDescs[1].AlignedByteOffset = static_cast<UINT>(offsetof(VertexData, texcoord));
        inputElementDescs[2].SemanticName = "NORMAL";
        inputElementDescs[2].SemanticIndex = 0;
        inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
        inputElementDescs[2].AlignedByteOffset = static_cast<UINT>(offsetof(VertexData, normal));

        D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
        inputLayoutDesc.pInputElementDescs = inputElementDescs;
        inputLayoutDesc.NumElements = _countof(inputElementDescs);

        D3D12_RASTERIZER_DESC rasterizerDesc{};
        rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE; // 板ポリゴンなので裏表どちらも描く
        rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

        D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
        depthStencilDesc.DepthEnable = true;
        depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; // 半透明なので深度書き込みはしない(通常のパーティクルの定石)
        depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

        // 通常合成(Alpha)のBlendState
        D3D12_BLEND_DESC alphaBlendDesc{};
        alphaBlendDesc.RenderTarget[0].BlendEnable = true;
        alphaBlendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        alphaBlendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        alphaBlendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        alphaBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        alphaBlendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        alphaBlendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        alphaBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        // 加算合成(Additive)のBlendState
        D3D12_BLEND_DESC additiveBlendDesc = alphaBlendDesc;
        additiveBlendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC baseDesc{};
        baseDesc.pRootSignature = renderRootSignature_.Get();
        baseDesc.InputLayout = inputLayoutDesc;
        baseDesc.RasterizerState = rasterizerDesc;
        baseDesc.NumRenderTargets = 1;
        baseDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT; // PostProcessのHDRシーンバッファに合わせる
        baseDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        baseDesc.SampleDesc.Count = 1;
        baseDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
        baseDesc.DepthStencilState = depthStencilDesc;
        baseDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT; // メインの深度バッファと合わせる

        // 共通のピクセルシェーダー(テクスチャ×パーティクル色。ライティング計算は一切無し)
        Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = ShaderCompiler::GetInstance()->Compile(L"Resources/EngineResources/HLSL/GPUParticle/GPUParticle.PS.hlsl", L"ps_6_0");
        assert(pixelShaderBlob != nullptr);

        // 非ビルボード用/ビルボード用の2種類の頂点シェーダー
        Microsoft::WRL::ComPtr<IDxcBlob> vsNoBillboardBlob = ShaderCompiler::GetInstance()->Compile(L"Resources/EngineResources/HLSL/GPUParticle/GPUParticle.VS.hlsl", L"vs_6_0");
        assert(vsNoBillboardBlob != nullptr);
        Microsoft::WRL::ComPtr<IDxcBlob> vsBillboardBlob = ShaderCompiler::GetInstance()->Compile(L"Resources/EngineResources/HLSL/GPUParticle/GPUParticleBillboard.VS.hlsl", L"vs_6_0");
        assert(vsBillboardBlob != nullptr);

        // インデックス: 0=通常/非ビルボード, 1=通常/ビルボード, 2=加算/非ビルボード, 3=加算/ビルボード
        for (int i = 0; i < 4; ++i) {
            bool billboard = (i & 1) != 0;
            bool additive = (i & 2) != 0;

            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = baseDesc;
            psoDesc.VS = billboard
                ? D3D12_SHADER_BYTECODE{ vsBillboardBlob->GetBufferPointer(), vsBillboardBlob->GetBufferSize() }
                : D3D12_SHADER_BYTECODE{ vsNoBillboardBlob->GetBufferPointer(), vsNoBillboardBlob->GetBufferSize() };
            psoDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
            psoDesc.BlendState = additive ? additiveBlendDesc : alphaBlendDesc;

            hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&renderPipelineStates_[i]));
            assert(SUCCEEDED(hr));
        }
    }
}

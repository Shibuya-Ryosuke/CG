#include "InstancedModelCommon.h"
#include "../../Core/Base/DirectXCommon.h"
#include "../../Core/Base/Logger.h"
#include "../../Core/Base/ShaderCompiler.h"
#include "../../Core/Math/Geometry.h" // VertexDataのoffsetof用
#include <cassert>
#include <cstddef>

namespace RyoEngine {

    InstancedModelCommon* InstancedModelCommon::GetInstance() {
        static InstancedModelCommon instance;
        return &instance;
    }

    void InstancedModelCommon::Initialize() {
        Logger::Log("InstancedModelCommon : Initializing...\n");
        CreateRootSignature();
        CreatePipelineStates();
        Logger::LogSuccess("InstancedModelCommon : Initialized\n");
    }

    void InstancedModelCommon::Finalize() {
        Logger::Log("InstancedModelCommon : Finalizing...\n");
        for (auto& pso : pipelineStates_) {
            pso.Reset();
        }
        rootSignature_.Reset();
        Logger::LogSuccess("InstancedModelCommon : Finalized\n");
    }

    void InstancedModelCommon::BeginDraw() {
        auto commandList = DirectXCommon::GetInstance()->GetCommandList();
        commandList->SetGraphicsRootSignature(rootSignature_.Get());

        // 現在のblendMode_に対応するPSOをセット
        size_t blendIdx = static_cast<size_t>(blendMode_);
        commandList->SetPipelineState(pipelineStates_[blendIdx].Get());
    }

    void InstancedModelCommon::Draw() {
        for (const auto& command : drawCommands_) {
            command();
        }
    }

    void InstancedModelCommon::CreateRootSignature() {
        auto device = DirectXCommon::GetInstance()->GetDevice();
        HRESULT hr = S_OK;

        // テクスチャ用DescriptorRange (t0, Pixel)
        D3D12_DESCRIPTOR_RANGE textureRange[1] = {};
        textureRange[0].BaseShaderRegister = 0;
        textureRange[0].NumDescriptors = 1;
        textureRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        textureRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_PARAMETER rootParameters[7] = {};
        // 0: インスタンス配列 (StructuredBuffer。Root Descriptor SRV、Vertexのみで読む)
        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        rootParameters[0].Descriptor.ShaderRegister = 0; // t0 (Vertex)
        // 1: カメラのView-Projection行列 (CBV、Vertex)
        rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        rootParameters[1].Descriptor.ShaderRegister = 0; // b0 (Vertex)
        // 2: テクスチャ (DescriptorTable SRV、Pixel)
        rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[2].DescriptorTable.pDescriptorRanges = textureRange;
        rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(textureRange);
        // 3: マテリアル(enableLighting/shadingModeのみ。色はインスタンスごとなのでここには無い)
        rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[3].Descriptor.ShaderRegister = 0; // b0 (Pixel)
        // 4: Light配列 (Root Descriptor SRV、Pixel。LightManagerと共通)
        rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[4].Descriptor.ShaderRegister = 1; // t1 (Pixel)
        // 5: 有効ライト数 (CBV、Pixel)
        rootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[5].Descriptor.ShaderRegister = 1; // b1 (Pixel)
        // 6: アンビエントライト (CBV、Pixel)
        rootParameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[6].Descriptor.ShaderRegister = 2; // b2 (Pixel)

        D3D12_STATIC_SAMPLER_DESC staticSampler{};
        staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
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
        hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
        assert(SUCCEEDED(hr));
    }

    void InstancedModelCommon::CreatePipelineStates() {
        auto device = DirectXCommon::GetInstance()->GetDevice();
        HRESULT hr = S_OK;

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
        rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
        rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

        Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = ShaderCompiler::GetInstance()->Compile(L"HLSL/Model/Instanced.VS.hlsl", L"vs_6_0");
        assert(vertexShaderBlob != nullptr);
        Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = ShaderCompiler::GetInstance()->Compile(L"HLSL/Model/Instanced.PS.hlsl", L"ps_6_0");
        assert(pixelShaderBlob != nullptr);

        D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
        depthStencilDesc.DepthEnable = true;
        depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

        // 6つのブレンドモード分のPSOを一括生成
        for (int i = 0; i < 6; ++i) {
            BlendMode mode = static_cast<BlendMode>(i);
            D3D12_BLEND_DESC blendDesc = CreateBlendDesc(mode);

            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
            psoDesc.pRootSignature = rootSignature_.Get();
            psoDesc.InputLayout = inputLayoutDesc;
            psoDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
            psoDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
            psoDesc.BlendState = blendDesc;
            psoDesc.RasterizerState = rasterizerDesc;
            psoDesc.NumRenderTargets = 1;
            psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
            psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            psoDesc.SampleDesc.Count = 1;
            psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
            psoDesc.DepthStencilState = depthStencilDesc;
            psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

            hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineStates_[i]));
            assert(SUCCEEDED(hr));
        }
    }

    D3D12_BLEND_DESC InstancedModelCommon::CreateBlendDesc(BlendMode blendMode) {
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
#include "SpriteCommon.h"
#include "../Base/DirectXCommon.h"
#include "../Base/Logger.h"
#include "../Base/ShaderCompiler.h"
#include "../Graphics/TextureManager.h"
#include <cassert>

namespace Engine {
    SpriteCommon* SpriteCommon::GetInstance() {
        static SpriteCommon instance;
        return &instance;
    }

    void SpriteCommon::Initialize() {
        dxCommon_ = DirectXCommon::GetInstance();
        CreateRootSignature();
        CreatePipelineState();
    }

    void SpriteCommon::BeginDraw() {
        auto commandList = dxCommon_->GetCommandList();
        commandList->SetGraphicsRootSignature(rootSignature_.Get());
        commandList->SetPipelineState(graphicsPipelineState_.Get());
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        ID3D12DescriptorHeap* ppHeaps[] = { TextureManager::GetInstance()->GetDescriptorHeap() };
        commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
    }

    void SpriteCommon::Finalize() {
        // グラフィックスパイプラインを解放
        graphicsPipelineState_.Reset();

        // ルートシグネチャを解放
        rootSignature_.Reset();

        // 保持していた DirectXCommon のポインタをクリア
        dxCommon_ = nullptr;
    }

    void SpriteCommon::CreateRootSignature() {
        D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
        descriptorRange[0].BaseShaderRegister = 0;
        descriptorRange[0].NumDescriptors = 1;
        descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        // RootParameter (0:Material, 1:WVP, 2:Texture)
        D3D12_ROOT_PARAMETER rootParameters[3] = {};
        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[0].Descriptor.ShaderRegister = 0;

        rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        rootParameters[1].Descriptor.ShaderRegister = 1;

        rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
        rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

        // サンプラーの設定 (テクスチャの補間方法など)
        D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
        staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; // 線形補間[cite: 15]
        staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // 繰り返し表示[cite: 15]
        staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
        staticSamplers[0].ShaderRegister = 0; // レジスタ番号 s0
        staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
        descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        descriptionRootSignature.pParameters = rootParameters;
        descriptionRootSignature.NumParameters = _countof(rootParameters);
        descriptionRootSignature.pStaticSamplers = staticSamplers;
        descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

        // シリアライズ (バイナリ化)
        Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
        HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature,
            D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
        if (FAILED(hr)) {
            Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
            assert(false);
        }

        // ルートシグネチャの生成
        hr = dxCommon_->GetDevice()->CreateRootSignature(0,
            signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
        assert(SUCCEEDED(hr));
    }

    void SpriteCommon::CreatePipelineState() {
        // 頂点レイアウトの設定
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[2] = {};
        inputElementDescs[0].SemanticName = "POSITION";
        inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        inputElementDescs[0].AlignedByteOffset = 0;

        inputElementDescs[1].SemanticName = "TEXCOORD";
        inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
        inputElementDescs[1].AlignedByteOffset = 16;

        D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
        inputLayoutDesc.pInputElementDescs = inputElementDescs;
        inputLayoutDesc.NumElements = _countof(inputElementDescs);

        // シェーダーのコンパイル (Sprite専用のHLSLがある場合はパスを変更してください)
        Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = ShaderCompiler::GetInstance()->Compile(L"Original/HLSL/Sprite.VS.hlsl", L"vs_6_0");
        Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = ShaderCompiler::GetInstance()->Compile(L"Original/HLSL/Sprite.PS.hlsl", L"ps_6_0");

        // PSO の作成
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
        psoDesc.pRootSignature = rootSignature_.Get();
        psoDesc.InputLayout = inputLayoutDesc;
        psoDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
        psoDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };

        // --- 2D用の重要な設定 ---

        // 1. デプスステンシル: 2Dは重なり順で描画するので、奥行き判定を無効化するか、比較を「常に通過」にする
        psoDesc.DepthStencilState.DepthEnable = true; // 有効にするが
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; // 深度値を書き込む
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;// 手前のものを描画（同じ深度なら上書き）
        psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;                    // 深度バッファのフォーマット（環境に合わせてください）

        // 2. ラスタライザ: カリングをしない（裏面も見えるようにする）
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;

        // 3. ブレンドステート: アルファブレンディングを有効化[cite: 15]
        D3D12_RENDER_TARGET_BLEND_DESC& blendDesc = psoDesc.BlendState.RenderTarget[0];
        blendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        blendDesc.BlendEnable = TRUE;
        blendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        blendDesc.BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
        blendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
        blendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;

        // 書き込む RTV の情報 (DirectXCommon の設定に合わせる)[cite: 16]
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; //[cite: 16]

        // DSV の情報[cite: 16]
        psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT; //[cite: 16]

        // 形状のタイプ (三角形)
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.SampleDesc.Count = 1;
        psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

        HRESULT hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&graphicsPipelineState_));
        assert(SUCCEEDED(hr));
    }
}
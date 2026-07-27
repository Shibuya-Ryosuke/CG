#include "ReflectCommon.h"
#include "../Base/DirectXCommon.h"
#include "../Graphics/TextureManager.h"
#include "../Base/ShaderCompiler.h"
#include "../Base/Logger.h"

namespace RyoEngine {


    ReflectCommon* ReflectCommon::GetInstance() {
        static ReflectCommon instance;
        return &instance;
    }

    void ReflectCommon::Initialize() {
        Logger::Log("ReflectCommon : Initializing...\n");
        dxCommon_ = DirectXCommon::GetInstance();
        CreateRootSignature();
        CreatePipelineState();
        Logger::LogSuccess("ReflectCommon : Initialized\n");
    }


    void ReflectCommon::PreDraw(ReflectModel* mirror) {
        auto commandList = dxCommon_->GetCommandList();
        activeMirror_ = mirror;

        // ✨ mirror からリソースを取得してバリアを張る
        ID3D12Resource* res = mirror->GetResource();
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = res;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        commandList->ResourceBarrier(1, &barrier);

        // ✨ mirror から RTV ハンドルを取得してレンダーターゲットを切り替える
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mirror->GetRtvHandle();
        auto dsvHandle = dxCommon_->GetDSVHandle();
        commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

        // クリア処理
        float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
        commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        // ビューポートとシザーの設定はそのまま
        D3D12_VIEWPORT viewport = { 0.0f, 0.0f, 1280.0f, 720.0f, 0.0f, 1.0f };
        D3D12_RECT scissor = { 0, 0, 1280, 720 };
        commandList->RSSetViewports(1, &viewport);
        commandList->RSSetScissorRects(1, &scissor);
    };

    void ReflectCommon::PostDraw(ReflectModel* mirror) {
        auto commandList = dxCommon_->GetCommandList();

        // ✨ mirror からリソースを取得してバリアを戻す
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = mirror->GetResource();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        commandList->ResourceBarrier(1, &barrier);
    }

    void ReflectCommon::Finalize() {
        Logger::Log("ReflectCommon : Finalizing...\n");
        dxCommon_ = nullptr;
        rootSignature_.Reset();
        graphicsPipelineState_.Reset();
        activeMirror_ = nullptr;
        Logger::LogSuccess("ReflectCommon : Finalized\n");
    }

    void ReflectCommon::CreateRootSignature() {
        auto device = dxCommon_->GetDevice();

        // DescriptorRange: t0(通常) と t1(反射用) の2枚分を確保
        D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
        descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        descriptorRange[0].NumDescriptors = 2; // ここを2にする
        descriptorRange[0].BaseShaderRegister = 0; // t0から開始
        descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        // RootParameter設定
        D3D12_ROOT_PARAMETER rootParameters[4] = {};
        // b0: Material (ReflectMaterial)
        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[0].Descriptor.ShaderRegister = 0;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // b1: TransformationMatrix (WVP)
        rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[1].Descriptor.ShaderRegister = 0; // VSのb0
        rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

        // t0, t1: DescriptorTable (Texture)
        rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
        rootParameters[2].DescriptorTable.pDescriptorRanges = &descriptorRange[0];
        rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // b3: DirectionalLight
        rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[3].Descriptor.ShaderRegister = 1; // PSのb1
        rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // Sampler設定
        D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
        staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;  // 引き延ばす
        staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

        staticSamplers[0].MipLODBias = 0;
        staticSamplers[0].MaxAnisotropy = 1;
        staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        staticSamplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        staticSamplers[0].MinLOD = 0.0f;
        staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
        staticSamplers[0].ShaderRegister = 0; // HLSL側の register(s0) に対応
        staticSamplers[0].RegisterSpace = 0;
        staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC description = {};
        description.NumParameters = _countof(rootParameters);
        description.pParameters = rootParameters;
        description.NumStaticSamplers = _countof(staticSamplers);
        description.pStaticSamplers = staticSamplers;
        description.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
        D3D12SerializeRootSignature(&description, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
        device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
    }

    void ReflectCommon::CreatePipelineState() {
        auto device = dxCommon_->GetDevice();

        // 新しい反射シェーダーをコンパイル
        auto vsBlob = ShaderCompiler::GetInstance()->Compile(L"HLSL/Reflect/Reflect.VS.hlsl", L"vs_6_0");
        auto psBlob = ShaderCompiler::GetInstance()->Compile(L"HLSL/Reflect/Reflect.PS.hlsl", L"ps_6_0");

        // InputLayout (Modelと同じ)
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
        psoDesc.pRootSignature = rootSignature_.Get();
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
        psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };

        // ブレンド設定 (標準的な不透明)
        psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        // ラスタライザ設定
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

        // 書き込むRTVの情報
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.SampleDesc.Count = 1;
        psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

        // 深度バッファ設定
        psoDesc.DepthStencilState.DepthEnable = TRUE;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

        device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&graphicsPipelineState_));
    }
}
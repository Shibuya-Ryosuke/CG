#include "ReflectCommon.h"
#include "../Base/DirectXCommon.h"
#include "../Graphics/TextureManager.h"
#include "../Base/ShaderCompiler.h"

namespace Engine {
    

    ReflectCommon* ReflectCommon::GetInstance() {
        static ReflectCommon instance;
        return &instance;
    }

    void ReflectCommon::Initialize() {
        dxCommon_ = DirectXCommon::GetInstance();
        CreateReflectionResource();
        CreateRootSignature();
        CreatePipelineState();
    }
    

    void ReflectCommon::PreDraw() {
        auto commandList = dxCommon_->GetCommandList();

        // バリアを「レンダーターゲット」に変更
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = reflectionResource_.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        commandList->ResourceBarrier(1, &barrier);

        // 描画先を鏡テクスチャに切り替え（深度バッファは共通のものを使用）
        auto rtvHandle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
        auto dsvHandle = dxCommon_->GetDSVHandle();
        commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

        // 鏡テクスチャをクリア
        float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
        commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
        commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    }

    void ReflectCommon::PostDraw() {
        auto commandList = dxCommon_->GetCommandList();

        // バリアを「ピクセルシェーダーリソース」に戻す（これで鏡に貼れるようになる）
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = reflectionResource_.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        commandList->ResourceBarrier(1, &barrier);
    }

    void ReflectCommon::Finalize() {
        // 解放処理
    }

    D3D12_GPU_DESCRIPTOR_HANDLE ReflectCommon::GetReflectionTextureHandle() const {
        // 仮のハンドルを返す（後で実装）
        return TextureManager::GetInstance()->GetGPUHandle(srvIndex_);
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
        staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSamplers[0].ShaderRegister = 0;
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
        auto vsBlob = ShaderCompiler::GetInstance()->Compile(L"Original/HLSL/Reflect/Reflect.VS.hlsl", L"vs_6_0");
        auto psBlob = ShaderCompiler::GetInstance()->Compile(L"Original/HLSL/Reflect/Reflect.PS.hlsl", L"ps_6_0");

        // InputLayout (Object3dと同じ)
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
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;

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

    void ReflectCommon::CreateReflectionResource() {
        auto device = dxCommon_->GetDevice();

        // 1. 反射用テクスチャの設定（画面サイズに合わせる）
        D3D12_RESOURCE_DESC resDesc{};
        resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        resDesc.Width = 1280; // ウィンドウサイズ。定数やWinAppから取得
        resDesc.Height = 720;
        resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        resDesc.DepthOrArraySize = 1;
        resDesc.MipLevels = 1;
        resDesc.SampleDesc.Count = 1;
        resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

        float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
        D3D12_CLEAR_VALUE clearValue{};
        clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        memcpy(clearValue.Color, clearColor, sizeof(float) * 4);

        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, // 最初は読み取り用
            &clearValue, IID_PPV_ARGS(&reflectionResource_)
        );

        // 2. RTV(描き込み用)の作成
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.NumDescriptors = 1;
        device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap_));
        device->CreateRenderTargetView(reflectionResource_.Get(), nullptr, rtvHeap_->GetCPUDescriptorHandleForHeapStart());

        // 3. SRV(鏡に貼る用)をTextureManagerに登録
        // TextureManager側に「リソースからSRVを作る」関数がある前提です
        srvIndex_ = TextureManager::GetInstance()->RegisterResource(reflectionResource_.Get());
    }
}
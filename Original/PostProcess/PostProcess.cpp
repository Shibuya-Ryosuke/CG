#include "PostProcess.h"
#include "../Base/DirectXCommon.h"
#include "../Base/Logger.h"
#include "../Base/ShaderCompiler.h"
#include "../Graphics/TextureManager.h"
#include "../Externals/imgui/imgui.h"
#include <cassert>

namespace RyoEngine {

    PostProcess* PostProcess::GetInstance() {
        static PostProcess instance;
        return &instance;
    }

    void PostProcess::Initialize() {
        Logger::Log("PostProcess : Initializing...\n");

        CreateSceneColorResource();
        CreateRootSignatureAndPSO();

        auto device = DirectXCommon::GetInstance()->GetDevice();
        compositeParamsResource_ = DirectXCommon::CreateBufferResource(device, sizeof(CompositeParams));
        compositeParamsResource_->Map(0, nullptr, reinterpret_cast<void**>(&compositeParamsData_));
        compositeParamsData_->acesEnabled = acesEnabled_ ? 1 : 0;
        compositeParamsData_->bloomEnabled = bloomEnabled_ ? 1 : 0;
        compositeParamsData_->exposure = exposure_;

        Logger::LogSuccess("PostProcess : Initialized\n");
    }

    void PostProcess::Finalize() {
        Logger::Log("PostProcess : Finalizing...\n");
        compositeParamsResource_.Reset();
        compositeParamsData_ = nullptr;
        compositePipelineState_.Reset();
        compositeRootSignature_.Reset();
        sceneColorRtvHeap_.Reset();
        sceneColorResource_.Reset();
        Logger::LogSuccess("PostProcess : Finalized\n");
    }

    void PostProcess::CreateSceneColorResource() {
        auto dxCommon = DirectXCommon::GetInstance();
        auto device = dxCommon->GetDevice();

        // HDRシーンカラーバッファ本体 (バックバッファと同じ解像度、浮動小数点フォーマット)
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

        // RTV用ディスクリプタヒープ(専用。1枚だけ)
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.NumDescriptors = 1;
        hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&sceneColorRtvHeap_));
        assert(SUCCEEDED(hr));

        device->CreateRenderTargetView(sceneColorResource_.Get(), nullptr, sceneColorRtvHeap_->GetCPUDescriptorHandleForHeapStart());

        // TextureManagerにSRVとして登録(合成パスのピクセルシェーダーが読む用)
        // NOTE: R16G16B16A16_FLOATは通常のフォーマットなので、RegisterResourceの
        //       フォーマット上書き引数は使わなくてよい(resource自体のFormatがそのまま使える)
        sceneColorTextureHandle_ = TextureManager::GetInstance()->RegisterResource(sceneColorResource_);
    }

    void PostProcess::CreateRootSignatureAndPSO() {
        auto device = DirectXCommon::GetInstance()->GetDevice();
        HRESULT hr = S_OK;

        // シーンカラー(HDRバッファ)のSRV用DescriptorRange
        D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
        descriptorRange[0].BaseShaderRegister = 0; // t0
        descriptorRange[0].NumDescriptors = 1;
        descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_PARAMETER rootParameters[2] = {};
        // シーンカラー(HDRバッファ)
        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[0].DescriptorTable.pDescriptorRanges = descriptorRange;
        rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);
        // 合成パラメータ(HDR/ブルームのON-OFFフラグ)
        rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[1].Descriptor.ShaderRegister = 0; // b0

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
        // NOTE: 頂点バッファを使わない(SV_VertexIDだけでフルスクリーン三角形を生成する)ため、
        //       ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUTフラグは不要
        rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
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

        // シェーダーコンパイル(頂点シェーダーは頂点バッファ無し、SV_VertexIDのみでフルスクリーン三角形を生成)
        Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = ShaderCompiler::GetInstance()->Compile(L"HLSL/PostProcess/Composite.VS.hlsl", L"vs_6_0");
        assert(vertexShaderBlob != nullptr);
        Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = ShaderCompiler::GetInstance()->Compile(L"HLSL/PostProcess/Composite.PS.hlsl", L"ps_6_0");
        assert(pixelShaderBlob != nullptr);

        D3D12_BLEND_DESC blendDesc{};
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        D3D12_RASTERIZER_DESC rasterizerDesc{};
        rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE; // フルスクリーン三角形なのでカリング不要
        rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
        psoDesc.pRootSignature = compositeRootSignature_.Get();
        psoDesc.InputLayout = { nullptr, 0 }; // 頂点バッファ不要
        psoDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
        psoDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
        psoDesc.BlendState = blendDesc;
        psoDesc.RasterizerState = rasterizerDesc;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DirectXCommon::GetInstance()->GetBackBufferFormat(); // gameRenderTargetResource_と同じフォーマット
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.SampleDesc.Count = 1;
        psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
        psoDesc.DepthStencilState.DepthEnable = false; // 深度不要(フルスクリーン三角形のみ)
        psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;

        hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&compositePipelineState_));
        assert(SUCCEEDED(hr));
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

    void PostProcess::BeginScenePass() {
        auto commandList = DirectXCommon::GetInstance()->GetCommandList();

        TransitionSceneColor(D3D12_RESOURCE_STATE_RENDER_TARGET);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = sceneColorRtvHeap_->GetCPUDescriptorHandleForHeapStart();
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = DirectXCommon::GetInstance()->GetDSVHandle();

        // NOTE: ビューポート/シザーはDirectXCommon::PreDraw()が直前にバックバッファサイズへ
        //       セット済み(同じ解像度なので)そのまま使い回せる。RTVだけHDRバッファへ差し替える。
        //       深度もPreDraw()で既にクリア済みで、まだ何も書き込まれていないため再クリア不要。
        commandList->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

        float clearColor[4] = { 0.1f, 0.25f, 0.5f, 1.0f };
        commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    }

    void PostProcess::EndScenePass() {
        TransitionSceneColor(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

    void PostProcess::Composite() {
        auto commandList = DirectXCommon::GetInstance()->GetCommandList();

        // ImGuiでの切り替え内容をシェーダー用バッファへ反映
        compositeParamsData_->acesEnabled = acesEnabled_ ? 1 : 0;
        compositeParamsData_->bloomEnabled = bloomEnabled_ ? 1 : 0;
        compositeParamsData_->exposure = exposure_;

        // 描画先をgameRenderTargetResource_へ戻す
        // (DirectXCommon::PreDraw()で既にRENDER_TARGET状態・ビューポート設定済みなのでそのまま使う)
        D3D12_CPU_DESCRIPTOR_HANDLE gameRtvHandle = DirectXCommon::GetInstance()->GetGameRenderTargetRTVHandle();
        commandList->OMSetRenderTargets(1, &gameRtvHandle, false, nullptr); // 深度不要

        commandList->SetGraphicsRootSignature(compositeRootSignature_.Get());
        commandList->SetPipelineState(compositePipelineState_.Get());

        ID3D12DescriptorHeap* ppHeaps[] = { TextureManager::GetInstance()->GetDescriptorHeap() };
        commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

        commandList->SetGraphicsRootDescriptorTable(0, TextureManager::GetInstance()->GetGPUHandle(sceneColorTextureHandle_));
        commandList->SetGraphicsRootConstantBufferView(1, compositeParamsResource_->GetGPUVirtualAddress());

        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->DrawInstanced(3, 1, 0, 0); // 頂点バッファ無し、SV_VertexIDだけでフルスクリーン三角形を1枚描く
    }

    void PostProcess::DrawImGui() {
        ImGui::Begin("PostProcess");
        ImGui::SliderFloat("露出", &exposure_, 0.0f, 5.0f);
        ImGui::Checkbox("ACES トーンマッピング", &acesEnabled_);
        if (!acesEnabled_) {
            ImGui::TextDisabled("(OFF時は露出後の値を単純にクリップします)");
        }
        ImGui::Checkbox("Bloom", &bloomEnabled_);
        if (bloomEnabled_) {
            ImGui::TextDisabled("(Bloom本体は未実装のため、今チェックを入れても見た目は変わりません)");
        }
        ImGui::End();
    }
}
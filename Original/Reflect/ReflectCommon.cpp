#include "ReflectCommon.h"
#include "../Base/DirectXCommon.h"
#include "../Graphics/TextureManager.h"

namespace Engine {
    

    ReflectCommon* ReflectCommon::GetInstance() {
        static ReflectCommon instance;
        return &instance;
    }

    void ReflectCommon::Initialize() {
        dxCommon_ = DirectXCommon::GetInstance();
        CreateReflectionResource();
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
        return {};
    }

    void ReflectCommon::CreateRootSignature() {}
    void ReflectCommon::CreatePipelineState() {}

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
#include "ShadowMap.h"
#include "../Base/DirectXCommon.h"
#include "../Base/Logger.h"
#include "../Graphics/TextureManager.h"
#include "../Light/LightManager.h"
#include <cassert>
#include <cmath>

namespace RyoEngine {

    ShadowMap* ShadowMap::GetInstance() {
        static ShadowMap instance;
        return &instance;
    }

    void ShadowMap::Initialize() {
        ShadowMap* instance = GetInstance();

        Logger::Log("ShadowMap : Initializing...\n");
        auto device = DirectXCommon::GetInstance()->GetDevice();

        // 深度テクスチャ本体
        // NOTE: R32_TYPELESSで作ることで、DSVは D32_FLOAT、SRVは R32_FLOAT として
        //       同じリソースを別々に解釈できるようにしている。
        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Width = kShadowMapSize;
        resourceDesc.Height = kShadowMapSize;
        resourceDesc.MipLevels = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_HEAP_PROPERTIES heapProperties{};
        heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_CLEAR_VALUE depthClearValue{};
        depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        depthClearValue.DepthStencil.Depth = 1.0f;

        HRESULT hr = device->CreateCommittedResource(
            &heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClearValue, IID_PPV_ARGS(&instance->shadowMapResource_)
        );
        assert(SUCCEEDED(hr));
        instance->currentState_ = D3D12_RESOURCE_STATE_DEPTH_WRITE;

        // DSV用ディスクリプタヒープ(専用。1枚だけなのでメインの深度バッファとは別に持つ)
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.NumDescriptors = 1;
        hr = device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&instance->dsvHeap_));
        assert(SUCCEEDED(hr));

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        device->CreateDepthStencilView(instance->shadowMapResource_.Get(), &dsvDesc, instance->dsvHeap_->GetCPUDescriptorHandleForHeapStart());

        // TextureManagerにSRVとして登録する。
        // R32_TYPELESSのままだとSRVが作れないため、明示的にR32_FLOATを指定する。
        instance->shadowMapTextureHandle_ = TextureManager::GetInstance()->RegisterResource(instance->shadowMapResource_, DXGI_FORMAT_R32_FLOAT);

        // ライトView-Projection行列用の定数バッファ
        instance->lightVPResource_ = DirectXCommon::CreateBufferResource(device, sizeof(Matrix4x4));
        instance->lightVPResource_->Map(0, nullptr, reinterpret_cast<void**>(&instance->lightVPData_));

        UpdateLightViewProj();

        Logger::LogSuccess("ShadowMap : Initialized\n");
    }

    void ShadowMap::Finalize() {
        ShadowMap* instance = GetInstance();

        Logger::Log("ShadowMap : Finalizing...\n");
        instance->lightVPResource_.Reset();
        instance->lightVPData_ = nullptr;
        instance->dsvHeap_.Reset();
        instance->shadowMapResource_.Reset();
        Logger::LogSuccess("ShadowMap : Finalized\n");
    }

    void ShadowMap::TransitionTo(D3D12_RESOURCE_STATES newState, ID3D12GraphicsCommandList* commandList) {
        ShadowMap* instance = GetInstance();
        if (instance->currentState_ == newState) {
            return;
        }
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = instance->shadowMapResource_.Get();
        barrier.Transition.StateBefore = instance->currentState_;
        barrier.Transition.StateAfter = newState;
        commandList->ResourceBarrier(1, &barrier);
        instance->currentState_ = newState;
    }

    void ShadowMap::UpdateLightViewProj() {
        ShadowMap* instance = GetInstance();
        // 現在のLightManagerの代表ライト(Light 0)の向きから、ライト視点のView-Projectionを作る
        Vector3 direction = Normalize(LightManager::GetInstance()->GetDirection());

        // ターゲット位置から、光の来る方向(-direction)へlightDistance_だけ離した位置を仮想的な光源位置にする
        Vector3 eye = instance->targetPosition_ - direction * instance->lightDistance_;

        // directionがほぼ真上/真下を向いていると up と平行になり LookAt が破綻するため回避する
        Vector3 up = { 0.0f, 1.0f, 0.0f };
        if (std::abs(Dot(direction, up)) > 0.99f) {
            up = { 0.0f, 0.0f, 1.0f };
        }

        Matrix4x4 view = MakeLookAtMatrix(eye, instance->targetPosition_, up);
        Matrix4x4 proj = MakeOrthographicMatrix(-instance->orthoHalfSize_, instance->orthoHalfSize_, instance->orthoHalfSize_, -instance->orthoHalfSize_, instance->nearZ_, instance->farZ_);

        *instance->lightVPData_ = view * proj;
    }

    void ShadowMap::BeginShadowPass() {
        ShadowMap* instance = GetInstance();
        auto commandList = DirectXCommon::GetInstance()->GetCommandList();

        TransitionTo(D3D12_RESOURCE_STATE_DEPTH_WRITE, commandList);

        // 描画直前に、現在のライトの向きを反映したView-Projectionへ更新する
        UpdateLightViewProj();

        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = instance->dsvHeap_->GetCPUDescriptorHandleForHeapStart();
        // カラーバッファ無し(深度のみ)でレンダーターゲットを設定する
        commandList->OMSetRenderTargets(0, nullptr, false, &dsvHandle);
        commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        D3D12_VIEWPORT viewport{};
        viewport.Width = static_cast<float>(kShadowMapSize);
        viewport.Height = static_cast<float>(kShadowMapSize);
        viewport.TopLeftX = 0;
        viewport.TopLeftY = 0;
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        commandList->RSSetViewports(1, &viewport);

        D3D12_RECT scissorRect{};
        scissorRect.left = 0;
        scissorRect.top = 0;
        scissorRect.right = static_cast<LONG>(kShadowMapSize);
        scissorRect.bottom = static_cast<LONG>(kShadowMapSize);
        commandList->RSSetScissorRects(1, &scissorRect);
    }

    void ShadowMap::EndShadowPass() {
        auto commandList = DirectXCommon::GetInstance()->GetCommandList();
        // 通常描画パスのピクセルシェーダーから読めるように状態を遷移させる
        TransitionTo(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, commandList);
    }
}

#include "ShadowMap.h"
#include "../../Core/Base/DirectXCommon.h"
#include "../../Core/Base/Logger.h"
#include "../../Graphics/2D/TextureManager.h"
#include "../Light/LightManager.h"
#include "../../Camera/CameraManager.h"
#include "../../Externals/imgui/imgui.h"
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
        auto device = DirectXCommon::GetDevice();

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
        instance->shadowMapTextureHandle_ = TextureManager::RegisterResource(instance->shadowMapResource_, DXGI_FORMAT_R32_FLOAT);

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

    void ShadowMap::UpdateTargetPosition() {
        ShadowMap* instance = GetInstance();

        // 描画カメラ(デバッグカメラ有効時はデバッグカメラ)か、常にゲームカメラか
        const Camera& camera = instance->followDebugCamera_ ? CameraManager::GetActive() : CameraManager::GetGameCamera();

        // 見下ろし時に中心が地面の下へもぐらないよう、前方向は水平成分だけ使う
        Vector3 forward = camera.GetForward();
        forward.y = 0.0f;
        Vector3 center = camera.GetWorldPosition();
        const float length = Length(forward);
        if (length > 1e-4f) {
            center = center + forward / length * instance->forwardOffset_;
        }
        instance->targetPosition_ = center;
    }

    void ShadowMap::UpdateLightViewProj() {
        ShadowMap* instance = GetInstance();
        // 現在のLightManagerの代表ライト(Light 0)の向きから、ライト視点のView-Projectionを作る
        Vector3 direction = Normalize(LightManager::GetDirection());

        // directionがほぼ真上/真下を向いていると up と平行になり LookAt が破綻するため回避する
        Vector3 up = { 0.0f, 1.0f, 0.0f };
        if (std::abs(Dot(direction, up)) > 0.99f) {
            up = { 0.0f, 0.0f, 1.0f };
        }

        if (instance->followCamera_) {
            // カメラが少し動くたびに影の輪郭がチラつく(シマー)のを防ぐため、
            // ライト空間で中心座標をシャドウマップ1テクセルの大きさ単位に丸める
            const Matrix4x4 lightRotation = MakeLookAtMatrix({ 0.0f, 0.0f, 0.0f }, direction, up);
            const float texelSize = instance->orthoHalfSize_ * 2.0f / static_cast<float>(kShadowMapSize);
            Vector3 centerLS = TransformPoint(instance->targetPosition_, lightRotation);
            centerLS.x = std::floor(centerLS.x / texelSize) * texelSize;
            centerLS.y = std::floor(centerLS.y / texelSize) * texelSize;
            instance->targetPosition_ = TransformPoint(centerLS, Inverse(lightRotation));
        }

        // ターゲット位置から、光の来る方向(-direction)へlightDistance_だけ離した位置を仮想的な光源位置にする
        Vector3 eye = instance->targetPosition_ - direction * instance->lightDistance_;

        Matrix4x4 view = MakeLookAtMatrix(eye, instance->targetPosition_, up);
        Matrix4x4 proj = MakeOrthographicMatrix(-instance->orthoHalfSize_, instance->orthoHalfSize_, instance->orthoHalfSize_, -instance->orthoHalfSize_, instance->nearZ_, instance->farZ_);

        *instance->lightVPData_ = view * proj;
    }

    void ShadowMap::BeginShadowPass() {
        ShadowMap* instance = GetInstance();
        auto commandList = DirectXCommon::GetCommandList();

        TransitionTo(D3D12_RESOURCE_STATE_DEPTH_WRITE, commandList);

        // カメラ追従ON時は、影を落とす範囲の中心をカメラ位置から求め直す
        // (Initialize()時点ではCameraManagerが未初期化なので、ここで行う)
        if (instance->followCamera_) {
            UpdateTargetPosition();
        }

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
        auto commandList = DirectXCommon::GetCommandList();
        // 通常描画パスのピクセルシェーダーから読めるように状態を遷移させる
        TransitionTo(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, commandList);
    }

    void ShadowMap::DrawImGui() {
#ifdef NRELEASE
        ShadowMap* instance = GetInstance();

        ImGui::Begin("Shadow");

        ImGui::Text("解像度 : %u x %u", kShadowMapSize, kShadowMapSize);
        ImGui::Text("範囲   : %.1f x %.1f", instance->orthoHalfSize_ * 2.0f, instance->orthoHalfSize_ * 2.0f);
        ImGui::DragFloat("範囲の半分", &instance->orthoHalfSize_, 0.5f, 1.0f, 500.0f);
        ImGui::DragFloat("光源までの距離", &instance->lightDistance_, 0.5f, 1.0f, 1000.0f);
        ImGui::DragFloat("Near", &instance->nearZ_, 0.1f, 0.01f, instance->farZ_ - 0.01f);
        ImGui::DragFloat("Far", &instance->farZ_, 0.5f, instance->nearZ_ + 0.01f, 2000.0f);

        ImGui::Separator();
        ImGui::Checkbox("カメラに追従", &instance->followCamera_);
        if (instance->followCamera_) {
            ImGui::Checkbox("デバッグカメラ中はデバッグカメラに追従", &instance->followDebugCamera_);
            ImGui::DragFloat("前方オフセット", &instance->forwardOffset_, 0.5f, -500.0f, 500.0f);
        } else {
            ImGui::DragFloat3("中心座標", &instance->targetPosition_.x, 0.5f);
        }
        ImGui::Text("現在の中心 : (%.2f, %.2f, %.2f)", instance->targetPosition_.x, instance->targetPosition_.y, instance->targetPosition_.z);

        ImGui::End();
#endif
    }
}

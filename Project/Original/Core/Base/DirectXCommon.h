#pragma once
#include "../../Graphics/2D/TextureManager.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <cstdint>

class ShaderCompiler;

namespace RyoEngine {

	class DirectXCommon {
	public:
		// インスタンス取得
		static DirectXCommon* GetInstance();

		/// <summary>
		/// DirectX初期化
		/// </summary>
		void Initialize(class WinApp* winApp, int32_t width = 1280, int32_t height = 720);

		/// <summary>
		/// 解放
		/// </summary>
		void Finalize();

		/// <summary>
		/// フレーム開始
		/// </summary>
		void PreDraw();

		/// <summary>
		/// フレーム終了
		/// </summary>
		void PostDraw();

		void CreateGameRenderTarget();

		void WaitForFence();
		// ディスクリプタヒープの生成
		static Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
			ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

		// CPUのDescriptorHandleを取得
		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(
			const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index) const;


		static Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(const Microsoft::WRL::ComPtr<ID3D12Device> device, size_t sizeInBytes);

		// ゲッター
		ID3D12Device* GetDevice() const { return device_.Get(); }
		ID3D12GraphicsCommandList* GetCommandList() const { return commandList_.Get(); }
		D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const {
			return dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
		}
		int32_t GetBackBufferWidth() const { return backBufferWidth_; }
		int32_t GetBackBufferHeight() const { return backBufferHeight_; }
		uint32_t GetBackBufferCount() const { return 2; }
		DXGI_FORMAT GetBackBufferFormat() const { return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; }

		// メインループでImGui::Imageに渡すためのGPUハンドルを取得するゲッター
		D3D12_GPU_DESCRIPTOR_HANDLE GetGameTextureGPUHandle() const {
			return TextureManager::GetInstance()->GetGPUHandle(gameTextureHandle_);
		}

		// PostProcess::Composite()が、HDR合成後の結果をgameRenderTargetResource_へ
		// 書き戻す際にOMSetRenderTargetsで使うためのRTV CPUハンドル
		D3D12_CPU_DESCRIPTOR_HANDLE GetGameRenderTargetRTVHandle() const {
#ifdef _DEBUG
			// Debug時: ImGui表示用のゲームテクスチャのRTVを返す
			return gameRtvHeap_->GetCPUDescriptorHandleForHeapStart();
#else
			// Release時: スワップチェーンの現在のバックバッファのRTVを返す
			uint32_t backBufferIndex = swapChain_->GetCurrentBackBufferIndex();
			const uint32_t descriptorSizeRTV = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			return GetCPUDescriptorHandle(rtvHeap_, descriptorSizeRTV, backBufferIndex);
#endif
		}

	private:
		DirectXCommon() = default;
		~DirectXCommon() = default;

		// デバイス周り
		Microsoft::WRL::ComPtr<ID3D12Device> device_;
		Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_;

		// コマンド周り
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;

		// スワップチェーン、レンダーターゲット
		Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_;
		Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources_[2];
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_;

		// 深度バッファ
		Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource_;

		// フェンス、イベント
		Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
		uint64_t fenceValue_ = 0;
		HANDLE fenceEvent_ = nullptr;

		// 画面サイズ保持
		int32_t backBufferWidth_ = 0;
		int32_t backBufferHeight_ = 0;

		// DSV用ディスクリプタヒープ
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_;

		// ゲーム画面のリソース系 
		Microsoft::WRL::ComPtr<ID3D12Resource> gameRenderTargetResource_;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> gameRtvHeap_;
		uint32_t gameTextureHandle_ = 0;

		// DSV作成用の内部関数
		void CreateDepthStencilView();
	};
}
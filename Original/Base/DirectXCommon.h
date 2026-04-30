#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <cstdint>

namespace Engine {

	class DirectXCommon {
	public:
		// インスタンス取得
		static DirectXCommon* GetInstance();

		/// <summary>
		/// DirectX初期化
		/// </summary>
		void Initialize(class WinApp* winApp, int32_t width, int32_t height);

		/// <summary>
		/// フレーム開始
		/// </summary>
		void PreRender();

		/// <summary>
		/// フレーム終了
		/// </summary>
		void PostRender();

		// ディスクリプタヒープの生成
		static Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
			ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

		// CPUのDescriptorHandleを取得
		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(
			const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index);


		// ゲッター
		ID3D12Device* GetDevice() const { return device_.Get(); };
		ID3D12GraphicsCommandList* GetCommandList() const { return commandList_.Get(); };


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
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap_;

		// フェンス、イベント
		Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
		uint64_t fenceValue_ = 0;
		HANDLE fenceEvent_ = nullptr;

		// 画面サイズ保持
		int32_t backBufferWidth_ = 0;
		int32_t backBufferHeight_ = 0;
	};
}
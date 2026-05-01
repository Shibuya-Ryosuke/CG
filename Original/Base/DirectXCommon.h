#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <cstdint>

class ShaderCompiler;

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

		// ディスクリプタヒープの生成
		static Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
			ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

		// CPUのDescriptorHandleを取得
		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(
			const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index);


		static Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(const Microsoft::WRL::ComPtr<ID3D12Device> device, size_t sizeInBytes);

		// ゲッター
		ID3D12Device* GetDevice() const { return device_.Get(); };
		ID3D12GraphicsCommandList* GetCommandList() const { return commandList_.Get(); };
		D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const {
			return dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
		}
		int32_t GetBackBufferWidth() { return backBufferWidth_; };
		int32_t GetBackBufferHeight() { return backBufferHeight_; };


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

		// DSV作成用の内部関数
		void CreateDepthStencilView();
	};
}
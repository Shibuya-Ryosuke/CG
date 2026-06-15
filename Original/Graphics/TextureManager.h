#pragma once
#include <d3d12.h>
#include <string>
#include <unordered_map>
#include <wrl.h>
#include <vector>

#include "../Externals/DirectXTex/DirectXTex.h"
#pragma comment(lib, "DirectXTex.lib")

namespace RyoEngine {
	class TextureManager {
	public:
		// インスタンス取得
		static TextureManager* GetInstance();

		/// <summary>
		/// 初期化
		/// </summary>
		/// <param name="device"></param>
		void Initialize();

		/// <summary>
		/// 解放
		/// </summary>
		void Finalize();


		/// <summary>
		/// テクスチャの読み込み
		/// </summary>
		/// <param name="filePath">ファイルパス</param>
		/// <returns>該当テクスチャが格納された場所を示すインデックス</returns>
		uint32_t Load(const std::string& filePath);

		void ClearIntermediateResources();

		// ゲッター
		ID3D12Resource* GetResource(uint32_t handle) const {
			assert(handle < textures_.size());
			return textures_[handle].resource.Get();
		}

		ID3D12DescriptorHeap* GetDescriptorHeap() const {
			return descriptorHeap_.Get();
		}

		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(uint32_t index) const {
			assert(index < kMaxTextures);
			D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap_->GetCPUDescriptorHandleForHeapStart();
			handleCPU.ptr += static_cast<size_t>(descriptorSize_) * index;
			return handleCPU;
		}

		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(uint32_t index) const {
			assert(index < kMaxTextures); // 最大数を超えていないかチェック

			// 1. ヒープの先頭住所を取得
			D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap_->GetGPUDescriptorHandleForHeapStart();

			// 2. インデックス分だけ後ろにずらす
			// (1つあたりのサイズ * 何番目か)
			handleGPU.ptr += static_cast<unsigned long long>(descriptorSize_) * index;

			return handleGPU;
		}

		uint32_t GetWhiteTex() { return whiteTex; }

		// 外部で作ったリソースを登録してインデックスを返す
		uint32_t RegisterResource(Microsoft::WRL::ComPtr<ID3D12Resource> resource);

	private:
		TextureManager() = default;
		~TextureManager() = default;
		TextureManager(const TextureManager&) = delete;
		TextureManager& operator=(const TextureManager&) = delete;


		struct Texture {
			Microsoft::WRL::ComPtr<ID3D12Resource> resource;
			// 必要に応じて SRV の場所などもここに保持
		};

		ID3D12Device* device_ = nullptr;
		std::vector<Texture> textures_; // インデックスがハンドルになる
		std::unordered_map<std::string, uint32_t> filePathMap_; // 重複読み込み防止
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> intermediateResources_;  // 中間リソースを一時的に貯めておく

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap_; // SRV用の棚
		uint32_t descriptorSize_ = 0;                                 // 1マス分のサイズ
		const size_t kMaxTextures = 128;                             // 最大数（任意）

		// Textureデータ読み込み
		DirectX::ScratchImage LoadTexture(const std::string& filePath);

		// TextureResource作成
		Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const DirectX::TexMetadata& metadata);

		// TextureResourceにデータを転送
		[[nodiscard]]  // 戻り値を破棄してはならない(破棄したら警告)
		Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(
			const Microsoft::WRL::ComPtr<ID3D12Resource> texture, const DirectX::ScratchImage& mipImages,
			const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList
		);


		uint32_t whiteTex = 0;
	};
}
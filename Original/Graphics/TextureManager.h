#pragma once
#include <d3d12.h>
#include <string>
#include <unordered_map>
#include <wrl.h>
#include <vector>

#include "../Externals/DirectXTex/DirectXTex.h"
#pragma comment(lib, "DirectXTex.lib")

namespace Engine {
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
		ID3D12Resource* GetResource(uint32_t handle) {
			assert(handle < textures_.size());
			return textures_[handle].resource.Get();
		}

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

	};
}
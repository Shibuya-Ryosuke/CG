#pragma once
#include <d3d12.h>
#include <string>
#include <unordered_map>
#include <wrl.h>
#include <vector>
#include <cassert>

#include "../../Externals/DirectXTex/DirectXTex.h"

namespace RyoEngine {
	/// <summary>
	/// 画像ファイルの読み込み・GPUへの転送を担当するクラス。
	/// ディスクリプタヒープの管理はSrvManagerに移譲した。
	/// Load()が返すハンドルはSrvManagerのスロット番号(index)そのものなので、
	/// SrvManager::GetGPUHandle(handle) でも TextureManager::GetGPUHandle(handle) でも同じ場所を指す。
	/// </summary>
	class TextureManager {
	public:
		/// <summary>
		/// 初期化 (SrvManager::Initialize()より後に呼ぶこと)
		/// </summary>
		static void Initialize();

		/// <summary>
		/// 解放 (SrvManager::Finalize()より前に呼ぶこと)
		/// </summary>
		static void Finalize();


		/// <summary>
		/// テクスチャの読み込み
		/// </summary>
		/// <param name="filePath">ファイルパス</param>
		/// <param name="isSRGB">true:色のテクスチャ(sRGBとして扱う) / false:法線マップなど色ではないデータ(リニアのまま扱う)</param>
		/// <returns>テクスチャのSRVが格納されたSrvManagerのスロット番号</returns>
		static uint32_t Load(const std::string& filePath, bool isSRGB = true);

		/// <summary>
		/// メモリ上の画像ファイルの中身 (png / jpg など) からテクスチャを読み込む。glb に埋め込まれた画像用。
		/// 読めなかったときは white1x1 を返す
		/// </summary>
		/// <param name="cacheKey">同じ画像を2回読まないためのキー (Load のファイルパスと同じ役目)</param>
		/// <param name="data">画像ファイルの中身</param>
		/// <param name="sizeInBytes">data のバイト数</param>
		/// <param name="isSRGB">Load と同じ</param>
		static uint32_t LoadFromMemory(const std::string& cacheKey, const void* data, size_t sizeInBytes, bool isSRGB = true);

		/// <summary>
		/// メモリ上の BGRA (1画素4バイト) の画素からテクスチャを作る。圧縮されていない埋め込み画像用
		/// </summary>
		static uint32_t LoadFromPixelsBGRA(const std::string& cacheKey, const void* pixels, uint32_t width, uint32_t height, bool isSRGB = true);

		static void ClearIntermediateResources();

		// ゲッター
		static ID3D12Resource* GetResource(uint32_t handle);

		static uint32_t GetWhiteTex() { return GetInstance()->whiteTex_; }

		// --- 互換用(既存コードがそのままコンパイルできるように残している) ---
		// 新しく書くコードではSrvManagerの同名関数を直接使うこと。
		static ID3D12DescriptorHeap* GetDescriptorHeap();
		static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(uint32_t index);
		static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(uint32_t index);

		// 外部で作ったリソースを登録してSRVのスロット番号を返す(互換用。中身はSrvManagerへの委譲)
		// srvFormatOverride省略時(DXGI_FORMAT_UNKNOWN)はresource自体のFormatをSRVに使う。
		// DXGI_FORMAT_R32_TYPELESS等、TYPELESSなリソース(シャドウマップの深度テクスチャ等)を登録する場合は、
		// SRVとして解釈させたい具体的なフォーマット(例：DXGI_FORMAT_R32_FLOAT)を明示的に渡すこと。
		static uint32_t RegisterResource(Microsoft::WRL::ComPtr<ID3D12Resource> resource, DXGI_FORMAT srvFormatOverride = DXGI_FORMAT_UNKNOWN);

	private:
		// インスタンス取得
		static TextureManager* GetInstance();

		TextureManager() = default;
		~TextureManager() = default;
		TextureManager(const TextureManager&) = delete;
		TextureManager& operator=(const TextureManager&) = delete;

		ID3D12Device* device_ = nullptr;
		// SrvManagerのスロット番号 → リソース実体
		std::unordered_map<uint32_t, Microsoft::WRL::ComPtr<ID3D12Resource>> textures_;
		std::unordered_map<std::string, uint32_t> filePathMap_; // 重複読み込み防止
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> intermediateResources_;  // 中間リソースを一時的に貯めておく

		// Textureデータ読み込み(isSRGB=falseならsRGB変換をせず、ミップもリニアのまま平均する)
		DirectX::ScratchImage LoadTexture(const std::string& filePath, bool isSRGB);

		// 読み込んだ画像にミップマップを付ける (name はエラー表示用)
		DirectX::ScratchImage GenerateMipChain(DirectX::ScratchImage&& image, bool isSRGB, const std::string& name);

		// ミップマップ付きの画像をGPUへ送り、SRVを作ってキャッシュに登録する (メモリからの読み込み用)
		uint32_t RegisterImage(const DirectX::ScratchImage& mipImages, const std::string& cacheKey, bool isSRGB);

		// TextureResource作成(isSRGBに応じて、フォーマットをsRGB版/リニア版にそろえる)
		Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const DirectX::TexMetadata& metadata, bool isSRGB);

		// TextureResourceにデータを転送
		[[nodiscard]]  // 戻り値を破棄してはならない(破棄したら警告)
		Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(
			const Microsoft::WRL::ComPtr<ID3D12Resource> texture, const DirectX::ScratchImage& mipImages,
			const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList
		);


		uint32_t whiteTex_ = 0;
	};
}
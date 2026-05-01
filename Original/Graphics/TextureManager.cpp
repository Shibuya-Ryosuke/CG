#include "TextureManager.h"
#include "../Base/Logger.h"
#include "../Base/DirectXCommon.h"
#include "../Externals/DirectXTex/d3dx12.h"


namespace Engine {
    TextureManager* TextureManager::GetInstance() {
        static TextureManager instance;
        return &instance;
    }

    void TextureManager::Initialize() {
        device_ = DirectXCommon::GetInstance()->GetDevice();
    }

    void TextureManager::Finalize() {
        // 中間リソース解放
        intermediateResources_.clear();

        // テクスチャの実体解放
        textures_.clear();

        // 重複読み込み防止用のマップをクリア
        filePathMap_.clear();

        // デバイスポインタを初期化
        device_ = nullptr;
    }

    uint32_t TextureManager::Load(const std::string& filePath) {
        // 1. すでに読み込んでいたらそのハンドルを返す
        if (filePathMap_.contains(filePath)) {
            return filePathMap_[filePath];
        }

        // 2. 提示された関数群を使ってリソース作成
        DirectX::ScratchImage mipImages = LoadTexture(filePath);
        auto resource = CreateTextureResource(mipImages.GetMetadata());

        // 3. 転送 (CommandListが必要)
        // ここで返ってくる intermediateResource は、
        // 「今流しているコマンド」が完了するまでクラス内で保持しておく必要がある
        auto intermediate = UploadTextureData(resource, mipImages, DirectXCommon::GetInstance()->GetCommandList());
        intermediateResources_.push_back(intermediate);

        // 4. textures_ に追加して、その index を返す
        uint32_t handle = static_cast<uint32_t>(textures_.size());
        textures_.push_back({ resource });
        filePathMap_[filePath] = handle;

        return handle;
    }

    void TextureManager::ClearIntermediateResources() {
        intermediateResources_.clear();
    }


    DirectX::ScratchImage TextureManager::LoadTexture(const std::string& filePath) {
        // テクスチャファイルを読んでプログラムで使えるようにする
        DirectX::ScratchImage image{};
        std::wstring filePathW = Logger::ConvertString(filePath);
        HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
        assert(SUCCEEDED(hr));

        // ミニマップの作成
        DirectX::ScratchImage mipImages{};
        hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
        assert(SUCCEEDED(hr));

        // ミップマップ付きのデータを返す
        return mipImages;
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> TextureManager::CreateTextureResource(const DirectX::TexMetadata& metadata) {
        // metadataを基にResourceの設定
        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Width = UINT(metadata.width);  // Textureの幅
        resourceDesc.Height = UINT(metadata.height);  // Textureの高さ
        resourceDesc.MipLevels = UINT16(metadata.mipLevels);  // mipmapの数
        resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize);  // 奥行き or 配列Textureの数
        resourceDesc.Format = metadata.format;  // TextureのFormat
        resourceDesc.SampleDesc.Count = 1;  // サンプリングのカウント。1固定
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);  // Textureの次元数

        // 利用するHeapの設定。(一般的な設定)
        D3D12_HEAP_PROPERTIES heapProperties{};
        heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;  // VRAMに配置の時はUNKNOWN
        heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;  // 上に同じ

        // Resourceの生成
        Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
        HRESULT hr = device_->CreateCommittedResource(
            &heapProperties,  // Heapの設定
            D3D12_HEAP_FLAG_NONE,  // Heapの特殊な設定。特になし
            &resourceDesc,  // Resourceの設定
            D3D12_RESOURCE_STATE_COPY_DEST,  // データ転送される設定
            nullptr,  // Clear最適値。使わないのでnullptr
            IID_PPV_ARGS(&resource)  // 作成するResourceポインタへのポインタ
        );
        assert(SUCCEEDED(hr));
        return resource;
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> TextureManager::UploadTextureData(
        const Microsoft::WRL::ComPtr<ID3D12Resource> texture, const DirectX::ScratchImage& mipImages,
        const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList)
    {
        // 中間リソースを作成
        std::vector<D3D12_SUBRESOURCE_DATA> subresources;
        DirectX::PrepareUpload(device_, mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresources);
        uint64_t intermediateSize = GetRequiredIntermediateSize(texture.Get(), 0, UINT(subresources.size()));
        Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource = DirectXCommon::CreateBufferResource(device_, intermediateSize);
        // Textureに転送
        UpdateSubresources(commandList.Get(), texture.Get(), intermediateResource.Get(), 0, 0, UINT(subresources.size()), subresources.data());
        // Textureへの転送後は利用できるよう、D3D12_RESOURCE_STATE_COPY_DEST(StateBefore)から
        // D3D12_RESOURCE_STATE_GENERIC_READ(StateAfter)へ変更
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = texture.Get();
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
        commandList->ResourceBarrier(1, &barrier);
        return intermediateResource;
    }
}

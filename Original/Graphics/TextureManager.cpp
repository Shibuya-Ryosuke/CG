#include "TextureManager.h"
#include "../Base/Logger.h"
#include "../Base/DirectXCommon.h"
#include "../Externals/DirectXTex/d3dx12.h"


namespace RyoEngine {
    TextureManager* TextureManager::GetInstance() {
        static TextureManager instance;
        return &instance;
    }

    void TextureManager::Initialize() {
        Logger::Log("TexManager : Initializing...\n");
        device_ = DirectXCommon::GetInstance()->GetDevice();

        // 1. ヒープの設定
        D3D12_DESCRIPTOR_HEAP_DESC srvDescriptorHeapDesc = {};
        srvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; // SRV用
        srvDescriptorHeapDesc.NumDescriptors = static_cast<UINT>(kMaxTextures); // 最大数(128など)
        srvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // シェーダーから見えるように

        // 2. 設定を元にヒープを生成
        HRESULT hr = device_->CreateDescriptorHeap(&srvDescriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap_));
        assert(SUCCEEDED(hr));

        // 3. 1マス分のサイズを取得しておく（GetGPUHandleで使用するため）
        descriptorSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        // 0番目のスロットをImGui用に予約
        textures_.push_back({ nullptr });

        whiteTex = Load("resources/white1x1.png");

        Logger::LogSuccess("Input : Initialized\n");
    }

    void TextureManager::Finalize() {
        Logger::Log("TexManager : Finalizing...\n");
        // 中間リソース解放
        intermediateResources_.clear();

        // テクスチャの実体解放
        textures_.clear();

        // 重複読み込み防止用のマップをクリア
        filePathMap_.clear();

        // ディスクリプタヒープの開放
        descriptorHeap_.Reset();

        // デバイスポインタを初期化
        device_ = nullptr;
        Logger::LogSuccess("TexManager : Finalized\n");
    }

    uint32_t TextureManager::Load(const std::string& filePath) {
        assert(textures_.size() < kMaxTextures);

        if (filePathMap_.contains(filePath)) return filePathMap_[filePath];

        DirectX::ScratchImage mipImages = LoadTexture(filePath);
        auto resource = CreateTextureResource(mipImages.GetMetadata());
        auto intermediate = UploadTextureData(resource, mipImages, DirectXCommon::GetInstance()->GetCommandList());
        intermediateResources_.push_back(intermediate);

        uint32_t handle = static_cast<uint32_t>(textures_.size());
        textures_.push_back({ resource });
        filePathMap_[filePath] = handle;

        // --- ここを追加：SRVの作成 ---
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = resource->GetDesc().Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels = resource->GetDesc().MipLevels;

        // ヒープの該当する場所のハンドルを取得
        D3D12_CPU_DESCRIPTOR_HANDLE lCpuHandle = descriptorHeap_->GetCPUDescriptorHandleForHeapStart();
        lCpuHandle.ptr += static_cast<unsigned long long>(descriptorSize_) * handle;

        // SRVの生成
        device_->CreateShaderResourceView(resource.Get(), &srvDesc, lCpuHandle);
        // ----------------------------

        return handle;
    }

    void TextureManager::ClearIntermediateResources() {
        intermediateResources_.clear();
    }


    uint32_t TextureManager::RegisterResource(Microsoft::WRL::ComPtr<ID3D12Resource> resource) {
        // 現在のテクスチャ配列の末尾をインデックスとする
        uint32_t index = static_cast<uint32_t>(textures_.size());
        assert(index < kMaxTextures);

        Texture texture;
        texture.resource = resource;
        textures_.push_back(texture);

        // シェーダーリソースビュー (SRV) の設定
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = resource->GetDesc().Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels = 1;

        // CPUハンドルを取得してSRVを作成
        device_->CreateShaderResourceView(
            resource.Get(),
            &srvDesc,
            GetCPUHandle(index)
        );

        return index;
    }

    DirectX::ScratchImage TextureManager::LoadTexture(const std::string& filePath) {
        // テクスチャファイルを読んでプログラムで使えるようにする
        DirectX::ScratchImage image{};
        std::wstring filePathW = Logger::ConvertString(filePath);
        HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
        assert(SUCCEEDED(hr));

        DirectX::ScratchImage mipImages{};
        mipImages = std::move(image);

        // 元画像サイズのまま出している。ミップマップを作っていない(RailSTGの時に変えた。戻しても良い)

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

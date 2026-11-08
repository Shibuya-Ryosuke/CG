#include "TextureManager.h"
#include "../../Core/Base/Logger.h"
#include "../../Core/Base/DirectXCommon.h"
#include "../../Core/Base/SrvManager.h"
#include "../../Externals/DirectXTex/d3dx12.h"


namespace RyoEngine {
    TextureManager* TextureManager::GetInstance() {
        static TextureManager instance;
        return &instance;
    }

    void TextureManager::Initialize() {
        TextureManager* instance = GetInstance();

        Logger::Log("TexManager : Initializing...\n");
        instance->device_ = DirectXCommon::GetDevice();

        // ヒープの生成・ImGui用スロットの予約はSrvManager/ImGuiManager側の責務になったので、ここでは行わない
        assert(SrvManager::GetDescriptorHeap() != nullptr && "SrvManager::Initialize()を先に呼ぶこと");

        instance->whiteTex_ = Load("Resources/EngineResources/white1x1.png");

        Logger::LogSuccess("TexManager : Initialized\n");
    }

    void TextureManager::Finalize() {
        TextureManager* instance = GetInstance();

        Logger::Log("TexManager : Finalizing...\n");
        // 中間リソース解放
        instance->intermediateResources_.clear();

        // 確保していたスロットをSrvManagerへ返却してから、テクスチャの実体を解放
        for (auto& [index, resource] : instance->textures_) {
            SrvManager::Free(index);
        }
        instance->textures_.clear();

        // 重複読み込み防止用のマップをクリア
        instance->filePathMap_.clear();

        // デバイスポインタを初期化
        instance->device_ = nullptr;
        Logger::LogSuccess("TexManager : Finalized\n");
    }

    uint32_t TextureManager::Load(const std::string& filePath, bool isSRGB) {
        TextureManager* instance = GetInstance();

        // 同じ画像でもsRGB版とリニア版は別物として扱うため、キャッシュのキーを分ける
        const std::string cacheKey = isSRGB ? filePath : filePath + "?linear";

        // 読み込み済みならそのまま返す(上限チェックより先に行うことで、満杯時でも既存テクスチャは取得できる)
        if (auto it = instance->filePathMap_.find(cacheKey); it != instance->filePathMap_.end()) {
            return it->second;
        }

        DirectX::ScratchImage mipImages = instance->LoadTexture(filePath, isSRGB);
        auto resource = instance->CreateTextureResource(mipImages.GetMetadata(), isSRGB);
        auto intermediate = instance->UploadTextureData(resource, mipImages, DirectXCommon::GetCommandList());
        instance->intermediateResources_.push_back(intermediate);

        // SRVのスロットを確保して生成(上限チェックはSrvManager::Allocate()内で行う)
        uint32_t handle = SrvManager::Allocate();
        SrvManager::CreateSRVForTexture2D(handle, resource.Get(), resource->GetDesc().Format);

        instance->textures_[handle] = resource;
        instance->filePathMap_[cacheKey] = handle;

        return handle;
    }

    void TextureManager::ClearIntermediateResources() {
        TextureManager* instance = GetInstance();

        instance->intermediateResources_.clear();
    }

    ID3D12Resource* TextureManager::GetResource(uint32_t handle) {
        TextureManager* instance = GetInstance();

        auto it = instance->textures_.find(handle);
        assert(it != instance->textures_.end() && "TextureManagerに登録されていないハンドル");
        return it->second.Get();
    }

    ID3D12DescriptorHeap* TextureManager::GetDescriptorHeap() {
        return SrvManager::GetDescriptorHeap();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE TextureManager::GetCPUHandle(uint32_t index) {
        return SrvManager::GetCPUHandle(index);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetGPUHandle(uint32_t index) {
        return SrvManager::GetGPUHandle(index);
    }

    uint32_t TextureManager::RegisterResource(Microsoft::WRL::ComPtr<ID3D12Resource> resource, DXGI_FORMAT srvFormatOverride) {
        TextureManager* instance = GetInstance();

        // NOTE: srvFormatOverrideが指定されていればそちらを優先する。
        //       DXGI_FORMAT_R32_TYPELESS等のTYPELESSリソース(シャドウマップの深度テクスチャ等)は
        //       resource自体のFormatをそのままSRVに渡すと失敗するため、呼び出し側で
        //       具体的な型(例：DXGI_FORMAT_R32_FLOAT)を明示的に渡す必要がある。
        DXGI_FORMAT format = (srvFormatOverride != DXGI_FORMAT_UNKNOWN) ? srvFormatOverride : resource->GetDesc().Format;

        uint32_t index = SrvManager::Allocate();
        SrvManager::CreateSRVForTexture2D(index, resource.Get(), format, 1);

        instance->textures_[index] = resource;
        return index;
    }

    DirectX::ScratchImage TextureManager::LoadTexture(const std::string& filePath, bool isSRGB) {
        // テクスチャファイルを読んでプログラムで使えるようにする
        DirectX::ScratchImage image{};
        std::wstring filePathW = Logger::ConvertString(filePath);
        // 色のテクスチャはsRGBとして、法線マップなどのデータはファイル内のsRGB指定を無視してリニアのまま読む
        const DirectX::WIC_FLAGS wicFlags = isSRGB ? DirectX::WIC_FLAGS_FORCE_SRGB : DirectX::WIC_FLAGS_IGNORE_SRGB;
        HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), wicFlags, nullptr, image);
        assert(SUCCEEDED(hr));

        // ミップマップを1x1まで生成する(遠く・斜めから見たときのザラつきやモアレを防ぐ)
        // 圧縮フォーマットや1x1の画像は生成できない/不要なので、元画像のまま使う
        DirectX::ScratchImage mipImages{};
        const DirectX::TexMetadata& metadata = image.GetMetadata();
        bool generated = false;
        if (!DirectX::IsCompressed(metadata.format) && (metadata.width > 1 || metadata.height > 1)) {
            // sRGBのテクスチャはリニア空間で平均しないと縮小段が暗くなるため、TEX_FILTER_SRGBを付ける
            // (リニアのデータはそのまま平均すればよいので付けない)
            const DirectX::TEX_FILTER_FLAGS mipFilter = isSRGB ? DirectX::TEX_FILTER_SRGB : DirectX::TEX_FILTER_DEFAULT;
            hr = DirectX::GenerateMipMaps(*image.GetImage(0, 0, 0), mipFilter, 0, mipImages);
            if (SUCCEEDED(hr)) {
                generated = true;
            } else {
                Logger::LogError("[TextureManager] ミップマップの生成に失敗したため、元画像のまま使います: " + filePath + "\n");
            }
        }
        if (!generated) {
            mipImages = std::move(image);
        }

        // ミップマップ付きのデータを返す
        return mipImages;
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> TextureManager::CreateTextureResource(const DirectX::TexMetadata& metadata, bool isSRGB) {
        // metadataを基にResourceの設定
        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Width = UINT(metadata.width);  // Textureの幅
        resourceDesc.Height = UINT(metadata.height);  // Textureの高さ
        resourceDesc.MipLevels = UINT16(metadata.mipLevels);  // mipmapの数
        resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize);  // 奥行き or 配列Textureの数
        // TextureのFormat。isSRGB=trueなら、sRGB版があるフォーマットは必ずsRGBとして扱う
        // (WIC_FLAGS_FORCE_SRGBで読んでも、読み込み結果によってはsRGBにならない場合があるため念押し)
        // isSRGB=false(法線マップなど)なら、逆に必ずリニア版のフォーマットにする
        // SRVはresource->GetDesc().Formatから作るので、ここを変えればSRVも同じになる
        resourceDesc.Format = isSRGB ? DirectX::MakeSRGB(metadata.format) : DirectX::MakeLinear(metadata.format);
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
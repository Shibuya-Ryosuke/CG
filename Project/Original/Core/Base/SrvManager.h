#pragma once
#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <vector>

namespace RyoEngine {

    /// <summary>
    /// shader-visibleなCBV_SRV_UAVディスクリプタヒープを1つだけ持ち、
    /// エンジン全体(テクスチャ・レンダーターゲット・シャドウマップ・ImGui・GPUパーティクル等)で共有するクラス。
    ///
    /// ・Allocate()でスロット番号(index)を受け取り、Create〜()でそのスロットにビューを作る
    /// ・使い終わったスロットはFree()で返却すると、次のAllocate()で再利用される
    /// ・SetDescriptorHeapsはPreDraw()に集約する(CBV_SRV_UAVヒープは同時に1つしかバインドできないため)
    ///
    /// 初期化順: DirectXCommon::Initialize → SrvManager::Initialize → TextureManager / ImGuiManager 等
    /// 解放順  : TextureManager / ImGuiManager 等 → SrvManager::Finalize → DirectXCommon::Finalize
    /// </summary>
    class SrvManager {
    public:
        static void Initialize();
        static void Finalize();

        // --- スロット管理 ---
        // 空きスロットを1つ確保してindexを返す(返却済みのスロットがあれば優先して再利用する)
        static uint32_t Allocate();
        // スロットを返却する。返却したindexのGPUハンドルを使い続けないこと
        // (GPUがまだそのディスクリプタを参照している可能性がある場合は、フェンス待ちの後に呼ぶこと)
        static void Free(uint32_t index);

        // --- ビュー生成 ---
        // Texture2DのSRV。mipLevels=0を渡すとresourceのMipLevelsをそのまま使う
        static void CreateSRVForTexture2D(uint32_t index, ID3D12Resource* resource, DXGI_FORMAT format, UINT mipLevels = 0);
        // StructuredBufferのSRV
        static void CreateSRVForStructuredBuffer(uint32_t index, ID3D12Resource* resource, UINT numElements, UINT structureByteStride);
        // RWStructuredBufferのUAV(カウンター無し)
        static void CreateUAVForStructuredBuffer(uint32_t index, ID3D12Resource* resource, UINT numElements, UINT structureByteStride);
        // RWTexture2DのUAV(リソースはD3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS付きで作ること)
        static void CreateUAVForTexture2D(uint32_t index, ID3D12Resource* resource, DXGI_FORMAT format);

        // --- ハンドル取得 ---
        static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(uint32_t index);
        static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(uint32_t index);
        static ID3D12DescriptorHeap* GetDescriptorHeap() { return GetInstance()->descriptorHeap_.Get(); }

        // ヒープをコマンドリストにセットする(描画/ディスパッチでDescriptorTableを使う前に呼ぶ)
        static void PreDraw(ID3D12GraphicsCommandList* commandList);

        // 確保中のスロット数(デバッグ表示用)
        static uint32_t GetUsedCount() { return GetInstance()->nextIndex_ - static_cast<uint32_t>(GetInstance()->freeList_.size()); }

        static constexpr uint32_t kMaxSRVCount = 512;

    private:
        static SrvManager* GetInstance();

        SrvManager() = default;
        ~SrvManager() = default;
        SrvManager(const SrvManager&) = delete;
        SrvManager& operator=(const SrvManager&) = delete;

        ID3D12Device* device_ = nullptr;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap_;
        uint32_t descriptorSize_ = 0;

        uint32_t nextIndex_ = 0;          // まだ一度も使っていない先頭のスロット
        std::vector<uint32_t> freeList_;  // 返却されたスロット
#ifdef _DEBUG
        std::vector<bool> inUse_;         // 二重解放・未確保スロットへのアクセス検出用
#endif
    };
}
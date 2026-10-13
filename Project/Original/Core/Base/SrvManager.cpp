#include "SrvManager.h"
#include "DirectXCommon.h"
#include "Logger.h"
#include <cassert>
#include <format>

namespace RyoEngine {

    SrvManager* SrvManager::GetInstance() {
        static SrvManager instance;
        return &instance;
    }

    void SrvManager::Initialize() {
        Logger::Log("SrvManager : Initializing...\n");
        device_ = DirectXCommon::GetInstance()->GetDevice();
        assert(device_ != nullptr && "DirectXCommon::Initialize()より後に呼ぶこと");

        descriptorHeap_ = DirectXCommon::CreateDescriptorHeap(
            device_, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kMaxSRVCount, true);
        descriptorSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        nextIndex_ = 0;
        freeList_.clear();
#ifdef _DEBUG
        inUse_.assign(kMaxSRVCount, false);
#endif
        Logger::LogSuccess("SrvManager : Initialized\n");
    }

    void SrvManager::Finalize() {
        Logger::Log("SrvManager : Finalizing...\n");
        if (GetUsedCount() != 0) {
            Logger::Log(std::format("SrvManager : {} slot(s) were not freed (OK at shutdown)\n", GetUsedCount()));
        }
        freeList_.clear();
        nextIndex_ = 0;
#ifdef _DEBUG
        inUse_.clear();
#endif
        descriptorHeap_.Reset();
        device_ = nullptr;
        Logger::LogSuccess("SrvManager : Finalized\n");
    }

    uint32_t SrvManager::Allocate() {
        uint32_t index = 0;
        if (!freeList_.empty()) {
            index = freeList_.back();
            freeList_.pop_back();
        } else {
            assert(nextIndex_ < kMaxSRVCount && "SRVヒープが満杯。kMaxSRVCountを増やすこと");
            index = nextIndex_++;
        }
#ifdef _DEBUG
        inUse_[index] = true;
#endif
        return index;
    }

    void SrvManager::Free(uint32_t index) {
        assert(index < nextIndex_);
#ifdef _DEBUG
        assert(inUse_[index] && "確保されていない(または解放済みの)スロットをFreeしようとした");
        inUse_[index] = false;
#endif
        freeList_.push_back(index);
    }

    void SrvManager::CreateSRVForTexture2D(uint32_t index, ID3D12Resource* resource, DXGI_FORMAT format, UINT mipLevels) {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = format;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = (mipLevels != 0) ? mipLevels : resource->GetDesc().MipLevels;
        device_->CreateShaderResourceView(resource, &srvDesc, GetCPUHandle(index));
    }

    void SrvManager::CreateSRVForStructuredBuffer(uint32_t index, ID3D12Resource* resource, UINT numElements, UINT structureByteStride) {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.NumElements = numElements;
        srvDesc.Buffer.StructureByteStride = structureByteStride;
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
        device_->CreateShaderResourceView(resource, &srvDesc, GetCPUHandle(index));
    }

    void SrvManager::CreateUAVForStructuredBuffer(uint32_t index, ID3D12Resource* resource, UINT numElements, UINT structureByteStride) {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
        uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.FirstElement = 0;
        uavDesc.Buffer.NumElements = numElements;
        uavDesc.Buffer.StructureByteStride = structureByteStride;
        uavDesc.Buffer.CounterOffsetInBytes = 0;
        uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
        device_->CreateUnorderedAccessView(resource, nullptr, &uavDesc, GetCPUHandle(index));
    }

    void SrvManager::CreateUAVForTexture2D(uint32_t index, ID3D12Resource* resource, DXGI_FORMAT format) {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
        uavDesc.Format = format;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Texture2D.MipSlice = 0;
        uavDesc.Texture2D.PlaneSlice = 0;
        device_->CreateUnorderedAccessView(resource, nullptr, &uavDesc, GetCPUHandle(index));
    }

    D3D12_CPU_DESCRIPTOR_HANDLE SrvManager::GetCPUHandle(uint32_t index) const {
        assert(index < kMaxSRVCount);
        D3D12_CPU_DESCRIPTOR_HANDLE handle = descriptorHeap_->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += static_cast<SIZE_T>(descriptorSize_) * index;
        return handle;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE SrvManager::GetGPUHandle(uint32_t index) const {
        assert(index < kMaxSRVCount);
        D3D12_GPU_DESCRIPTOR_HANDLE handle = descriptorHeap_->GetGPUDescriptorHandleForHeapStart();
        handle.ptr += static_cast<UINT64>(descriptorSize_) * index;
        return handle;
    }

    void SrvManager::PreDraw(ID3D12GraphicsCommandList* commandList) {
        ID3D12DescriptorHeap* heaps[] = { descriptorHeap_.Get() };
        commandList->SetDescriptorHeaps(_countof(heaps), heaps);
    }
}
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
        SrvManager* instance = GetInstance();

        Logger::Log("SrvManager : Initializing...\n");
        instance->device_ = DirectXCommon::GetInstance()->GetDevice();
        assert(instance->device_ != nullptr && "DirectXCommon::Initialize()より後に呼ぶこと");

        instance->descriptorHeap_ = DirectXCommon::CreateDescriptorHeap(
            instance->device_, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kMaxSRVCount, true);
        instance->descriptorSize_ = instance->device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        instance->nextIndex_ = 0;
        instance->freeList_.clear();
#ifdef _DEBUG
        instance->inUse_.assign(kMaxSRVCount, false);
#endif
        Logger::LogSuccess("SrvManager : Initialized\n");
    }

    void SrvManager::Finalize() {
        SrvManager* instance = GetInstance();

        Logger::Log("SrvManager : Finalizing...\n");
        if (GetUsedCount() != 0) {
            Logger::Log(std::format("SrvManager : {} slot(s) were not freed (OK at shutdown)\n", GetUsedCount()));
        }
        instance->freeList_.clear();
        instance->nextIndex_ = 0;
#ifdef _DEBUG
        instance->inUse_.clear();
#endif
        instance->descriptorHeap_.Reset();
        instance->device_ = nullptr;
        Logger::LogSuccess("SrvManager : Finalized\n");
    }

    uint32_t SrvManager::Allocate() {
        SrvManager* instance = GetInstance();

        uint32_t index = 0;
        if (!instance->freeList_.empty()) {
            index = instance->freeList_.back();
            instance->freeList_.pop_back();
        } else {
            assert(instance->nextIndex_ < kMaxSRVCount && "SRVヒープが満杯。kMaxSRVCountを増やすこと");
            index = instance->nextIndex_++;
        }
#ifdef _DEBUG
        instance->inUse_[index] = true;
#endif
        return index;
    }

    void SrvManager::Free(uint32_t index) {
        SrvManager* instance = GetInstance();

        assert(index < instance->nextIndex_);
#ifdef _DEBUG
        assert(instance->inUse_[index] && "確保されていない(または解放済みの)スロットをFreeしようとした");
        instance->inUse_[index] = false;
#endif
        instance->freeList_.push_back(index);
    }

    void SrvManager::CreateSRVForTexture2D(uint32_t index, ID3D12Resource* resource, DXGI_FORMAT format, UINT mipLevels) {
        SrvManager* instance = GetInstance();

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = format;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = (mipLevels != 0) ? mipLevels : resource->GetDesc().MipLevels;
        instance->device_->CreateShaderResourceView(resource, &srvDesc, GetCPUHandle(index));
    }

    void SrvManager::CreateSRVForStructuredBuffer(uint32_t index, ID3D12Resource* resource, UINT numElements, UINT structureByteStride) {
        SrvManager* instance = GetInstance();

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.NumElements = numElements;
        srvDesc.Buffer.StructureByteStride = structureByteStride;
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
        instance->device_->CreateShaderResourceView(resource, &srvDesc, GetCPUHandle(index));
    }

    void SrvManager::CreateUAVForStructuredBuffer(uint32_t index, ID3D12Resource* resource, UINT numElements, UINT structureByteStride) {
        SrvManager* instance = GetInstance();

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
        uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.FirstElement = 0;
        uavDesc.Buffer.NumElements = numElements;
        uavDesc.Buffer.StructureByteStride = structureByteStride;
        uavDesc.Buffer.CounterOffsetInBytes = 0;
        uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
        instance->device_->CreateUnorderedAccessView(resource, nullptr, &uavDesc, GetCPUHandle(index));
    }

    void SrvManager::CreateUAVForTexture2D(uint32_t index, ID3D12Resource* resource, DXGI_FORMAT format) {
        SrvManager* instance = GetInstance();

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
        uavDesc.Format = format;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Texture2D.MipSlice = 0;
        uavDesc.Texture2D.PlaneSlice = 0;
        instance->device_->CreateUnorderedAccessView(resource, nullptr, &uavDesc, GetCPUHandle(index));
    }

    D3D12_CPU_DESCRIPTOR_HANDLE SrvManager::GetCPUHandle(uint32_t index) {
        SrvManager* instance = GetInstance();

        assert(index < kMaxSRVCount);
        D3D12_CPU_DESCRIPTOR_HANDLE handle = instance->descriptorHeap_->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += static_cast<SIZE_T>(instance->descriptorSize_) * index;
        return handle;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE SrvManager::GetGPUHandle(uint32_t index) {
        SrvManager* instance = GetInstance();

        assert(index < kMaxSRVCount);
        D3D12_GPU_DESCRIPTOR_HANDLE handle = instance->descriptorHeap_->GetGPUDescriptorHandleForHeapStart();
        handle.ptr += static_cast<UINT64>(instance->descriptorSize_) * index;
        return handle;
    }

    void SrvManager::PreDraw(ID3D12GraphicsCommandList* commandList) {
        SrvManager* instance = GetInstance();

        ID3D12DescriptorHeap* heaps[] = { instance->descriptorHeap_.Get() };
        commandList->SetDescriptorHeaps(_countof(heaps), heaps);
    }
}
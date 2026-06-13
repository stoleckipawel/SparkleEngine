#include "D3D12/Resources/D3D12ResourceService.h"

#include "D3D12/D3D12TypeConversions.h"
#include "D3D12/Descriptors/D3D12DescriptorHeapManager.h"
#include "D3D12/Descriptors/D3D12DescriptorService.h"
#include "D3D12/Device/D3D12Rhi.h"
#include "D3D12/Memory/D3D12GpuAllocation.h"
#include "D3D12/Memory/D3D12GpuMemoryAllocator.h"
#include "D3D12/Textures/TextureFactory.h"
#include "Resources/Texture.h"
#include "RHI/Public/Validation/RhiValidation.h"

#include <algorithm>
#include <cstring>
#include <d3d12.h>

D3D12ResourceService::D3D12ResourceService(
    D3D12Rhi& rhi,
    D3D12GpuMemoryAllocator& memoryAllocator,
    D3D12DescriptorHeapManager& descriptorHeapManager,
    D3D12DescriptorService& descriptorService,
    const RhiCapabilities& capabilities) noexcept :
	m_rhi(&rhi), m_memoryAllocator(&memoryAllocator), m_descriptorHeapManager(&descriptorHeapManager),
	m_descriptorService(&descriptorService), m_capabilities(&capabilities)
{
}

std::unique_ptr<Texture> D3D12ResourceService::CreateTexture(
    RhiTextureUploadDesc textureUpload,
    std::wstring_view debugName)
{
	(void) debugName;
	if (m_rhi == nullptr || m_descriptorHeapManager == nullptr || !textureUpload.IsValid())
	{
		return {};
	}

	std::unique_ptr<TextureFactory> textureFactory = TextureFactory::Create(*m_rhi, *m_descriptorHeapManager);
	return textureFactory != nullptr ? textureFactory->CreateTexture(std::move(textureUpload)) : std::unique_ptr<Texture>{};
}

RhiOwnedResourceHandle D3D12ResourceService::CreateTextureResource(
    const RhiTextureResourceDesc& desc,
    ResourceState initialState,
    RhiMemoryCategory category,
    RhiMemoryResidencyClass residencyClass,
    std::wstring_view debugName)
{
	if (m_memoryAllocator == nullptr || m_capabilities == nullptr ||
	    !RhiValidation::ValidateTextureResourceDesc(*m_capabilities, desc, "RHI.D3D12.CreateTextureResource"))
	{
		return {};
	}

	const D3D12_RESOURCE_DESC resourceDesc = D3D12TypeConversions::BuildTextureResourceDesc(desc);
	std::unique_ptr<D3D12GpuAllocationRecord> ownedRecord = m_memoryAllocator->CreateTexture(
	    resourceDesc,
	    D3D12TypeConversions::ToResourceStates(initialState),
	    nullptr,
	    category,
	    residencyClass,
	    CopyDebugName(debugName, L"TextureResource"));
	return ownedRecord != nullptr ? WrapOwnedResource(std::move(ownedRecord)) : RhiOwnedResourceHandle{};
}

RhiOwnedResourceHandle D3D12ResourceService::CreateBufferResource(
    const RhiBufferResourceDesc& desc,
    ResourceState initialState,
    RhiMemoryCategory category,
    RhiMemoryResidencyClass residencyClass,
    std::wstring_view debugName)
{
	if (m_memoryAllocator == nullptr || desc.SizeInBytes == 0)
	{
		return {};
	}

	const D3D12_RESOURCE_DESC resourceDesc = D3D12TypeConversions::BuildBufferResourceDesc(desc);
	std::unique_ptr<D3D12GpuAllocationRecord> ownedRecord = m_memoryAllocator->CreateBuffer(
	    resourceDesc,
	    D3D12TypeConversions::ToResourceStates(initialState),
	    category,
	    residencyClass,
	    CopyDebugName(debugName, L"BufferResource"));
	return ownedRecord != nullptr ? WrapOwnedResource(std::move(ownedRecord)) : RhiOwnedResourceHandle{};
}

bool D3D12ResourceService::CreateVertexBuffer(
    const void* data,
    std::size_t sizeInBytes,
    std::uint32_t strideInBytes,
    std::wstring_view debugName,
    RhiOwnedResourceHandle& outResource,
    RhiVertexBufferView& outView)
{
	outResource = {};
	outView = {};
	if (m_rhi == nullptr || m_memoryAllocator == nullptr || data == nullptr || sizeInBytes == 0 || strideInBytes == 0)
	{
		return false;
	}

	D3D12_RESOURCE_DESC resourceDesc = D3D12TypeConversions::BuildBufferResourceDesc(RhiBufferResourceDesc{.SizeInBytes = sizeInBytes});
	std::wstring ownedDebugName = CopyDebugName(debugName, L"VertexBuffer");
	std::unique_ptr<D3D12GpuAllocationRecord> ownedRecord = m_memoryAllocator->CreateBuffer(
	    resourceDesc,
	    D3D12_RESOURCE_STATE_GENERIC_READ,
	    RhiMemoryCategory::Mesh,
	    RhiMemoryResidencyClass::HostUpload,
	    ownedDebugName);
	if (ownedRecord == nullptr || ownedRecord->Resource == nullptr)
	{
		return false;
	}

	ID3D12Resource* const ownedResource = ownedRecord->Resource.Get();
	void* mappedData = nullptr;
	const D3D12_RANGE readRange{0, 0};
	if (FAILED(ownedResource->Map(0, &readRange, &mappedData)))
	{
		return false;
	}

	ownedRecord->IsMapped = true;
	ownedRecord->CpuMappedAddress = mappedData;
	std::memcpy(mappedData, data, sizeInBytes);
	ownedResource->Unmap(0, nullptr);
	ownedRecord->IsMapped = false;
	ownedRecord->CpuMappedAddress = nullptr;

	outView = RhiVertexBufferView{
	    .BufferLocation = ownedResource->GetGPUVirtualAddress(),
	    .SizeInBytes = static_cast<std::uint32_t>(sizeInBytes),
	    .StrideInBytes = strideInBytes};
	outResource = WrapOwnedResource(std::move(ownedRecord));
	return true;
}

bool D3D12ResourceService::CreateStructuredBuffer(
    const void* data,
    std::size_t sizeInBytes,
    std::uint32_t strideInBytes,
    std::wstring_view debugName,
    RhiOwnedResourceHandle& outResource,
    RhiResourceViewHandle& outView)
{
	outResource = {};
	outView = {};
	if (m_rhi == nullptr || m_memoryAllocator == nullptr || m_descriptorService == nullptr || data == nullptr || sizeInBytes == 0 ||
	    strideInBytes == 0)
	{
		return false;
	}

	const RhiBufferResourceDesc bufferDesc{.SizeInBytes = sizeInBytes, .StrideInBytes = strideInBytes};
	const D3D12_RESOURCE_DESC resourceDesc = D3D12TypeConversions::BuildBufferResourceDesc(bufferDesc);
	std::wstring ownedDebugName = CopyDebugName(debugName, L"StructuredBuffer");
	std::unique_ptr<D3D12GpuAllocationRecord> ownedRecord = m_memoryAllocator->CreateBuffer(
	    resourceDesc,
	    D3D12_RESOURCE_STATE_GENERIC_READ,
	    RhiMemoryCategory::Mesh,
	    RhiMemoryResidencyClass::HostUpload,
	    ownedDebugName.empty() ? L"StructuredBuffer" : ownedDebugName);
	if (ownedRecord == nullptr || ownedRecord->Resource == nullptr)
	{
		return false;
	}

	ID3D12Resource* const ownedResource = ownedRecord->Resource.Get();
	void* mappedData = nullptr;
	const D3D12_RANGE readRange{0, 0};
	if (FAILED(ownedResource->Map(0, &readRange, &mappedData)))
	{
		return false;
	}

	ownedRecord->IsMapped = true;
	ownedRecord->CpuMappedAddress = mappedData;
	std::memcpy(mappedData, data, sizeInBytes);
	ownedResource->Unmap(0, nullptr);
	ownedRecord->IsMapped = false;
	ownedRecord->CpuMappedAddress = nullptr;

	outResource = WrapOwnedResource(std::move(ownedRecord));
	outView = m_descriptorService->CreateResourceView(
	    RhiResourceViewDesc::BufferShaderResource(GetNativeResource(outResource), sizeInBytes, strideInBytes));
	if (!outView)
	{
		ReleaseOwnedResource(outResource);
		outResource = {};
		return false;
	}

	return true;
}

bool D3D12ResourceService::CreateIndexBuffer(
    const void* data,
    std::size_t sizeInBytes,
    RhiIndexFormat format,
    std::wstring_view debugName,
    RhiOwnedResourceHandle& outResource,
    RhiIndexBufferView& outView)
{
	outResource = {};
	outView = {};
	if (m_rhi == nullptr || m_memoryAllocator == nullptr || data == nullptr || sizeInBytes == 0)
	{
		return false;
	}

	D3D12_RESOURCE_DESC resourceDesc = D3D12TypeConversions::BuildBufferResourceDesc(RhiBufferResourceDesc{.SizeInBytes = sizeInBytes});
	std::wstring ownedDebugName = CopyDebugName(debugName, L"IndexBuffer");
	std::unique_ptr<D3D12GpuAllocationRecord> ownedRecord = m_memoryAllocator->CreateBuffer(
	    resourceDesc,
	    D3D12_RESOURCE_STATE_GENERIC_READ,
	    RhiMemoryCategory::Mesh,
	    RhiMemoryResidencyClass::HostUpload,
	    ownedDebugName);
	if (ownedRecord == nullptr || ownedRecord->Resource == nullptr)
	{
		return false;
	}

	ID3D12Resource* const ownedResource = ownedRecord->Resource.Get();
	void* mappedData = nullptr;
	const D3D12_RANGE readRange{0, 0};
	if (FAILED(ownedResource->Map(0, &readRange, &mappedData)))
	{
		return false;
	}

	ownedRecord->IsMapped = true;
	ownedRecord->CpuMappedAddress = mappedData;
	std::memcpy(mappedData, data, sizeInBytes);
	ownedResource->Unmap(0, nullptr);
	ownedRecord->IsMapped = false;
	ownedRecord->CpuMappedAddress = nullptr;

	outView = RhiIndexBufferView{
	    .BufferLocation = ownedResource->GetGPUVirtualAddress(),
	    .SizeInBytes = static_cast<std::uint32_t>(sizeInBytes),
	    .Format = format};
	outResource = WrapOwnedResource(std::move(ownedRecord));
	return true;
}

void D3D12ResourceService::ReleaseOwnedResource(RhiOwnedResourceHandle resource) noexcept
{
	if (resource.Value == nullptr)
	{
		return;
	}

	std::unique_ptr<D3D12GpuAllocationRecord> ownedRecord = TakeD3D12OwnedResourceHandle(resource);
	if (ownedRecord == nullptr)
	{
		return;
	}

	std::uint64_t retireFenceValue = 0;
	if (m_rhi != nullptr)
	{
		retireFenceValue = m_rhi->GetNextFenceValue();
	}

	DrainCompletedReleases();
	m_pendingOwnedResourceReleases.push_back(
	    PendingOwnedResourceRelease{.Record = std::move(ownedRecord), .RetireFenceValue = retireFenceValue});
}

void D3D12ResourceService::DrainCompletedReleases() noexcept
{
	if (m_pendingOwnedResourceReleases.empty() && m_pendingOwnedMemoryBlockReleases.empty())
	{
		return;
	}

	std::uint64_t completedFenceValue = UINT64_MAX;
	if (m_rhi != nullptr && m_rhi->GetFence())
	{
		completedFenceValue = m_rhi->GetFence()->GetCompletedValue();
	}

	auto eraseBegin = std::remove_if(
	    m_pendingOwnedResourceReleases.begin(),
	    m_pendingOwnedResourceReleases.end(),
	    [completedFenceValue](const PendingOwnedResourceRelease& pendingRelease)
	    {
		    return pendingRelease.Record == nullptr || pendingRelease.RetireFenceValue <= completedFenceValue;
	    });
	m_pendingOwnedResourceReleases.erase(eraseBegin, m_pendingOwnedResourceReleases.end());

	auto heapEraseBegin = std::remove_if(
	    m_pendingOwnedMemoryBlockReleases.begin(),
	    m_pendingOwnedMemoryBlockReleases.end(),
	    [completedFenceValue](const PendingOwnedMemoryBlockRelease& pendingRelease)
	    {
		    return pendingRelease.Record == nullptr || pendingRelease.RetireFenceValue <= completedFenceValue;
	    });
	m_pendingOwnedMemoryBlockReleases.erase(heapEraseBegin, m_pendingOwnedMemoryBlockReleases.end());
}

NativeResourceHandle D3D12ResourceService::GetNativeResource(RhiOwnedResourceHandle resource) const noexcept
{
	return NativeResourceHandle{GetD3D12Resource(resource)};
}

RhiGpuVirtualAddress D3D12ResourceService::GetResourceGpuVirtualAddress(RhiOwnedResourceHandle resource) const noexcept
{
	ID3D12Resource* const nativeResource = GetD3D12Resource(resource);
	return nativeResource != nullptr ? nativeResource->GetGPUVirtualAddress() : 0;
}

RhiResourceAllocationInfo D3D12ResourceService::GetTextureAllocationInfo(const RhiTextureResourceDesc& desc) const noexcept
{
	if (m_rhi == nullptr)
	{
		return {};
	}

	const D3D12_RESOURCE_DESC resourceDesc = D3D12TypeConversions::BuildTextureResourceDesc(desc);
	const D3D12_RESOURCE_ALLOCATION_INFO allocationInfo = m_rhi->GetDevice()->GetResourceAllocationInfo(0, 1, &resourceDesc);
	return RhiResourceAllocationInfo{.SizeInBytes = allocationInfo.SizeInBytes, .Alignment = allocationInfo.Alignment};
}

RhiResourceAllocationInfo D3D12ResourceService::GetBufferAllocationInfo(const RhiBufferResourceDesc& desc) const noexcept
{
	if (m_rhi == nullptr)
	{
		return {};
	}

	const D3D12_RESOURCE_DESC resourceDesc = D3D12TypeConversions::BuildBufferResourceDesc(desc);
	const D3D12_RESOURCE_ALLOCATION_INFO allocationInfo = m_rhi->GetDevice()->GetResourceAllocationInfo(0, 1, &resourceDesc);
	return RhiResourceAllocationInfo{.SizeInBytes = allocationInfo.SizeInBytes, .Alignment = allocationInfo.Alignment};
}

RhiOwnedMemoryBlockHandle D3D12ResourceService::CreateTransientMemoryBlock(
    RhiTransientAllocationPool pool,
    std::uint64_t sizeInBytes,
    std::uint64_t alignment,
    std::wstring_view debugName)
{
	if (m_rhi == nullptr || m_memoryAllocator == nullptr || sizeInBytes == 0)
	{
		return {};
	}

	std::wstring ownedDebugName = CopyDebugName(debugName, L"TransientMemoryBlock");
	std::unique_ptr<D3D12GpuHeapRecord> ownedMemoryBlock =
	    m_memoryAllocator->CreateTransientHeap(pool, sizeInBytes, alignment, ownedDebugName);
	return ownedMemoryBlock != nullptr ? WrapOwnedMemoryBlock(std::move(ownedMemoryBlock)) : RhiOwnedMemoryBlockHandle{};
}

void D3D12ResourceService::ReleaseTransientMemoryBlock(RhiOwnedMemoryBlockHandle memoryBlock) noexcept
{
	std::unique_ptr<D3D12GpuHeapRecord> ownedMemoryBlock = TakeD3D12OwnedMemoryBlockHandle(memoryBlock);
	if (ownedMemoryBlock == nullptr)
	{
		return;
	}

	std::uint64_t retireFenceValue = 0;
	if (m_rhi != nullptr)
	{
		retireFenceValue = m_rhi->GetNextFenceValue();
	}

	DrainCompletedReleases();
	m_pendingOwnedMemoryBlockReleases.push_back(
	    PendingOwnedMemoryBlockRelease{.Record = std::move(ownedMemoryBlock), .RetireFenceValue = retireFenceValue});
}

RhiOwnedResourceHandle D3D12ResourceService::CreateAliasingTextureResource(
    RhiOwnedMemoryBlockHandle memoryBlock,
    std::uint64_t memoryBlockOffset,
    const RhiTransientTextureAllocationDesc& desc,
    std::wstring_view debugName)
{
	D3D12GpuHeapRecord* const ownedMemoryBlock = GetD3D12GpuHeapRecord(memoryBlock);
	if (m_rhi == nullptr || m_memoryAllocator == nullptr || m_capabilities == nullptr || ownedMemoryBlock == nullptr ||
	    !RhiValidation::ValidateTextureResourceDesc(*m_capabilities, desc.ResourceDesc, "RHI.D3D12.CreateAliasingTextureResource"))
	{
		return {};
	}

	const D3D12_RESOURCE_DESC resourceDesc = D3D12TypeConversions::BuildTextureResourceDesc(desc.ResourceDesc);
	const D3D12_CLEAR_VALUE clearValue = D3D12TypeConversions::BuildClearValue(desc.ClearValue);
	const D3D12_CLEAR_VALUE* clearValuePtr = desc.ClearValue.ValueType == RhiOptimizedClearValue::Type::None ? nullptr : &clearValue;
	std::wstring ownedDebugName = CopyDebugName(debugName, L"AliasingTexture");
	std::unique_ptr<D3D12GpuAllocationRecord> ownedResource = m_memoryAllocator->CreateAliasingTexture(
	    *ownedMemoryBlock,
	    memoryBlockOffset,
	    resourceDesc,
	    D3D12TypeConversions::ToResourceStates(desc.InitialState),
	    clearValuePtr,
	    ownedDebugName);
	return ownedResource != nullptr ? WrapOwnedResource(std::move(ownedResource)) : RhiOwnedResourceHandle{};
}

RhiOwnedResourceHandle D3D12ResourceService::CreateAliasingBufferResource(
    RhiOwnedMemoryBlockHandle memoryBlock,
    std::uint64_t memoryBlockOffset,
    const RhiTransientBufferAllocationDesc& desc,
    std::wstring_view debugName)
{
	D3D12GpuHeapRecord* const ownedMemoryBlock = GetD3D12GpuHeapRecord(memoryBlock);
	if (m_rhi == nullptr || m_memoryAllocator == nullptr || ownedMemoryBlock == nullptr)
	{
		return {};
	}

	const D3D12_RESOURCE_DESC resourceDesc = D3D12TypeConversions::BuildBufferResourceDesc(desc.ResourceDesc);
	std::wstring ownedDebugName = CopyDebugName(debugName, L"AliasingBuffer");
	std::unique_ptr<D3D12GpuAllocationRecord> ownedResource = m_memoryAllocator->CreateAliasingBuffer(
	    *ownedMemoryBlock,
	    memoryBlockOffset,
	    resourceDesc,
	    D3D12TypeConversions::ToResourceStates(desc.InitialState),
	    ownedDebugName);
	return ownedResource != nullptr ? WrapOwnedResource(std::move(ownedResource)) : RhiOwnedResourceHandle{};
}

bool D3D12ResourceService::SupportsUnorderedAccess(NativeResourceHandle resource) const noexcept
{
	return ResourceSupportsUnorderedAccess(static_cast<ID3D12Resource*>(resource.Value));
}

std::wstring D3D12ResourceService::CopyDebugName(std::wstring_view debugName, std::wstring_view fallbackName)
{
	return debugName.empty() ? std::wstring(fallbackName) : std::wstring(debugName);
}

RhiOwnedResourceHandle D3D12ResourceService::WrapOwnedResource(std::unique_ptr<D3D12GpuAllocationRecord> record) noexcept
{
	return MakeD3D12OwnedResourceHandle(std::move(record));
}

RhiOwnedMemoryBlockHandle D3D12ResourceService::WrapOwnedMemoryBlock(std::unique_ptr<D3D12GpuHeapRecord> record) noexcept
{
	if (record == nullptr)
	{
		return {};
	}

	return MakeD3D12OwnedMemoryBlockHandle(std::move(record));
}

bool D3D12ResourceService::ResourceSupportsUnorderedAccess(ID3D12Resource* resource) noexcept
{
	return resource != nullptr && (resource->GetDesc().Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0;
}

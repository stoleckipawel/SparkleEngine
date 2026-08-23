#include "D3D12/Resources/D3D12ResourceService.h"

#include "D3D12/D3D12TypeConversions.h"
#include "D3D12/Device/D3D12Rhi.h"
#include "D3D12/Memory/D3D12GpuAllocation.h"
#include "D3D12/Memory/D3D12GpuMemoryAllocator.h"
#include "RHI/Public/Diagnostics/RhiDiagnostics.h"
#include "Validation/RhiContract.h"

#include <algorithm>
#include <cstring>
#include <d3d12.h>

D3D12ResourceService::D3D12ResourceService(
    D3D12Rhi& rhi,
    D3D12GpuMemoryAllocator& memoryAllocator,
    const RhiCapabilities& capabilities) noexcept :
    m_rhi(&rhi),
    m_memoryAllocator(&memoryAllocator),
    m_capabilities(&capabilities)
{
}

RhiOwnedResourceHandle D3D12ResourceService::CreateTextureResource(
    const RhiTextureResourceDesc& desc,
    ResourceState initialState,
    RhiMemoryCategory category,
    RhiMemoryResidencyClass residencyClass,
    std::wstring_view debugName)
{
	if (m_memoryAllocator == nullptr || m_capabilities == nullptr || !RhiContract::IsTextureResourceDescUsable(*m_capabilities, desc))
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
	    MakeDebugName(debugName, L"TextureResource"));
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
	    MakeDebugName(debugName, L"BufferResource"));
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
	std::wstring ownedDebugName = MakeDebugName(debugName, L"VertexBuffer");
	std::unique_ptr<D3D12GpuAllocationRecord> ownedRecord = m_memoryAllocator->CreateBuffer(
	    resourceDesc,
	    D3D12_RESOURCE_STATE_GENERIC_READ,
	    RhiMemoryCategory::Mesh,
	    RhiMemoryResidencyClass::HostUpload,
	    ownedDebugName);
	if (ownedRecord == nullptr || ownedRecord->Resource == nullptr)
	{
		CollectCrashDiagnosticsOnce();
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

bool D3D12ResourceService::CreateStructuredBufferResource(
    const void* data,
    std::size_t sizeInBytes,
    std::uint32_t strideInBytes,
    std::wstring_view debugName,
    RhiOwnedResourceHandle& outResource)
{
	outResource = {};
	if (m_rhi == nullptr || m_memoryAllocator == nullptr || data == nullptr || sizeInBytes == 0 || strideInBytes == 0)
	{
		return false;
	}

	const RhiBufferResourceDesc bufferDesc{.SizeInBytes = sizeInBytes, .StrideInBytes = strideInBytes};
	const D3D12_RESOURCE_DESC resourceDesc = D3D12TypeConversions::BuildBufferResourceDesc(bufferDesc);
	std::wstring ownedDebugName = MakeDebugName(debugName, L"StructuredBuffer");
	std::unique_ptr<D3D12GpuAllocationRecord> ownedRecord = m_memoryAllocator->CreateBuffer(
	    resourceDesc,
	    D3D12_RESOURCE_STATE_GENERIC_READ,
	    RhiMemoryCategory::Mesh,
	    RhiMemoryResidencyClass::HostUpload,
	    ownedDebugName);
	if (ownedRecord == nullptr || ownedRecord->Resource == nullptr)
	{
		CollectCrashDiagnosticsOnce();
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
	return static_cast<bool>(outResource);
}

bool D3D12ResourceService::WriteBufferResource(
    RhiOwnedResourceHandle resource,
    std::size_t destinationOffsetInBytes,
    const void* data,
    std::size_t sizeInBytes) noexcept
{
	D3D12GpuAllocationRecord* const record =
	    GetD3D12GpuAllocationRecord(resource);
	if (record == nullptr || record->Resource == nullptr || data == nullptr ||
	    sizeInBytes == 0 ||
	    destinationOffsetInBytes > record->Resource->GetDesc().Width ||
	    sizeInBytes > record->Resource->GetDesc().Width - destinationOffsetInBytes)
	{
		return false;
	}

	void* mappedData = nullptr;
	const D3D12_RANGE readRange{0, 0};
	if (FAILED(record->Resource->Map(0, &readRange, &mappedData)))
	{
		return false;
	}

	record->IsMapped = true;
	record->CpuMappedAddress = mappedData;
	std::memcpy(
	    static_cast<std::byte*>(mappedData) + destinationOffsetInBytes,
	    data,
	    sizeInBytes);
	const D3D12_RANGE writtenRange{
	    destinationOffsetInBytes,
	    destinationOffsetInBytes + sizeInBytes};
	record->Resource->Unmap(0, &writtenRange);
	record->IsMapped = false;
	record->CpuMappedAddress = nullptr;
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
	std::wstring ownedDebugName = MakeDebugName(debugName, L"IndexBuffer");
	std::unique_ptr<D3D12GpuAllocationRecord> ownedRecord = m_memoryAllocator->CreateBuffer(
	    resourceDesc,
	    D3D12_RESOURCE_STATE_GENERIC_READ,
	    RhiMemoryCategory::Mesh,
	    RhiMemoryResidencyClass::HostUpload,
	    ownedDebugName);
	if (ownedRecord == nullptr || ownedRecord->Resource == nullptr)
	{
		CollectCrashDiagnosticsOnce();
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

	DrainCompletedResourceReleases();
	ownedRecord->PendingRelease = true;
	m_pendingOwnedResourceReleases.push_back(std::move(ownedRecord));
}

void D3D12ResourceService::DrainCompletedResourceReleases() noexcept
{
	if (m_pendingOwnedResourceReleases.empty() && m_pendingOwnedMemoryBlockReleases.empty())
	{
		return;
	}

	std::array<std::uint64_t, RhiQueueTypeCount> completedValues{};
	if (m_rhi != nullptr)
	{
		for (std::size_t queueIndex = 0; queueIndex < RhiQueueTypeCount; ++queueIndex)
		{
			completedValues[queueIndex] =
			    m_rhi->GetCompletedSubmissionValue(static_cast<ERhiQueueType>(queueIndex));
		}
	}
	else
	{
		completedValues.fill(UINT64_MAX);
	}

	auto eraseBegin = std::remove_if(
	    m_pendingOwnedResourceReleases.begin(),
	    m_pendingOwnedResourceReleases.end(),
	    [&completedValues](const std::unique_ptr<D3D12GpuAllocationRecord>& record)
	    {
		    return record == nullptr ||
		           (record->RecordingReferenceCount.load(
		                std::memory_order_relaxed) == 0 &&
		            record->LastUse.IsComplete(completedValues));
	    });
	m_pendingOwnedResourceReleases.erase(eraseBegin, m_pendingOwnedResourceReleases.end());

	auto heapEraseBegin = std::remove_if(
	    m_pendingOwnedMemoryBlockReleases.begin(),
	    m_pendingOwnedMemoryBlockReleases.end(),
	    [&completedValues](const std::unique_ptr<D3D12GpuHeapRecord>& record)
	    {
		    return record == nullptr ||
		           (record->RecordingReferenceCount.load(
		                std::memory_order_relaxed) == 0 &&
		            record->LastUse.IsComplete(completedValues));
	    });
	m_pendingOwnedMemoryBlockReleases.erase(heapEraseBegin, m_pendingOwnedMemoryBlockReleases.end());
}

void D3D12ResourceService::FlushDeferredResourceReleases() noexcept
{
	DrainCompletedResourceReleases();
}

RhiResourceHandle D3D12ResourceService::GetResourceHandle(RhiOwnedResourceHandle resource) const noexcept
{
	return RhiResourceHandle{GetD3D12Resource(resource)};
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

	std::wstring ownedDebugName = MakeDebugName(debugName, L"TransientMemoryBlock");
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

	DrainCompletedResourceReleases();
	m_pendingOwnedMemoryBlockReleases.push_back(std::move(ownedMemoryBlock));
}

RhiOwnedResourceHandle D3D12ResourceService::CreateAliasingTextureResource(
    RhiOwnedMemoryBlockHandle memoryBlock,
    std::uint64_t memoryBlockOffset,
    const RhiTransientTextureAllocationDesc& desc,
    std::wstring_view debugName)
{
	D3D12GpuHeapRecord* const ownedMemoryBlock = GetD3D12GpuHeapRecord(memoryBlock);
	if (m_rhi == nullptr || m_memoryAllocator == nullptr || m_capabilities == nullptr || ownedMemoryBlock == nullptr ||
	    !RhiContract::IsTextureResourceDescUsable(*m_capabilities, desc.ResourceDesc))
	{
		return {};
	}

	const D3D12_RESOURCE_DESC resourceDesc = D3D12TypeConversions::BuildTextureResourceDesc(desc.ResourceDesc);
	const D3D12_CLEAR_VALUE clearValue = D3D12TypeConversions::BuildClearValue(desc.ClearValue);
	const D3D12_CLEAR_VALUE* clearValuePtr = desc.ClearValue.ValueType == RhiOptimizedClearValue::Type::None ? nullptr : &clearValue;
	std::wstring ownedDebugName = MakeDebugName(debugName, L"AliasingTexture");
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
	std::wstring ownedDebugName = MakeDebugName(debugName, L"AliasingBuffer");
	std::unique_ptr<D3D12GpuAllocationRecord> ownedResource = m_memoryAllocator->CreateAliasingBuffer(
	    *ownedMemoryBlock,
	    memoryBlockOffset,
	    resourceDesc,
	    D3D12TypeConversions::ToResourceStates(desc.InitialState),
	    ownedDebugName);
	return ownedResource != nullptr ? WrapOwnedResource(std::move(ownedResource)) : RhiOwnedResourceHandle{};
}

bool D3D12ResourceService::SupportsUnorderedAccess(RhiResourceHandle resource) const noexcept
{
	return ResourceSupportsUnorderedAccess(static_cast<ID3D12Resource*>(resource.Value));
}

D3D12RecordingResourceUseToken D3D12ResourceService::BeginResourceTracking(
    RhiResourceHandle resource,
    bool coordinatorRecording) noexcept
{
	if (m_memoryAllocator == nullptr || !resource)
	{
		return {};
	}

	D3D12RecordingResourceUseToken use =
	    m_memoryAllocator->RetainRecordingResource(resource);
	if (!use && coordinatorRecording)
	{
		use = m_memoryAllocator->RetainCoordinatorRecordingResource(resource);
	}
	return use;
}

void D3D12ResourceService::EndResourceTracking(
    D3D12RecordingResourceUseToken use,
    RhiSubmissionToken submissionToken) noexcept
{
	if (m_memoryAllocator != nullptr)
	{
		m_memoryAllocator->ReleaseRecordingResource(use, submissionToken);
	}
}

std::wstring D3D12ResourceService::MakeDebugName(std::wstring_view debugName, std::wstring_view defaultDebugName)
{
	return debugName.empty() ? std::wstring(defaultDebugName) : std::wstring(debugName);
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

void D3D12ResourceService::CollectCrashDiagnosticsOnce() noexcept
{
	if (m_crashDiagnosticsCollected || m_rhi == nullptr || !m_rhi->SupportsCrashDiagnostics())
	{
		return;
	}

	m_crashDiagnosticsCollected = true;
	m_rhi->CollectCrashDiagnostics();

	RhiDiagnosticMessage message{};
	while (m_rhi->TryPopDebugMessage(message))
	{
		SPDLOG_LOGGER_WARN(Logging::GetOrCreateLogger("RHI.D3D12.Diagnostics"), "{}", message.Text);
	}
}

#include "PCH.h"

#include "D3D12/Resources/D3D12UploadService.h"

#include "Commands/RenderCommandList.h"
#include "D3D12/Commands/D3D12RecordingUploadPage.h"
#include "D3D12/Commands/D3D12RenderCommandList.h"
#include "D3D12/D3D12TypeConversions.h"
#include "D3D12/Device/D3D12Rhi.h"
#include "D3D12/Memory/D3D12GpuAllocation.h"
#include "D3D12/Memory/D3D12GpuMemoryAllocator.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <vector>

D3D12UploadService::D3D12UploadService(
    D3D12Rhi& rhi,
    D3D12GpuMemoryAllocator& memoryAllocator) noexcept :
	m_rhi(&rhi), m_memoryAllocator(&memoryAllocator)
{
}

D3D12UploadService::~D3D12UploadService() noexcept = default;

void D3D12UploadService::BeginFrame() noexcept
{
	DrainCompletedTextureUploads();
}

RhiGpuVirtualAddress D3D12UploadService::AllocateUniformConstantBuffer(
    RenderCommandList& commandList,
    const void* data,
    std::uint32_t sizeInBytes)
{
	if (commandList.GetBackendApi() != ERhiBackendApi::D3D12 || data == nullptr || sizeInBytes == 0)
	{
		return 0;
	}

	D3D12RecordingUploadPage* const uploadPage =
	    static_cast<D3D12RenderCommandList&>(commandList).GetRecordingUploadPage();
	return uploadPage != nullptr ? uploadPage->AllocateAndCopy(data, sizeInBytes) : 0;
}

bool D3D12UploadService::UploadTexture(
    RenderCommandList& commandList,
    RhiOwnedResourceHandle destination,
    const RhiTextureUploadDesc& textureUpload,
    ResourceState finalState,
    std::wstring_view debugName)
{
	D3D12GpuAllocationRecord* const destinationRecord = GetD3D12GpuAllocationRecord(destination);
	if (!ValidateTextureUploadRequest(commandList, destinationRecord, textureUpload))
	{
		return false;
	}

	auto& d3dCommandList = static_cast<D3D12RenderCommandList&>(commandList);
	if (d3dCommandList.GetD3D12CommandList() == nullptr)
	{
		return false;
	}

	const UINT subresourceCount = textureUpload.GetSubresourceCount();
	std::unique_ptr<D3D12GpuAllocationRecord> stagingResource =
	    CreateTextureStagingResource(*destinationRecord, subresourceCount, debugName);
	if (stagingResource == nullptr)
	{
		return false;
	}

	if (!RecordTextureUpload(
	        d3dCommandList,
	        *destinationRecord,
	        *stagingResource,
	        textureUpload,
	        finalState))
	{
		return false;
	}

	commandList.TrackResource(RhiResourceHandle{destinationRecord->Resource.Get()});
	commandList.TrackResource(RhiResourceHandle{stagingResource->Resource.Get()});

	DrainCompletedTextureUploads();
	m_pendingTextureUploads.push_back(PendingTextureUpload{.StagingResource = std::move(stagingResource)});
	return true;
}

bool D3D12UploadService::ValidateTextureUploadRequest(
    const RenderCommandList& commandList,
    const D3D12GpuAllocationRecord* destination,
    const RhiTextureUploadDesc& textureUpload) const noexcept
{
	return m_rhi != nullptr &&
	       m_memoryAllocator != nullptr &&
	       destination != nullptr &&
	       destination->Resource != nullptr &&
	       textureUpload.IsValid() &&
	       commandList.GetBackendApi() == ERhiBackendApi::D3D12;
}

std::unique_ptr<D3D12GpuAllocationRecord> D3D12UploadService::CreateTextureStagingResource(
    const D3D12GpuAllocationRecord& destination,
    std::uint32_t subresourceCount,
    std::wstring_view debugName)
{
	const UINT64 uploadBufferSize =
	    GetRequiredIntermediateSize(destination.Resource.Get(), 0, subresourceCount);
	if (uploadBufferSize == 0)
	{
		return {};
	}

	std::unique_ptr<D3D12GpuAllocationRecord> stagingResource = m_memoryAllocator->CreateBuffer(
	    CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
	    D3D12_RESOURCE_STATE_GENERIC_READ,
	    RhiMemoryCategory::Upload,
	    RhiMemoryResidencyClass::HostUpload,
	    debugName.empty() ? L"TextureUpload" : debugName);
	if (stagingResource == nullptr || stagingResource->Resource == nullptr)
	{
		return {};
	}

	return stagingResource;
}

bool D3D12UploadService::RecordTextureUpload(
    D3D12RenderCommandList& commandList,
    D3D12GpuAllocationRecord& destination,
    D3D12GpuAllocationRecord& stagingResource,
    const RhiTextureUploadDesc& textureUpload,
    ResourceState finalState)
{
	ID3D12GraphicsCommandList4* const nativeCommandList = commandList.GetD3D12CommandList();
	const UINT subresourceCount = textureUpload.GetSubresourceCount();

	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	subresources.reserve(subresourceCount);
	for (const RhiTextureArraySliceUploadData& arraySlice : textureUpload.ArraySlices)
	{
		for (const RhiTextureMipUploadData& mipLevel : arraySlice.MipLevels)
		{
			subresources.push_back(
			    D3D12_SUBRESOURCE_DATA{
			        .pData = mipLevel.Data.data(),
			        .RowPitch = static_cast<LONG_PTR>(mipLevel.RowPitch),
			        .SlicePitch = static_cast<LONG_PTR>(mipLevel.SlicePitch)});
		}
	}

	if (UpdateSubresources(
	        nativeCommandList,
	        destination.Resource.Get(),
	        stagingResource.Resource.Get(),
	        0,
	        0,
	        subresourceCount,
	        subresources.data()) == 0)
	{
		return false;
	}

	const ResourceState submittedFinalState =
	    commandList.GetQueueType() == ERhiQueueType::Copy ? ResourceState::Common : finalState;
	const D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
	    destination.Resource.Get(),
	    D3D12_RESOURCE_STATE_COPY_DEST,
	    D3D12TypeConversions::ToResourceStates(submittedFinalState));
	nativeCommandList->ResourceBarrier(1, &barrier);
	return true;
}

void D3D12UploadService::DrainCompletedTextureUploads() noexcept
{
	if (m_pendingTextureUploads.empty())
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
	const auto firstPending = std::remove_if(
	    m_pendingTextureUploads.begin(),
	    m_pendingTextureUploads.end(),
	    [&completedValues](const PendingTextureUpload& upload)
	    {
		    return upload.StagingResource == nullptr ||
		           (upload.StagingResource->RecordingReferenceCount.load(
		                std::memory_order_relaxed) == 0 &&
		            upload.StagingResource->LastUse.IsComplete(completedValues));
	    });
	m_pendingTextureUploads.erase(firstPending, m_pendingTextureUploads.end());
}

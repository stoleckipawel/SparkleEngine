#include "PCH.h"

#include "D3D12/Resources/D3D12UploadService.h"

#include "Commands/RenderCommandList.h"
#include "D3D12/D3D12TypeConversions.h"
#include "D3D12/Device/D3D12Rhi.h"
#include "D3D12/Memory/D3D12GpuAllocation.h"
#include "D3D12/Memory/D3D12GpuMemoryAllocator.h"
#include "D3D12/Resources/D3D12FrameResource.h"
#include "Interop/RhiInteropService.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <vector>

namespace
{
	const auto g_d3d12UploadLogger = Logging::GetOrCreateLogger("RHI.D3D12.Upload");
}

D3D12UploadService::D3D12UploadService(
    D3D12Rhi& rhi,
    D3D12FrameResourceManager& frameResourceManager,
    D3D12GpuMemoryAllocator& memoryAllocator) noexcept :
	m_rhi(&rhi), m_frameResourceManager(&frameResourceManager), m_memoryAllocator(&memoryAllocator)
{
}

D3D12UploadService::~D3D12UploadService() noexcept = default;

void D3D12UploadService::BeginFrame() noexcept
{
	DrainCompletedTextureUploads();
}

RhiGpuVirtualAddress D3D12UploadService::AllocateUniformConstantBuffer(const void* data, std::uint32_t sizeInBytes)
{
	if (m_frameResourceManager == nullptr || data == nullptr || sizeInBytes == 0)
	{
		return 0;
	}

	D3D12LinearAllocation allocation = m_frameResourceManager->GetCurrentAllocator().Allocate(sizeInBytes, 256);
	std::memcpy(allocation.CpuPtr, data, sizeInBytes);
	return allocation.GpuAddress;
}

bool D3D12UploadService::UploadTexture(
    RenderCommandList& commandList,
    RhiOwnedResourceHandle destination,
    const RhiTextureUploadDesc& textureUpload,
    ResourceState finalState,
    std::wstring_view debugName)
{
	D3D12GpuAllocationRecord* const destinationRecord = GetD3D12GpuAllocationRecord(destination);
	if (m_rhi == nullptr || m_memoryAllocator == nullptr || destinationRecord == nullptr || destinationRecord->Resource == nullptr ||
	    !textureUpload.IsValid() || commandList.GetBackendApi() != ERhiBackendApi::D3D12)
	{
		SPDLOG_LOGGER_ERROR(
		    g_d3d12UploadLogger,
		    "UploadTexture rejected invalid input (rhi={}, allocator={}, destination={}, uploadValid={}, backend={}).",
		    m_rhi != nullptr,
		    m_memoryAllocator != nullptr,
		    destinationRecord != nullptr && destinationRecord->Resource != nullptr,
		    textureUpload.IsValid(),
		    static_cast<std::uint32_t>(commandList.GetBackendApi()));
		return false;
	}

	const NativeGraphicsCommandListHandle nativeCommandList = commandList.GetNativeHandle(
	    RhiNativeInteropRequest{.Consumer = ERhiNativeInteropConsumer::Unknown, .Reason = "RHI texture upload"});
	auto* const d3dCommandList = static_cast<ID3D12GraphicsCommandList*>(nativeCommandList.Value);
	if (d3dCommandList == nullptr)
	{
		SPDLOG_LOGGER_ERROR(g_d3d12UploadLogger, "UploadTexture could not resolve the native command list.");
		return false;
	}

	const UINT subresourceCount = textureUpload.GetSubresourceCount();
	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(destinationRecord->Resource.Get(), 0, subresourceCount);
	if (uploadBufferSize == 0)
	{
		SPDLOG_LOGGER_ERROR(g_d3d12UploadLogger, "UploadTexture calculated an empty intermediate buffer.");
		return false;
	}

	std::unique_ptr<D3D12GpuAllocationRecord> stagingResource = m_memoryAllocator->CreateBuffer(
	    CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
	    D3D12_RESOURCE_STATE_GENERIC_READ,
	    RhiMemoryCategory::Upload,
	    RhiMemoryResidencyClass::HostUpload,
	    debugName.empty() ? L"TextureUpload" : debugName);
	if (stagingResource == nullptr || stagingResource->Resource == nullptr)
	{
		SPDLOG_LOGGER_ERROR(
		    g_d3d12UploadLogger,
		    "UploadTexture failed to allocate {} bytes of staging memory.",
		    uploadBufferSize);
		return false;
	}

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
	        d3dCommandList,
	        destinationRecord->Resource.Get(),
	        stagingResource->Resource.Get(),
	        0,
	        0,
	        subresourceCount,
	        subresources.data()) == 0)
	{
		SPDLOG_LOGGER_ERROR(
		    g_d3d12UploadLogger,
		    "UploadTexture failed to record {} subresources ({} staging bytes) on the {} queue.",
		    subresourceCount,
		    uploadBufferSize,
		    RhiQueueTypeToString(commandList.GetQueueType()));
		return false;
	}

	const ResourceState submittedFinalState =
	    commandList.GetQueueType() == ERhiQueueType::Copy ? ResourceState::Common : finalState;
	const D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
	    destinationRecord->Resource.Get(),
	    D3D12_RESOURCE_STATE_COPY_DEST,
	    D3D12TypeConversions::ToResourceStates(submittedFinalState));
	d3dCommandList->ResourceBarrier(1, &barrier);
	commandList.TrackResource(NativeResourceHandle{destinationRecord->Resource.Get()});
	commandList.TrackResource(NativeResourceHandle{stagingResource->Resource.Get()});

	DrainCompletedTextureUploads();
	m_pendingTextureUploads.push_back(PendingTextureUpload{.StagingResource = std::move(stagingResource)});
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
		           (upload.StagingResource->RecordingReferenceCount == 0 &&
		            upload.StagingResource->LastUse.IsComplete(completedValues));
	    });
	m_pendingTextureUploads.erase(firstPending, m_pendingTextureUploads.end());
}

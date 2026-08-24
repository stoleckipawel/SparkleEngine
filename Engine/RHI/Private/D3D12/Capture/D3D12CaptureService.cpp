#include "PCH.h"

#include "D3D12/Capture/D3D12CaptureService.h"

#include "Capture/RhiCaptureFormat.h"
#include "D3D12/D3D12TypeConversions.h"
#include "D3D12/Device/D3D12Rhi.h"

#include <d3d12.h>
#include <wrl/client.h>

#include <algorithm>
#include <cstring>

class D3D12CaptureCommands final
{
public:
	static void RecordTransition(
	    ID3D12GraphicsCommandList* commandList,
	    ID3D12Resource* resource,
	    ResourceState before,
	    ResourceState after) noexcept
	{
		if (commandList == nullptr || resource == nullptr || before == after)
		{
			return;
		}

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = resource;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12TypeConversions::ToResourceStates(before);
		barrier.Transition.StateAfter = D3D12TypeConversions::ToResourceStates(after);
		commandList->ResourceBarrier(1, &barrier);
	}
};

struct D3D12CaptureService::PendingReadback final
{
	RhiCaptureTicket Ticket;
	RhiTextureCaptureRequest Request;
	RhiSubmissionToken Submission;
	Microsoft::WRL::ComPtr<ID3D12Resource> Buffer;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT Footprint{};
	std::uint64_t TotalBytes = 0;
	PixelFormat Format = PixelFormat::Unknown;
	bool Cancelled = false;
};

D3D12CaptureService::D3D12CaptureService(D3D12Rhi& rhi) noexcept :
    m_rhi(&rhi)
{
}

D3D12CaptureService::~D3D12CaptureService() noexcept
{
	if (m_rhi != nullptr)
	{
		for (const std::unique_ptr<PendingReadback>& pending : m_pendingReadbacks)
		{
			if (pending && pending->Submission.IsValid())
			{
				m_rhi->WaitForSubmission(pending->Submission);
			}
		}
	}
	m_pendingReadbacks.clear();
}

RhiCaptureTicket D3D12CaptureService::BeginTextureReadback(const RhiTextureCaptureRequest& request) noexcept
{
	DrainCancelledReadbacks();
	ID3D12Device* const device = m_rhi != nullptr ? m_rhi->GetDevice().Get() : nullptr;
	ID3D12Resource* const sourceResource = static_cast<ID3D12Resource*>(request.Resource.Value);
	if (device == nullptr || sourceResource == nullptr)
	{
		return {};
	}

	const D3D12_RESOURCE_DESC sourceDesc = sourceResource->GetDesc();
	RhiCaptureFormat captureFormat;
	if (sourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D || sourceDesc.Width == 0 || sourceDesc.Height == 0
	    || !TryResolveRhiCaptureFormat(request.SourceFormat, captureFormat)
	    || sourceDesc.Format != D3D12TypeConversions::ToDxgiFormat(request.SourceFormat))
	{
		return {};
	}

	auto pending = std::make_unique<PendingReadback>();
	pending->Ticket = RhiCaptureTicket{m_nextTicket++};
	pending->Request = request;
	pending->Format = request.SourceFormat;
	UINT rowCount = 0;
	UINT64 rowSizeInBytes = 0;
	device->GetCopyableFootprints(&sourceDesc, 0, 1, 0, &pending->Footprint, &rowCount, &rowSizeInBytes, &pending->TotalBytes);
	if (pending->TotalBytes == 0 || rowCount == 0)
	{
		return {};
	}

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;
	D3D12_RESOURCE_DESC readbackDesc{};
	readbackDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	readbackDesc.Width = pending->TotalBytes;
	readbackDesc.Height = 1;
	readbackDesc.DepthOrArraySize = 1;
	readbackDesc.MipLevels = 1;
	readbackDesc.SampleDesc.Count = 1;
	readbackDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	if (FAILED(device->CreateCommittedResource(
	        &heapProperties,
	        D3D12_HEAP_FLAG_NONE,
	        &readbackDesc,
	        D3D12_RESOURCE_STATE_COPY_DEST,
	        nullptr,
	        IID_PPV_ARGS(&pending->Buffer)))
	    || FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pending->CommandAllocator)))
	    || FAILED(device->CreateCommandList(
	        0,
	        D3D12_COMMAND_LIST_TYPE_DIRECT,
	        pending->CommandAllocator.Get(),
	        nullptr,
	        IID_PPV_ARGS(&pending->CommandList))))
	{
		return {};
	}

	D3D12CaptureCommands::RecordTransition(pending->CommandList.Get(), sourceResource, request.SourceState, ResourceState::CopySource);
	D3D12_TEXTURE_COPY_LOCATION sourceLocation{};
	sourceLocation.pResource = sourceResource;
	sourceLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	D3D12_TEXTURE_COPY_LOCATION destinationLocation{};
	destinationLocation.pResource = pending->Buffer.Get();
	destinationLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	destinationLocation.PlacedFootprint = pending->Footprint;
	pending->CommandList->CopyTextureRegion(&destinationLocation, 0, 0, 0, &sourceLocation, nullptr);
	D3D12CaptureCommands::RecordTransition(pending->CommandList.Get(), sourceResource, ResourceState::CopySource, request.SourceState);
	if (FAILED(pending->CommandList->Close()))
	{
		return {};
	}

	ID3D12CommandList* const commandLists[] = {pending->CommandList.Get()};
	pending->Submission = m_rhi->SubmitCommandLists(ERhiQueueType::Graphics, commandLists);
	if (!pending->Submission.IsValid())
	{
		return {};
	}

	const RhiCaptureTicket ticket = pending->Ticket;
	m_pendingReadbacks.push_back(std::move(pending));
	return ticket;
}

bool D3D12CaptureService::TryTakeTextureReadback(RhiCaptureTicket ticket, RhiCaptureReadback& readback) noexcept
{
	DrainCancelledReadbacks();
	PendingReadback* pending = FindPending(ticket);
	if (pending == nullptr || m_rhi == nullptr || !m_rhi->IsSubmissionComplete(pending->Submission))
	{
		return false;
	}

	void* mappedData = nullptr;
	const D3D12_RANGE readRange{0, static_cast<SIZE_T>(pending->TotalBytes)};
	if (FAILED(pending->Buffer->Map(0, &readRange, &mappedData)) || mappedData == nullptr)
	{
		return false;
	}

	readback.Result = RhiCaptureResult{
	    .Status = ERhiCaptureStatus::Succeeded,
	    .BackendApi = ERhiBackendApi::D3D12,
	    .FrameId = pending->Request.FrameId,
	    .ViewMode = pending->Request.ViewMode,
	    .ViewModeName = pending->Request.ViewModeName,
	    .ArtifactPath = pending->Request.OutputPath};
	readback.Width = pending->Footprint.Footprint.Width;
	readback.Height = pending->Footprint.Footprint.Height;
	readback.RowPitch = pending->Footprint.Footprint.RowPitch;
	readback.Format = pending->Format;
	const std::size_t byteCount = static_cast<std::size_t>(readback.RowPitch) * readback.Height;
	readback.Pixels.resize(byteCount);
	const std::byte* source = static_cast<const std::byte*>(mappedData) + pending->Footprint.Offset;
	std::memcpy(readback.Pixels.data(), source, byteCount);
	const D3D12_RANGE writeRange{0, 0};
	pending->Buffer->Unmap(0, &writeRange);

	const auto iterator = std::find_if(
	    m_pendingReadbacks.begin(),
	    m_pendingReadbacks.end(),
	    [ticket](const std::unique_ptr<PendingReadback>& candidate) { return candidate && candidate->Ticket.Value == ticket.Value; });
	if (iterator != m_pendingReadbacks.end())
	{
		m_pendingReadbacks.erase(iterator);
	}
	return true;
}

void D3D12CaptureService::CancelTextureReadback(RhiCaptureTicket ticket) noexcept
{
	const auto iterator = std::find_if(
	    m_pendingReadbacks.begin(),
	    m_pendingReadbacks.end(),
	    [ticket](const std::unique_ptr<PendingReadback>& candidate) { return candidate && candidate->Ticket.Value == ticket.Value; });
	if (iterator == m_pendingReadbacks.end())
	{
		return;
	}
	(*iterator)->Cancelled = true;
	DrainCancelledReadbacks();
}

D3D12CaptureService::PendingReadback* D3D12CaptureService::FindPending(RhiCaptureTicket ticket) noexcept
{
	for (const std::unique_ptr<PendingReadback>& pending : m_pendingReadbacks)
	{
		if (pending && pending->Ticket.Value == ticket.Value)
		{
			return pending.get();
		}
	}
	return nullptr;
}

void D3D12CaptureService::DrainCancelledReadbacks() noexcept
{
	if (m_rhi == nullptr)
	{
		return;
	}
	for (std::size_t index = 0; index < m_pendingReadbacks.size();)
	{
		const std::unique_ptr<PendingReadback>& pending = m_pendingReadbacks[index];
		if (!pending || !pending->Cancelled || !m_rhi->IsSubmissionComplete(pending->Submission))
		{
			++index;
			continue;
		}
		m_pendingReadbacks.erase(m_pendingReadbacks.begin() + index);
	}
}

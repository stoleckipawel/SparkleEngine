#include "D3D12/Capture/D3D12CaptureService.h"

#include "Capture/RhiBmpWriter.h"
#include "D3D12/D3D12RenderHardwareInterface.h"
#include "D3D12/D3D12TypeConversions.h"

#include <d3d12.h>
#include <wrl/client.h>

namespace
{
	bool TryMapCaptureFormat(DXGI_FORMAT sourceFormat, RhiBmpSourceFormat& outFormat) noexcept
	{
		switch (sourceFormat)
		{
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
			outFormat = RhiBmpSourceFormat::Rgba32Float;
			return true;
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			outFormat = RhiBmpSourceFormat::Rgba8Unorm;
			return true;
		case DXGI_FORMAT_B8G8R8A8_UNORM:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
			outFormat = RhiBmpSourceFormat::Bgra8Unorm;
			return true;
		default:
			return false;
		}
	}

	void RecordCaptureTransition(
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
}

D3D12CaptureService::D3D12CaptureService(D3D12RenderHardwareInterface& owner) noexcept : m_owner(&owner) {}

RhiCaptureResult D3D12CaptureService::CaptureTextureToBmp(const RhiTextureCaptureRequest& request) noexcept
{
	const bool captured = CaptureNativeTextureToBmp(request.Resource, request.SourceState, request.OutputPath);
	return RhiCaptureResult{
	    .Status = captured ? ERhiCaptureStatus::Succeeded : ERhiCaptureStatus::Failed,
	    .BackendApi = ERhiBackendApi::D3D12,
	    .FrameIndex = request.FrameIndex,
	    .ViewMode = request.ViewMode,
	    .ViewModeName = request.ViewModeName,
	    .ArtifactPath = captured ? request.OutputPath : std::filesystem::path{},
	    .FailureReason = captured ? "" : "D3D12 texture capture failed; verify the resource is a valid Texture2D and the output path is writable."};
}

bool D3D12CaptureService::CaptureNativeTextureToBmp(
    NativeResourceHandle resource,
    ResourceState sourceState,
    const std::filesystem::path& outputPath) noexcept
{
	ID3D12Device* const device = m_owner != nullptr ? static_cast<ID3D12Device*>(m_owner->GetDeviceHandle().Value) : nullptr;
	ID3D12CommandQueue* const queue = m_owner != nullptr ? static_cast<ID3D12CommandQueue*>(m_owner->GetGraphicsQueueHandle().Value) : nullptr;
	ID3D12Resource* const sourceResource = static_cast<ID3D12Resource*>(resource.Value);
	if (device == nullptr || queue == nullptr || sourceResource == nullptr)
	{
		return false;
	}

	const D3D12_RESOURCE_DESC sourceDesc = sourceResource->GetDesc();
	if (sourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D || sourceDesc.Width == 0 || sourceDesc.Height == 0)
	{
		return false;
	}
	RhiBmpSourceFormat captureFormat = RhiBmpSourceFormat::Rgba8Unorm;
	if (!TryMapCaptureFormat(sourceDesc.Format, captureFormat))
	{
		return false;
	}

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
	UINT rowCount = 0;
	UINT64 rowSizeInBytes = 0;
	UINT64 totalBytes = 0;
	device->GetCopyableFootprints(&sourceDesc, 0, 1, 0, &footprint, &rowCount, &rowSizeInBytes, &totalBytes);
	if (totalBytes == 0 || rowCount == 0)
	{
		return false;
	}

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC readbackDesc{};
	readbackDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	readbackDesc.Width = totalBytes;
	readbackDesc.Height = 1;
	readbackDesc.DepthOrArraySize = 1;
	readbackDesc.MipLevels = 1;
	readbackDesc.SampleDesc.Count = 1;
	readbackDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	Microsoft::WRL::ComPtr<ID3D12Resource> readbackBuffer;
	if (FAILED(device->CreateCommittedResource(
	        &heapProperties,
	        D3D12_HEAP_FLAG_NONE,
	        &readbackDesc,
	        D3D12_RESOURCE_STATE_COPY_DEST,
	        nullptr,
	        IID_PPV_ARGS(&readbackBuffer))))
	{
		return false;
	}

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
	if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator))) ||
	    FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList))))
	{
		return false;
	}

	RecordCaptureTransition(commandList.Get(), sourceResource, sourceState, ResourceState::CopySource);

	D3D12_TEXTURE_COPY_LOCATION sourceLocation{};
	sourceLocation.pResource = sourceResource;
	sourceLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

	D3D12_TEXTURE_COPY_LOCATION destinationLocation{};
	destinationLocation.pResource = readbackBuffer.Get();
	destinationLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	destinationLocation.PlacedFootprint = footprint;
	commandList->CopyTextureRegion(&destinationLocation, 0, 0, 0, &sourceLocation, nullptr);

	RecordCaptureTransition(commandList.Get(), sourceResource, ResourceState::CopySource, sourceState);

	if (FAILED(commandList->Close()))
	{
		return false;
	}

	ID3D12CommandList* const commandLists[] = {commandList.Get()};
	queue->ExecuteCommandLists(1, commandLists);

	Microsoft::WRL::ComPtr<ID3D12Fence> fence;
	if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))) || FAILED(queue->Signal(fence.Get(), 1)))
	{
		return false;
	}

	if (fence->GetCompletedValue() < 1)
	{
		const HANDLE fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
		if (fenceEvent == nullptr)
		{
			return false;
		}
		const HRESULT eventResult = fence->SetEventOnCompletion(1, fenceEvent);
		if (FAILED(eventResult))
		{
			CloseHandle(fenceEvent);
			return false;
		}
		WaitForSingleObject(fenceEvent, INFINITE);
		CloseHandle(fenceEvent);
	}

	void* mappedData = nullptr;
	D3D12_RANGE readRange{0, static_cast<SIZE_T>(totalBytes)};
	if (FAILED(readbackBuffer->Map(0, &readRange, &mappedData)) || mappedData == nullptr)
	{
		return false;
	}

	const std::byte* sourcePixels = static_cast<const std::byte*>(mappedData) + footprint.Offset;
	const bool wroteCapture = WriteRhiBmp(
	    outputPath,
	    sourcePixels,
	    footprint.Footprint.Width,
	    footprint.Footprint.Height,
	    footprint.Footprint.RowPitch,
	    captureFormat);
	const D3D12_RANGE writeRange{0, 0};
	readbackBuffer->Unmap(0, &writeRange);
	return wroteCapture;
}

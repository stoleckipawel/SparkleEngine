#include "PCH.h"

#include "D3D12/D3D12RenderHardwareInterface.h"

#include "D3D12/Commands/D3D12RenderCommandList.h"
#include "D3D12/Device/D3D12ExternalFeatureInteropCapabilities.h"
#include "D3D12/Device/D3D12Rhi.h"
#include "D3D12/SwapChain/D3D12SwapChain.h"
#include "D3D12/D3D12TypeConversions.h"
#include "D3D12/Diagnostics/D3D12RenderDiagnostics.h"
#include "D3D12/Descriptors/D3D12DescriptorHeap.h"
#include "D3D12/Descriptors/D3D12DescriptorHeapManager.h"
#include "D3D12/Memory/D3D12GpuMemoryAllocator.h"
#include "D3D12/Pipeline/D3D12BindingLayout.h"
#include "D3D12/Pipeline/D3D12PipelineState.h"
#include "D3D12/RayTracing/D3D12RayTracingServices.h"
#include "D3D12/Resources/D3D12ConstantBufferManager.h"
#include "D3D12/Samplers/D3D12SamplerLibrary.h"
#include "D3D12/UI/D3D12ImGuiBackend.h"
#include "Resources/Texture.h"
#include "D3D12/Textures/TextureFactory.h"
#include "RHI/Public/Validation/RhiValidation.h"
#include "Shaders/CookedShaderPackage.h"

#include <algorithm>
#include <d3d12.h>
#include <wrl/client.h>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace
{
#pragma pack(push, 1)
	struct DiagnosticBmpFileHeader final
	{
		std::uint16_t Type = 0x4D42;
		std::uint32_t Size = 0;
		std::uint16_t Reserved1 = 0;
		std::uint16_t Reserved2 = 0;
		std::uint32_t OffBits = 54;
	};

	struct DiagnosticBmpInfoHeader final
	{
		std::uint32_t Size = sizeof(DiagnosticBmpInfoHeader);
		std::int32_t Width = 0;
		std::int32_t Height = 0;
		std::uint16_t Planes = 1;
		std::uint16_t BitCount = 32;
		std::uint32_t Compression = 0;
		std::uint32_t SizeImage = 0;
		std::int32_t XPelsPerMeter = 2835;
		std::int32_t YPelsPerMeter = 2835;
		std::uint32_t ClrUsed = 0;
		std::uint32_t ClrImportant = 0;
	};
#pragma pack(pop)

	std::byte ToByte(float value) noexcept
	{
		const float clamped = std::clamp(value, 0.0f, 1.0f);
		return static_cast<std::byte>(static_cast<std::uint32_t>(clamped * 255.0f + 0.5f));
	}

	bool WriteDiagnosticBmp(
	    const std::filesystem::path& outputPath,
	    const std::byte* sourcePixels,
	    std::uint32_t width,
	    std::uint32_t height,
	    std::uint32_t sourceRowPitch,
	    DXGI_FORMAT sourceFormat) noexcept
	{
		if (sourcePixels == nullptr || width == 0 || height == 0)
		{
			return false;
		}

		std::error_code error;
		if (const std::filesystem::path parentPath = outputPath.parent_path(); !parentPath.empty())
		{
			std::filesystem::create_directories(parentPath, error);
			if (error)
			{
				return false;
			}
		}

		const std::uint32_t outputRowPitch = width * 4u;
		std::vector<std::byte> outputPixels(static_cast<std::size_t>(outputRowPitch) * height);
		for (std::uint32_t y = 0; y < height; ++y)
		{
			const std::byte* sourceRow = sourcePixels + static_cast<std::size_t>(sourceRowPitch) * y;
			std::byte* outputRow = outputPixels.data() + static_cast<std::size_t>(outputRowPitch) * y;
			for (std::uint32_t x = 0; x < width; ++x)
			{
				std::byte* outputPixel = outputRow + static_cast<std::size_t>(x) * 4u;
				if (sourceFormat == DXGI_FORMAT_R32G32B32A32_FLOAT)
				{
					const float* rgba = reinterpret_cast<const float*>(sourceRow + static_cast<std::size_t>(x) * 16u);
					outputPixel[0] = ToByte(rgba[2]);
					outputPixel[1] = ToByte(rgba[1]);
					outputPixel[2] = ToByte(rgba[0]);
					outputPixel[3] = ToByte(rgba[3]);
				}
				else if (sourceFormat == DXGI_FORMAT_R8G8B8A8_UNORM || sourceFormat == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)
				{
					const std::byte* sourcePixel = sourceRow + static_cast<std::size_t>(x) * 4u;
					outputPixel[0] = sourcePixel[2];
					outputPixel[1] = sourcePixel[1];
					outputPixel[2] = sourcePixel[0];
					outputPixel[3] = sourcePixel[3];
				}
				else if (sourceFormat == DXGI_FORMAT_B8G8R8A8_UNORM || sourceFormat == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB)
				{
					const std::byte* sourcePixel = sourceRow + static_cast<std::size_t>(x) * 4u;
					outputPixel[0] = sourcePixel[0];
					outputPixel[1] = sourcePixel[1];
					outputPixel[2] = sourcePixel[2];
					outputPixel[3] = sourcePixel[3];
				}
				else
				{
					return false;
				}
			}
		}

		DiagnosticBmpFileHeader fileHeader{};
		DiagnosticBmpInfoHeader infoHeader{};
		infoHeader.Width = static_cast<std::int32_t>(width);
		infoHeader.Height = -static_cast<std::int32_t>(height);
		infoHeader.SizeImage = static_cast<std::uint32_t>(outputPixels.size());
		fileHeader.Size = fileHeader.OffBits + infoHeader.SizeImage;

		std::ofstream output(outputPath, std::ios::binary);
		if (!output)
		{
			return false;
		}

		output.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
		output.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));
		output.write(reinterpret_cast<const char*>(outputPixels.data()), static_cast<std::streamsize>(outputPixels.size()));
		return output.good();
	}
}

D3D12RenderHardwareInterface::D3D12RenderHardwareInterface(
    D3D12Rhi& rhi,
	D3D12GpuMemoryAllocator& memoryAllocator,
    D3D12DescriptorHeapManager& descriptorHeapManager,
    D3D12SwapChain& swapChain,
    D3D12ConstantBufferManager& constantBufferManager) noexcept :
	m_interopService(*this), m_captureService(*this), m_diagnosticsService(*this), m_presentationService(*this), m_rhi(&rhi),
	m_memoryAllocator(&memoryAllocator), m_descriptorHeapManager(&descriptorHeapManager), m_swapChain(&swapChain),
	m_constantBufferManager(&constantBufferManager)
{
	m_rayTracingServices = std::make_unique<D3D12RayTracingServices>(rhi, memoryAllocator);
	for (std::uint32_t frameIndex = 0; frameIndex < RenderConfig::FramesInFlight; ++frameIndex)
	{
		m_commandLists[frameIndex] = std::make_unique<D3D12RenderCommandList>(*this, rhi.GetCommandList(frameIndex).Get());
	}

	m_capabilities = BuildCapabilities();
	m_diagnostics = CreateD3D12RenderDiagnostics(rhi);
	m_imguiBackend = std::make_unique<D3D12ImGuiBackend>(*this);
}

D3D12RenderHardwareInterface::~D3D12RenderHardwareInterface() noexcept = default;

ERhiBackendApi D3D12RenderHardwareInterface::GetBackendApi() const noexcept
{
	return ERhiBackendApi::D3D12;
}

CookedShaderBinaryFormat D3D12RenderHardwareInterface::GetRequiredShaderBinaryFormat() const noexcept
{
	return CookedShaderBinaryFormat::Dxil;
}

std::wstring D3D12RenderHardwareInterface::CopyDebugName(std::wstring_view debugName, std::wstring_view fallbackName)
{
	return debugName.empty() ? std::wstring(fallbackName) : std::wstring(debugName);
}

bool D3D12RenderHardwareInterface::ResourceSupportsUnorderedAccess(ID3D12Resource* resource) noexcept
{
	return resource != nullptr && (resource->GetDesc().Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0;
}

RhiOwnedResourceHandle D3D12RenderHardwareInterface::WrapOwnedResource(std::unique_ptr<D3D12GpuAllocationRecord> record) noexcept
{
	return MakeD3D12OwnedResourceHandle(std::move(record));
}

RhiOwnedResourceHandle D3D12RenderHardwareInterface::WrapOwnedResource(
    Microsoft::WRL::ComPtr<ID3D12Resource>&& resource,
    std::wstring debugName) noexcept
{
	if (resource == nullptr)
	{
		return {};
	}

	auto record = std::make_unique<D3D12GpuAllocationRecord>();
	record->Resource = std::move(resource);
	record->DebugName = std::move(debugName);
	return MakeD3D12OwnedResourceHandle(std::move(record));
}

RhiOwnedMemoryBlockHandle D3D12RenderHardwareInterface::WrapOwnedMemoryBlock(std::unique_ptr<D3D12GpuHeapRecord> record) noexcept
{
	if (record == nullptr)
	{
		return {};
	}

	return MakeD3D12OwnedMemoryBlockHandle(std::move(record));
}

RhiCapabilities D3D12RenderHardwareInterface::BuildCapabilities() const noexcept
{
	RhiCapabilities capabilities{};
	capabilities.BackendApi = ERhiBackendApi::D3D12;
	capabilities.RequiredShaderBinaryFormat = CookedShaderBinaryFormat::Dxil;
	capabilities.DescriptorModel = ERhiDescriptorModel::DescriptorTables;
	capabilities.BindingLimits = RhiBindingLimits{
	    .MaxDescriptorSets = 1,
	    .MaxShaderResourceDescriptors = D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2,
	    .MaxSamplerDescriptors = D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE,
	    .MaxDescriptorTableEntries = D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2,
	    .MaxPushConstantBytes = 256};
	capabilities.UploadReadback = RhiUploadReadbackCapabilities{
	    .SupportsBufferUpload = true,
	    .SupportsTextureUpload = true,
	    .SupportsReadback = true};
	for (std::size_t index = 0; index < capabilities.FormatSupport.size(); ++index)
	{
		capabilities.FormatSupport[index] = QueryFormatSupport(kRhiCapabilityPixelFormats[index]);
	}
	capabilities.SupportsTimestampQueries = m_rhi != nullptr && m_rhi->GetCommandQueue() != nullptr;
	capabilities.RayTracing = m_rhi != nullptr ? m_rhi->GetRayTracingCapabilities() : RhiRayTracingCapabilities{};
	capabilities.SupportsMeshShaders = false;
	capabilities.SupportsTaskShaders = false;
	capabilities.Queues = RhiQueueCapabilities{.SupportsGraphics = true, .SupportsCompute = false, .SupportsCopy = false};
	capabilities.SupportsPresent = m_swapChain != nullptr && m_swapChain->GetBackBufferFormat() != PixelFormat::Unknown;
	capabilities.MemoryAllocator = ERhiMemoryAllocatorBackend::D3D12Managed;
	capabilities.ExternalFeatureInterop = BuildD3D12ExternalFeatureInteropCapabilities(
	    m_rhi,
	    !m_commandLists.empty() && m_commandLists[0] != nullptr);
	return capabilities;
}

RhiFormatSupport D3D12RenderHardwareInterface::QueryFormatSupport(PixelFormat format) const noexcept
{
	RhiFormatSupport support{.Format = format};
	if (m_rhi == nullptr || m_rhi->GetDevice() == nullptr || format == PixelFormat::Unknown)
	{
		return support;
	}

	D3D12_FEATURE_DATA_FORMAT_SUPPORT data{};
	data.Format = D3D12TypeConversions::ToDxgiFormat(format);
	if (data.Format == DXGI_FORMAT_UNKNOWN || FAILED(m_rhi->GetDevice()->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &data, sizeof(data))))
	{
		return support;
	}

	support.SupportsTexture = (data.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURE2D) != 0;
	support.SupportsShaderResource = (data.Support1 & (D3D12_FORMAT_SUPPORT1_SHADER_LOAD | D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE)) != 0;
	support.SupportsUnorderedAccess = (data.Support2 & (D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD | D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE)) != 0;
	support.SupportsRenderTarget = (data.Support1 & D3D12_FORMAT_SUPPORT1_RENDER_TARGET) != 0;
	support.SupportsDepthStencil = (data.Support1 & D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL) != 0;
	return support;
}

std::uint32_t D3D12RenderHardwareInterface::GetCurrentFrameIndex() const noexcept
{
	return m_rhi != nullptr ? m_rhi->GetCurrentFrameIndex() : 0u;
}

void D3D12RenderHardwareInterface::WaitForIdle() noexcept
{
	if (m_rhi != nullptr)
	{
		m_rhi->Flush();
		DrainCompletedOwnedResourceReleases();
	}
}

RhiInteropService& D3D12RenderHardwareInterface::GetInteropService() noexcept
{
	return m_interopService;
}

const RhiInteropService& D3D12RenderHardwareInterface::GetInteropService() const noexcept
{
	return m_interopService;
}

RhiCaptureService& D3D12RenderHardwareInterface::GetCaptureService() noexcept
{
	return m_captureService;
}

RhiDiagnosticsService& D3D12RenderHardwareInterface::GetDiagnosticsService() noexcept
{
	return m_diagnosticsService;
}

const RhiDiagnosticsService& D3D12RenderHardwareInterface::GetDiagnosticsService() const noexcept
{
	return m_diagnosticsService;
}

RhiPresentationService& D3D12RenderHardwareInterface::GetPresentationService() noexcept
{
	return m_presentationService;
}

const RhiPresentationService& D3D12RenderHardwareInterface::GetPresentationService() const noexcept
{
	return m_presentationService;
}

RhiNativeDeviceQueueInterop D3D12RenderHardwareInterface::InteropService::GetDeviceQueueInterop(
    RhiNativeInteropRequest request) const noexcept
{
	return RhiNativeDeviceQueueInterop{
	    .BackendApi = m_owner != nullptr ? m_owner->GetBackendApi() : ERhiBackendApi::Unknown,
	    .Device = GetDeviceHandle(),
	    .GraphicsQueue = GetGraphicsQueueHandle(),
	    .Request = request};
}

NativeGraphicsDeviceHandle D3D12RenderHardwareInterface::InteropService::GetDeviceHandle() const noexcept
{
	return m_owner != nullptr ? m_owner->GetDeviceHandle() : NativeGraphicsDeviceHandle{};
}

NativeGraphicsQueueHandle D3D12RenderHardwareInterface::InteropService::GetGraphicsQueueHandle() const noexcept
{
	return m_owner != nullptr ? m_owner->GetGraphicsQueueHandle() : NativeGraphicsQueueHandle{};
}

bool D3D12RenderHardwareInterface::InteropService::UpgradePresentationInterface(
    RhiNativeInterfaceUpgradeCallback callback,
    void* userData) noexcept
{
	return m_owner != nullptr && m_owner->UpgradePresentationInterface(callback, userData);
}

NativeTextureViewInfo D3D12RenderHardwareInterface::InteropService::GetNativeTextureViewInfo(
    RhiResourceViewHandle view,
    ResourceState state) const noexcept
{
	return m_owner != nullptr ? m_owner->GetNativeTextureViewInfo(view, state) : NativeTextureViewInfo{};
}

NativeGraphicsDeviceHandle D3D12RenderHardwareInterface::GetDeviceHandle() const noexcept
{
	return NativeGraphicsDeviceHandle{m_rhi != nullptr ? m_rhi->GetDevice().Get() : nullptr};
}

NativeGraphicsQueueHandle D3D12RenderHardwareInterface::GetGraphicsQueueHandle() const noexcept
{
	return NativeGraphicsQueueHandle{m_rhi != nullptr ? m_rhi->GetCommandQueue().Get() : nullptr};
}

bool D3D12RenderHardwareInterface::UpgradePresentationInterface(RhiNativeInterfaceUpgradeCallback callback, void* userData) noexcept
{
	return m_swapChain != nullptr && m_swapChain->UpgradeNativeInterface(callback, userData);
}

RhiCaptureResult D3D12RenderHardwareInterface::CaptureService::CaptureTextureToBmp(const RhiTextureCaptureRequest& request) noexcept
{
	const bool captured =
	    m_owner != nullptr && m_owner->CaptureTextureToBmp(request.Resource, request.Width, request.Height, request.OutputPath);
	return RhiCaptureResult{
	    .Status = captured ? ERhiCaptureStatus::Succeeded : ERhiCaptureStatus::Failed,
	    .BackendApi = ERhiBackendApi::D3D12,
	    .FrameIndex = request.FrameIndex,
	    .ViewMode = request.ViewMode,
	    .ViewModeName = request.ViewModeName,
	    .ArtifactPath = captured ? request.OutputPath : std::filesystem::path{},
	    .FailureReason = captured ? "" : "D3D12 texture capture failed; verify the resource is a valid Texture2D and the output path is writable."};
}

bool D3D12RenderHardwareInterface::CaptureTextureToBmp(
    NativeResourceHandle resource,
    std::uint32_t,
    std::uint32_t,
    const std::filesystem::path& outputPath) noexcept
{
	ID3D12Device* const device = m_rhi != nullptr ? m_rhi->GetDevice().Get() : nullptr;
	ID3D12CommandQueue* const queue = m_rhi != nullptr ? m_rhi->GetCommandQueue().Get() : nullptr;
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

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = sourceResource;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
	commandList->ResourceBarrier(1, &barrier);

	D3D12_TEXTURE_COPY_LOCATION sourceLocation{};
	sourceLocation.pResource = sourceResource;
	sourceLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

	D3D12_TEXTURE_COPY_LOCATION destinationLocation{};
	destinationLocation.pResource = readbackBuffer.Get();
	destinationLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	destinationLocation.PlacedFootprint = footprint;
	commandList->CopyTextureRegion(&destinationLocation, 0, 0, 0, &sourceLocation, nullptr);

	std::swap(barrier.Transition.StateBefore, barrier.Transition.StateAfter);
	commandList->ResourceBarrier(1, &barrier);

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
	const bool wroteCapture = WriteDiagnosticBmp(
	    outputPath,
	    sourcePixels,
	    footprint.Footprint.Width,
	    footprint.Footprint.Height,
	    footprint.Footprint.RowPitch,
	    sourceDesc.Format);
	const D3D12_RANGE writeRange{0, 0};
	readbackBuffer->Unmap(0, &writeRange);
	return wroteCapture;
}

RenderCommandList& D3D12RenderHardwareInterface::GetGraphicsCommandList(std::uint32_t frameIndex) noexcept
{
	DrainCompletedOwnedResourceReleases();
	return *m_commandLists[frameIndex];
}

RhiRayTracingCapabilities D3D12RenderHardwareInterface::GetRayTracingCapabilities() const noexcept
{
	return m_rayTracingServices != nullptr ? m_rayTracingServices->GetCapabilities() : RhiRayTracingCapabilities{};
}

RenderDiagnostics& D3D12RenderHardwareInterface::GetDiagnostics() noexcept
{
	return *m_diagnostics;
}

const RenderDiagnostics& D3D12RenderHardwareInterface::GetDiagnostics() const noexcept
{
	return *m_diagnostics;
}

RenderDiagnostics& D3D12RenderHardwareInterface::DiagnosticsService::GetDiagnostics() noexcept
{
	return m_owner->GetDiagnostics();
}

const RenderDiagnostics& D3D12RenderHardwareInterface::DiagnosticsService::GetDiagnostics() const noexcept
{
	return m_owner->GetDiagnostics();
}

RhiImGuiRenderer& D3D12RenderHardwareInterface::GetImGuiRenderer() noexcept
{
	return *m_imguiBackend;
}

std::unique_ptr<RenderBindingSet> D3D12RenderHardwareInterface::CreateBindingSet(const RenderBindingSetDesc& desc)
{
	return std::make_unique<RenderBindingSet>(*this, desc);
}

std::unique_ptr<RenderBindingLayout> D3D12RenderHardwareInterface::CreateBindingLayout(const RenderBindingLayoutCompileDesc& desc)
{
	if (m_rhi == nullptr || desc.ParameterLayout == nullptr || desc.ShaderPackage == nullptr)
	{
		return {};
	}

	return D3D12BindingLayoutCompiler::Compile(*m_rhi, desc);
}

std::unique_ptr<RenderPipelineState> D3D12RenderHardwareInterface::CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc)
{
	if (m_rhi == nullptr || desc.BindingLayout == nullptr || !desc.VertexShader.IsValid())
	{
		return {};
	}

	return std::make_unique<D3D12PipelineState>(*m_rhi, desc);
}

std::unique_ptr<RenderPipelineState> D3D12RenderHardwareInterface::CreateComputePipelineState(const ComputePipelineStateDesc& desc)
{
	if (m_rhi == nullptr || desc.BindingLayout == nullptr || !desc.ComputeShader.IsValid())
	{
		return {};
	}

	return std::make_unique<D3D12PipelineState>(*m_rhi, desc);
}

void D3D12RenderHardwareInterface::BindGlobalDescriptorState(RenderCommandList& commandList) const noexcept
{
	if (m_descriptorHeapManager != nullptr && commandList.GetBackendApi() == ERhiBackendApi::D3D12)
	{
		m_descriptorHeapManager->BindGlobalDescriptorState(static_cast<D3D12RenderCommandList&>(commandList));
	}
}

ID3D12DescriptorHeap* D3D12RenderHardwareInterface::GetD3D12ShaderResourceDescriptorHeap() const noexcept
{
	if (m_descriptorHeapManager == nullptr)
	{
		return nullptr;
	}

	D3D12DescriptorHeap* heap = m_descriptorHeapManager->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	return heap != nullptr ? heap->GetRaw() : nullptr;
}

RhiDescriptorAllocation D3D12RenderHardwareInterface::AllocateDescriptor(ERhiDescriptorAllocatorType descriptorType)
{
	RhiDescriptorAllocation allocation{};
	if (m_descriptorHeapManager == nullptr)
	{
		return allocation;
	}

	const D3D12_DESCRIPTOR_HEAP_TYPE nativeType = D3D12TypeConversions::ToDescriptorHeapType(descriptorType);
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{};
	m_descriptorHeapManager->AllocateHandle(nativeType, cpuHandle, gpuHandle);
	allocation.CpuHandle = RhiCpuDescriptorHandle{cpuHandle.ptr};
	allocation.GpuHandle = RhiGpuDescriptorHandle{gpuHandle.ptr};
	return allocation;
}

void D3D12RenderHardwareInterface::ReleaseDescriptor(ERhiDescriptorAllocatorType descriptorType, const RhiDescriptorAllocation& allocation) noexcept
{
	if (m_descriptorHeapManager == nullptr || !allocation.CpuHandle)
	{
		return;
	}

	m_descriptorHeapManager->FreeHandle(
	    D3D12TypeConversions::ToDescriptorHeapType(descriptorType),
	    D3D12_CPU_DESCRIPTOR_HANDLE{allocation.CpuHandle.Value},
	    D3D12_GPU_DESCRIPTOR_HANDLE{allocation.GpuHandle.Value});
}

RhiDescriptorTableHandle D3D12RenderHardwareInterface::AllocateDescriptorTable(
    ERhiDescriptorAllocatorType descriptorType,
    std::uint32_t descriptorCount)
{
	if (m_descriptorHeapManager == nullptr || descriptorCount == 0)
	{
		return {};
	}

	const D3D12DescriptorHandle nativeHandle =
	    m_descriptorHeapManager->AllocateContiguous(D3D12TypeConversions::ToDescriptorHeapType(descriptorType), descriptorCount);
	if (!nativeHandle.IsValid())
	{
		return {};
	}

	DescriptorTableRecord record{};
	record.descriptorType = descriptorType;
	record.descriptorCount = descriptorCount;
	record.nativeHandle = nativeHandle;

	if (!m_freeDescriptorTableIndices.empty())
	{
		const std::uint32_t recordIndex = m_freeDescriptorTableIndices.back();
		m_freeDescriptorTableIndices.pop_back();
		m_descriptorTableRecords[recordIndex] = record;
		return RhiDescriptorTableHandle{recordIndex + 1u};
	}

	m_descriptorTableRecords.push_back(record);
	return RhiDescriptorTableHandle{static_cast<std::uint32_t>(m_descriptorTableRecords.size())};
}

RhiCpuDescriptorHandle D3D12RenderHardwareInterface::GetDescriptorTableCpuHandle(
    RhiDescriptorTableHandle tableHandle,
    std::uint32_t descriptorIndex) const noexcept
{
	return RhiCpuDescriptorHandle{ResolveDescriptorTableCpuHandle(tableHandle, descriptorIndex).ptr};
}

void D3D12RenderHardwareInterface::ReleaseDescriptorTable(RhiDescriptorTableHandle tableHandle) noexcept
{
	DescriptorTableRecord* const record = FindDescriptorTableRecord(tableHandle);
	if (record == nullptr || m_descriptorHeapManager == nullptr || !record->IsAllocated())
	{
		return;
	}

	m_descriptorHeapManager->FreeContiguous(
	    D3D12TypeConversions::ToDescriptorHeapType(record->descriptorType),
	    record->nativeHandle,
	    record->descriptorCount);
	*record = DescriptorTableRecord{};
	m_freeDescriptorTableIndices.push_back(tableHandle.Value - 1u);
}

void D3D12RenderHardwareInterface::AllocateShaderResourceDescriptor(
    RhiCpuDescriptorHandle& outCpuHandle,
    RhiGpuDescriptorHandle& outGpuHandle)
{
	outCpuHandle = {};
	outGpuHandle = {};
	if (m_descriptorHeapManager == nullptr)
	{
		return;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{};
	m_descriptorHeapManager->AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, cpuHandle, gpuHandle);
	outCpuHandle.Value = cpuHandle.ptr;
	outGpuHandle.Value = gpuHandle.ptr;
}

void D3D12RenderHardwareInterface::ReleaseShaderResourceDescriptor(
    RhiCpuDescriptorHandle cpuHandle,
    RhiGpuDescriptorHandle gpuHandle) noexcept
{
	if (m_descriptorHeapManager == nullptr || !cpuHandle)
	{
		return;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE nativeCpuHandle{};
	nativeCpuHandle.ptr = cpuHandle.Value;
	D3D12_GPU_DESCRIPTOR_HANDLE nativeGpuHandle{};
	nativeGpuHandle.ptr = gpuHandle.Value;
	m_descriptorHeapManager->FreeHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, nativeCpuHandle, nativeGpuHandle);
}

const PerFrameConstantBufferData& D3D12RenderHardwareInterface::GetPerFrameConstantData() const noexcept
{
	static const PerFrameConstantBufferData emptyData{};
	return m_constantBufferManager != nullptr ? m_constantBufferManager->GetPerFrameData() : emptyData;
}

RhiGpuVirtualAddress D3D12RenderHardwareInterface::GetPerFrameConstantGpuAddress() const noexcept
{
	return m_constantBufferManager != nullptr ? m_constantBufferManager->GetPerFrameGpuAddress() : 0;
}


RhiGpuVirtualAddress D3D12RenderHardwareInterface::AllocateUniformConstantBuffer(const void* data, std::uint32_t sizeInBytes)
{
	return m_constantBufferManager != nullptr ? m_constantBufferManager->AllocateUniform(data, sizeInBytes) : 0;
}

RhiGpuVirtualAddress D3D12RenderHardwareInterface::AllocatePerViewConstantBuffer(const PerViewConstantBufferData& data)
{
	return m_constantBufferManager != nullptr ? m_constantBufferManager->AllocatePerView(data) : 0;
}

RhiGpuVirtualAddress D3D12RenderHardwareInterface::AllocatePerObjectVertexConstants(const PerObjectVSConstantBufferData& data)
{
	return m_constantBufferManager != nullptr ? m_constantBufferManager->UpdatePerObjectVS(data) : 0;
}

RhiGpuVirtualAddress D3D12RenderHardwareInterface::AllocatePerObjectPixelConstants(const PerObjectPSConstantBufferData& data)
{
	return m_constantBufferManager != nullptr ? m_constantBufferManager->UpdatePerObjectPS(data) : 0;
}

RhiDescriptorTableBinding D3D12RenderHardwareInterface::GetSharedSamplerBinding(const RhiSamplerDesc& samplerDesc) const noexcept
{
	D3D12SamplerLibrary::Slot slot = D3D12SamplerLibrary::Slot::Count;
	if (!m_samplerTableHandle || !D3D12SamplerLibrary::TryGetSlot(samplerDesc, slot))
	{
		return {};
	}

	return RhiDescriptorTableBinding{m_samplerTableHandle, static_cast<std::uint32_t>(slot)};
}

RhiViewport D3D12RenderHardwareInterface::GetBackBufferViewport() const noexcept
{
	return m_swapChain != nullptr ? m_swapChain->GetDefaultViewport() : RhiViewport{};
}

RhiViewport D3D12RenderHardwareInterface::PresentationService::GetBackBufferViewport() const noexcept
{
	return m_owner != nullptr ? m_owner->GetBackBufferViewport() : RhiViewport{};
}

RhiRect D3D12RenderHardwareInterface::GetBackBufferScissorRect() const noexcept
{
	return m_swapChain != nullptr ? m_swapChain->GetDefaultScissorRect() : RhiRect{};
}

RhiRect D3D12RenderHardwareInterface::PresentationService::GetBackBufferScissorRect() const noexcept
{
	return m_owner != nullptr ? m_owner->GetBackBufferScissorRect() : RhiRect{};
}

RhiCpuDescriptorHandle D3D12RenderHardwareInterface::GetBackBufferRenderTargetView() const noexcept
{
	return m_swapChain != nullptr ? RhiCpuDescriptorHandle{m_swapChain->GetCPUHandle().ptr} : RhiCpuDescriptorHandle{};
}

RhiCpuDescriptorHandle D3D12RenderHardwareInterface::PresentationService::GetBackBufferRenderTargetView() const noexcept
{
	return m_owner != nullptr ? m_owner->GetBackBufferRenderTargetView() : RhiCpuDescriptorHandle{};
}

NativeResourceHandle D3D12RenderHardwareInterface::GetBackBufferResource() const noexcept
{
	return NativeResourceHandle{m_swapChain != nullptr ? m_swapChain->GetCurrentResource() : nullptr};
}

NativeResourceHandle D3D12RenderHardwareInterface::PresentationService::GetBackBufferResource() const noexcept
{
	return m_owner != nullptr ? m_owner->GetBackBufferResource() : NativeResourceHandle{};
}

std::unique_ptr<Texture> D3D12RenderHardwareInterface::CreateTexture(RhiTextureUploadDesc textureUpload, std::wstring_view debugName)
{
	(void) debugName;
	if (m_rhi == nullptr || m_descriptorHeapManager == nullptr)
	{
		return {};
	}

	if (!textureUpload.IsValid())
	{
		return {};
	}

	std::unique_ptr<TextureFactory> textureFactory = TextureFactory::Create(*m_rhi, *m_descriptorHeapManager);
	return textureFactory != nullptr ? textureFactory->CreateTexture(std::move(textureUpload)) : std::unique_ptr<Texture>{};
}

RhiOwnedResourceHandle D3D12RenderHardwareInterface::CreateTextureResource(
    const RhiTextureResourceDesc& desc,
    ResourceState initialState,
    RhiMemoryCategory category,
    RhiMemoryResidencyClass residencyClass,
    std::wstring_view debugName)
{
	if (m_memoryAllocator == nullptr || !RhiValidation::ValidateTextureResourceDesc(m_capabilities, desc, "RHI.D3D12.CreateTextureResource"))
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

RhiOwnedResourceHandle D3D12RenderHardwareInterface::CreateBufferResource(
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

bool D3D12RenderHardwareInterface::CreateVertexBuffer(
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

bool D3D12RenderHardwareInterface::CreateStructuredBuffer(
    const void* data,
    std::size_t sizeInBytes,
    std::uint32_t strideInBytes,
    std::wstring_view debugName,
    RhiOwnedResourceHandle& outResource,
    RhiResourceViewHandle& outView)
{
	outResource = {};
	outView = {};
	if (m_rhi == nullptr || m_memoryAllocator == nullptr || data == nullptr || sizeInBytes == 0 || strideInBytes == 0)
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
	outView = CreateResourceView(RhiResourceViewDesc::BufferShaderResource(GetNativeResource(outResource), sizeInBytes, strideInBytes));
	if (!outView)
	{
		ReleaseOwnedResource(outResource);
		outResource = {};
		return false;
	}

	return true;
}

bool D3D12RenderHardwareInterface::CreateIndexBuffer(
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

void D3D12RenderHardwareInterface::ReleaseOwnedResource(RhiOwnedResourceHandle resource) noexcept
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

	DrainCompletedOwnedResourceReleases();
	m_pendingOwnedResourceReleases.push_back(
	    PendingOwnedResourceRelease{.Record = std::move(ownedRecord), .RetireFenceValue = retireFenceValue});
}

void D3D12RenderHardwareInterface::DrainCompletedOwnedResourceReleases() noexcept
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

NativeResourceHandle D3D12RenderHardwareInterface::GetNativeResource(RhiOwnedResourceHandle resource) const noexcept
{
	return NativeResourceHandle{GetD3D12Resource(resource)};
}

RhiGpuVirtualAddress D3D12RenderHardwareInterface::GetResourceGpuVirtualAddress(RhiOwnedResourceHandle resource) const noexcept
{
	ID3D12Resource* const nativeResource = GetD3D12Resource(resource);
	return nativeResource != nullptr ? nativeResource->GetGPUVirtualAddress() : 0;
}

RhiRayTracingAccelerationStructurePrebuildInfo D3D12RenderHardwareInterface::GetBottomLevelAccelerationStructurePrebuildInfo(
    const RhiRayTracingGeometryDesc& geometry) const noexcept
{
	return m_rayTracingServices != nullptr ? m_rayTracingServices->GetBottomLevelAccelerationStructurePrebuildInfo(geometry) :
	                                        RhiRayTracingAccelerationStructurePrebuildInfo{};
}

RhiRayTracingAccelerationStructurePrebuildInfo D3D12RenderHardwareInterface::GetTopLevelAccelerationStructurePrebuildInfo(
    std::uint32_t instanceCount) const noexcept
{
	return m_rayTracingServices != nullptr ? m_rayTracingServices->GetTopLevelAccelerationStructurePrebuildInfo(instanceCount) :
	                                        RhiRayTracingAccelerationStructurePrebuildInfo{};
}

RhiOwnedResourceHandle D3D12RenderHardwareInterface::CreateRayTracingScratchBuffer(std::uint64_t sizeInBytes, std::wstring_view debugName)
{
	return m_rayTracingServices != nullptr ? m_rayTracingServices->CreateScratchBuffer(sizeInBytes, debugName) : RhiOwnedResourceHandle{};
}

RhiOwnedResourceHandle D3D12RenderHardwareInterface::CreateRayTracingAccelerationStructureBuffer(
    std::uint64_t sizeInBytes,
    ERhiRayTracingAccelerationStructureType type,
    std::wstring_view debugName)
{
	return m_rayTracingServices != nullptr ? m_rayTracingServices->CreateAccelerationStructureBuffer(sizeInBytes, type, debugName) :
	                                        RhiOwnedResourceHandle{};
}

RhiOwnedResourceHandle D3D12RenderHardwareInterface::CreateRayTracingInstanceBuffer(
    const RhiRayTracingInstanceDesc* instances,
    std::uint32_t instanceCount,
    std::wstring_view debugName)
{
	return m_rayTracingServices != nullptr ? m_rayTracingServices->CreateInstanceBuffer(instances, instanceCount, debugName) :
	                                        RhiOwnedResourceHandle{};
}

RhiResourceAllocationInfo D3D12RenderHardwareInterface::GetTextureAllocationInfo(const RhiTextureResourceDesc& desc) const noexcept
{
	if (m_rhi == nullptr)
	{
		return {};
	}

	const D3D12_RESOURCE_DESC resourceDesc = D3D12TypeConversions::BuildTextureResourceDesc(desc);
	const D3D12_RESOURCE_ALLOCATION_INFO allocationInfo = m_rhi->GetDevice()->GetResourceAllocationInfo(0, 1, &resourceDesc);
	return RhiResourceAllocationInfo{.SizeInBytes = allocationInfo.SizeInBytes, .Alignment = allocationInfo.Alignment};
}

RhiResourceAllocationInfo D3D12RenderHardwareInterface::GetBufferAllocationInfo(const RhiBufferResourceDesc& desc) const noexcept
{
	if (m_rhi == nullptr)
	{
		return {};
	}

	const D3D12_RESOURCE_DESC resourceDesc = D3D12TypeConversions::BuildBufferResourceDesc(desc);
	const D3D12_RESOURCE_ALLOCATION_INFO allocationInfo = m_rhi->GetDevice()->GetResourceAllocationInfo(0, 1, &resourceDesc);
	return RhiResourceAllocationInfo{.SizeInBytes = allocationInfo.SizeInBytes, .Alignment = allocationInfo.Alignment};
}

RhiOwnedMemoryBlockHandle D3D12RenderHardwareInterface::CreateTransientMemoryBlock(
    RhiTransientAllocationPool pool,
    std::uint64_t sizeInBytes,
    std::uint64_t alignment,
    std::wstring_view debugName)
{
	if (m_rhi == nullptr || sizeInBytes == 0)
	{
		return {};
	}

	std::wstring ownedDebugName = CopyDebugName(debugName, L"TransientMemoryBlock");
	std::unique_ptr<D3D12GpuHeapRecord> ownedMemoryBlock =
	    m_memoryAllocator != nullptr ? m_memoryAllocator->CreateTransientHeap(pool, sizeInBytes, alignment, ownedDebugName) : nullptr;
	if (ownedMemoryBlock == nullptr)
	{
		return {};
	}

	return WrapOwnedMemoryBlock(std::move(ownedMemoryBlock));
}

void D3D12RenderHardwareInterface::ReleaseTransientMemoryBlock(RhiOwnedMemoryBlockHandle memoryBlock) noexcept
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

	DrainCompletedOwnedResourceReleases();
	m_pendingOwnedMemoryBlockReleases.push_back(
	    PendingOwnedMemoryBlockRelease{.Record = std::move(ownedMemoryBlock), .RetireFenceValue = retireFenceValue});
}

RhiOwnedResourceHandle D3D12RenderHardwareInterface::CreateAliasingTextureResource(
    RhiOwnedMemoryBlockHandle memoryBlock,
    std::uint64_t memoryBlockOffset,
    const RhiTransientTextureAllocationDesc& desc,
    std::wstring_view debugName)
{
	D3D12GpuHeapRecord* const ownedMemoryBlock = GetD3D12GpuHeapRecord(memoryBlock);
	if (m_rhi == nullptr || m_memoryAllocator == nullptr || ownedMemoryBlock == nullptr ||
	    !RhiValidation::ValidateTextureResourceDesc(m_capabilities, desc.ResourceDesc, "RHI.D3D12.CreateAliasingTextureResource"))
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
	if (ownedResource == nullptr)
	{
		return {};
	}

	return WrapOwnedResource(std::move(ownedResource));
}

RhiOwnedResourceHandle D3D12RenderHardwareInterface::CreateAliasingBufferResource(
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
	if (ownedResource == nullptr)
	{
		return {};
	}

	return WrapOwnedResource(std::move(ownedResource));
}

RhiResourceViewHandle D3D12RenderHardwareInterface::CreateResourceView(const RhiResourceViewDesc& desc)
{
	if (m_rhi == nullptr || m_descriptorHeapManager == nullptr)
	{
		return {};
	}

	const ERhiDescriptorAllocatorType descriptorType = ResolveResourceViewDescriptorAllocatorType(desc.Kind);
	RhiDescriptorAllocation allocation = AllocateDescriptor(descriptorType);
	if (!allocation.IsValid())
	{
		return {};
	}

	if (!WriteD3D12ResourceViewDescriptor(desc, allocation.CpuHandle))
	{
		ReleaseDescriptor(descriptorType, allocation);
		return {};
	}

	ResourceViewRecord record{};
	record.kind = desc.Kind;
	record.descriptorType = descriptorType;
	record.descriptorAllocation = allocation;

	if (!m_freeResourceViewIndices.empty())
	{
		const std::uint32_t recordIndex = m_freeResourceViewIndices.back();
		m_freeResourceViewIndices.pop_back();
		m_resourceViewRecords[recordIndex] = record;
		return RhiResourceViewHandle{recordIndex + 1u};
	}

	m_resourceViewRecords.push_back(record);
	return RhiResourceViewHandle{static_cast<std::uint32_t>(m_resourceViewRecords.size())};
}

void D3D12RenderHardwareInterface::ReleaseResourceView(RhiResourceViewHandle view) noexcept
{
	ResourceViewRecord* const record = FindResourceViewRecord(view);
	if (record == nullptr)
	{
		return;
	}

	ReleaseDescriptor(record->descriptorType, record->descriptorAllocation);
	*record = ResourceViewRecord{};
	m_freeResourceViewIndices.push_back(view.Value - 1u);
}

RhiCpuDescriptorHandle D3D12RenderHardwareInterface::GetResourceViewCpuHandle(RhiResourceViewHandle view) const noexcept
{
	const ResourceViewRecord* const record = FindResourceViewRecord(view);
	return record != nullptr ? record->descriptorAllocation.CpuHandle : RhiCpuDescriptorHandle{};
}

RhiGpuDescriptorHandle D3D12RenderHardwareInterface::GetResourceViewGpuHandle(RhiResourceViewHandle view) const noexcept
{
	const ResourceViewRecord* const record = FindResourceViewRecord(view);
	return record != nullptr ? record->descriptorAllocation.GpuHandle : RhiGpuDescriptorHandle{};
}

NativeTextureViewInfo D3D12RenderHardwareInterface::GetNativeTextureViewInfo(RhiResourceViewHandle, ResourceState) const noexcept
{
	return {};
}

std::uint64_t D3D12RenderHardwareInterface::ResolveImGuiTextureId(RhiGpuDescriptorHandle shaderResourceView) noexcept
{
	return shaderResourceView.Value;
}

std::uint64_t D3D12RenderHardwareInterface::PresentationService::ResolveImGuiTextureId(
    RhiGpuDescriptorHandle shaderResourceView) noexcept
{
	return m_owner != nullptr ? m_owner->ResolveImGuiTextureId(shaderResourceView) : 0;
}

bool D3D12RenderHardwareInterface::WriteD3D12ResourceViewDescriptor(
    const RhiResourceViewDesc& desc,
    RhiCpuDescriptorHandle destination) noexcept
{
	if (m_rhi == nullptr || !destination)
	{
		return false;
	}

	ID3D12Device* const device = m_rhi->GetDevice().Get();
	if (device == nullptr)
	{
		return false;
	}

	const D3D12_CPU_DESCRIPTOR_HANDLE nativeDestination = D3D12TypeConversions::ToCpuDescriptor(destination);
	switch (desc.Kind)
	{
		case ERhiResourceViewKind::RenderTarget:
		{
			if (!desc.Resource)
			{
				return false;
			}

			D3D12_RENDER_TARGET_VIEW_DESC viewDesc{};
			viewDesc.Format = D3D12TypeConversions::ToDxgiFormat(desc.Format);
			viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			device->CreateRenderTargetView(D3D12TypeConversions::ToResource(desc.Resource), &viewDesc, nativeDestination);
			return true;
		}
		case ERhiResourceViewKind::DepthStencil:
		{
			if (!desc.Resource)
			{
				return false;
			}

			D3D12_DEPTH_STENCIL_VIEW_DESC viewDesc{};
			viewDesc.Format = D3D12TypeConversions::ToDxgiFormat(desc.Format);
			viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			viewDesc.Flags = D3D12_DSV_FLAG_NONE;
			device->CreateDepthStencilView(D3D12TypeConversions::ToResource(desc.Resource), &viewDesc, nativeDestination);
			return true;
		}
		case ERhiResourceViewKind::TextureShaderResource:
		{
			if (!desc.Resource)
			{
				return false;
			}

			D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc{};
			viewDesc.Format = D3D12TypeConversions::ToDxgiFormat(desc.Format);
			viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			viewDesc.Texture2D.MostDetailedMip = desc.Texture.MostDetailedMip;
			viewDesc.Texture2D.MipLevels = desc.Texture.MipCount;
			device->CreateShaderResourceView(D3D12TypeConversions::ToResource(desc.Resource), &viewDesc, nativeDestination);
			return true;
		}
		case ERhiResourceViewKind::TextureUnorderedAccess:
		{
			if (!desc.Resource)
			{
				return false;
			}

			D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc{};
			viewDesc.Format = D3D12TypeConversions::ToDxgiFormat(desc.Format);
			viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			viewDesc.Texture2D.MipSlice = desc.Texture.MostDetailedMip;
			viewDesc.Texture2D.PlaneSlice = 0;
			device->CreateUnorderedAccessView(D3D12TypeConversions::ToResource(desc.Resource), nullptr, &viewDesc, nativeDestination);
			return true;
		}
		case ERhiResourceViewKind::BufferShaderResource:
		{
			if (!desc.Resource || desc.Buffer.SizeInBytes == 0)
			{
				return false;
			}

			D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc{};
			viewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			if (desc.Buffer.StrideInBytes > 0)
			{
				viewDesc.Format = DXGI_FORMAT_UNKNOWN;
				viewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
				viewDesc.Buffer.FirstElement = desc.Buffer.OffsetInBytes / desc.Buffer.StrideInBytes;
				viewDesc.Buffer.StructureByteStride = desc.Buffer.StrideInBytes;
				viewDesc.Buffer.NumElements = static_cast<UINT>(desc.Buffer.SizeInBytes / desc.Buffer.StrideInBytes);
			}
			else
			{
				viewDesc.Format = DXGI_FORMAT_R32_TYPELESS;
				viewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
				viewDesc.Buffer.FirstElement = desc.Buffer.OffsetInBytes / sizeof(std::uint32_t);
				viewDesc.Buffer.StructureByteStride = 0;
				viewDesc.Buffer.NumElements = static_cast<UINT>(desc.Buffer.SizeInBytes / sizeof(std::uint32_t));
			}
			device->CreateShaderResourceView(D3D12TypeConversions::ToResource(desc.Resource), &viewDesc, nativeDestination);
			return true;
		}
		case ERhiResourceViewKind::BufferUnorderedAccess:
		{
			if (!desc.Resource || desc.Buffer.SizeInBytes == 0)
			{
				return false;
			}

			D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc{};
			viewDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			if (desc.Buffer.StrideInBytes > 0)
			{
				viewDesc.Format = DXGI_FORMAT_UNKNOWN;
				viewDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
				viewDesc.Buffer.FirstElement = desc.Buffer.OffsetInBytes / desc.Buffer.StrideInBytes;
				viewDesc.Buffer.StructureByteStride = desc.Buffer.StrideInBytes;
				viewDesc.Buffer.NumElements = static_cast<UINT>(desc.Buffer.SizeInBytes / desc.Buffer.StrideInBytes);
			}
			else
			{
				viewDesc.Format = DXGI_FORMAT_R32_TYPELESS;
				viewDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
				viewDesc.Buffer.FirstElement = desc.Buffer.OffsetInBytes / sizeof(std::uint32_t);
				viewDesc.Buffer.StructureByteStride = 0;
				viewDesc.Buffer.NumElements = static_cast<UINT>(desc.Buffer.SizeInBytes / sizeof(std::uint32_t));
			}
			device->CreateUnorderedAccessView(D3D12TypeConversions::ToResource(desc.Resource), nullptr, &viewDesc, nativeDestination);
			return true;
		}
		case ERhiResourceViewKind::AccelerationStructureShaderResource:
		{
			if (desc.AccelerationStructureGpuAddress == 0)
			{
				return false;
			}

			D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc{};
			viewDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
			viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			viewDesc.RaytracingAccelerationStructure.Location = desc.AccelerationStructureGpuAddress;
			device->CreateShaderResourceView(nullptr, &viewDesc, nativeDestination);
			return true;
		}
		default:
			return false;
	}
}

bool D3D12RenderHardwareInterface::SupportsUnorderedAccess(NativeResourceHandle resource) const noexcept
{
	return ResourceSupportsUnorderedAccess(D3D12TypeConversions::ToResource(resource));
}

void D3D12RenderHardwareInterface::BeginPresentRenderPass(const float clearColor[4]) noexcept
{
	if (m_swapChain == nullptr)
	{
		return;
	}

	NativeResourceHandle presentTexture{m_swapChain->GetCurrentResource()};
	if (!presentTexture)
	{
		return;
	}

	RenderCommandList& commandList = GetGraphicsCommandList(GetCurrentFrameIndex());
	commandList.TransitionResource(presentTexture, ResourceState::Present, ResourceState::RenderTarget);
	BindGlobalDescriptorState(commandList);

	const RhiCpuDescriptorHandle renderTargetView = GetBackBufferRenderTargetView();
	commandList.SetRenderTarget(renderTargetView);

	static constexpr float defaultClearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	commandList.ClearRenderTarget(renderTargetView, clearColor != nullptr ? clearColor : defaultClearColor);
}

void D3D12RenderHardwareInterface::BeginPresentOverlayPass() noexcept
{
	if (m_swapChain == nullptr)
	{
		return;
	}

	NativeResourceHandle presentTexture{m_swapChain->GetCurrentResource()};
	if (!presentTexture)
	{
		return;
	}

	RenderCommandList& commandList = GetGraphicsCommandList(GetCurrentFrameIndex());
	commandList.TransitionResource(presentTexture, ResourceState::Present, ResourceState::RenderTarget);
	BindGlobalDescriptorState(commandList);

	const RhiCpuDescriptorHandle renderTargetView = GetBackBufferRenderTargetView();
	commandList.SetRenderTarget(renderTargetView);
}

void D3D12RenderHardwareInterface::PresentationService::BeginPresentRenderPass(const float clearColor[4]) noexcept
{
	if (m_owner != nullptr)
	{
		m_owner->BeginPresentRenderPass(clearColor);
	}
}

void D3D12RenderHardwareInterface::PresentationService::BeginPresentOverlayPass() noexcept
{
	if (m_owner != nullptr)
	{
		m_owner->BeginPresentOverlayPass();
	}
}

void D3D12RenderHardwareInterface::EndPresentRenderPass() noexcept
{
	if (m_swapChain == nullptr)
	{
		return;
	}

	NativeResourceHandle presentTexture{m_swapChain->GetCurrentResource()};
	if (!presentTexture)
	{
		return;
	}

	RenderCommandList& commandList = GetGraphicsCommandList(GetCurrentFrameIndex());
	commandList.TransitionResource(presentTexture, ResourceState::RenderTarget, ResourceState::Present);
}

void D3D12RenderHardwareInterface::PresentationService::EndPresentRenderPass() noexcept
{
	if (m_owner != nullptr)
	{
		m_owner->EndPresentRenderPass();
	}
}

PixelFormat D3D12RenderHardwareInterface::GetPresentColorFormat() const noexcept
{
	return m_swapChain != nullptr ? m_swapChain->GetBackBufferFormat() : PixelFormat::Unknown;
}

PixelFormat D3D12RenderHardwareInterface::PresentationService::GetPresentColorFormat() const noexcept
{
	return m_owner != nullptr ? m_owner->GetPresentColorFormat() : PixelFormat::Unknown;
}

void D3D12RenderHardwareInterface::SetSamplerTableHandle(RhiDescriptorTableHandle samplerTableHandle) noexcept
{
	m_samplerTableHandle = samplerTableHandle;
}

ERhiDescriptorAllocatorType D3D12RenderHardwareInterface::ResolveResourceViewDescriptorAllocatorType(
    ERhiResourceViewKind kind) noexcept
{
	switch (kind)
	{
		case ERhiResourceViewKind::RenderTarget:
			return ERhiDescriptorAllocatorType::RenderTarget;
		case ERhiResourceViewKind::DepthStencil:
			return ERhiDescriptorAllocatorType::DepthStencil;
		case ERhiResourceViewKind::TextureShaderResource:
		case ERhiResourceViewKind::TextureUnorderedAccess:
		case ERhiResourceViewKind::BufferShaderResource:
		case ERhiResourceViewKind::BufferUnorderedAccess:
		case ERhiResourceViewKind::AccelerationStructureShaderResource:
		default:
			return ERhiDescriptorAllocatorType::ShaderResource;
	}
}

D3D12RenderHardwareInterface::DescriptorTableRecord* D3D12RenderHardwareInterface::FindDescriptorTableRecord(
    RhiDescriptorTableHandle tableHandle) noexcept
{
	if (!tableHandle || tableHandle.Value == 0 || tableHandle.Value > m_descriptorTableRecords.size())
	{
		return nullptr;
	}

	DescriptorTableRecord& record = m_descriptorTableRecords[tableHandle.Value - 1u];
	return record.IsAllocated() ? &record : nullptr;
}

const D3D12RenderHardwareInterface::DescriptorTableRecord* D3D12RenderHardwareInterface::FindDescriptorTableRecord(
    RhiDescriptorTableHandle tableHandle) const noexcept
{
	if (!tableHandle || tableHandle.Value == 0 || tableHandle.Value > m_descriptorTableRecords.size())
	{
		return nullptr;
	}

	const DescriptorTableRecord& record = m_descriptorTableRecords[tableHandle.Value - 1u];
	return record.IsAllocated() ? &record : nullptr;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12RenderHardwareInterface::ResolveDescriptorTableCpuHandle(
    RhiDescriptorTableHandle tableHandle,
    std::uint32_t descriptorIndex) const noexcept
{
	const DescriptorTableRecord* const record = FindDescriptorTableRecord(tableHandle);
	if (record == nullptr || descriptorIndex >= record->descriptorCount)
	{
		return D3D12_CPU_DESCRIPTOR_HANDLE{};
	}

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = record->nativeHandle.GetCPU();
	cpuHandle.ptr += static_cast<SIZE_T>(descriptorIndex) * record->nativeHandle.GetIncrementSize();
	return cpuHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12RenderHardwareInterface::ResolveDescriptorTableGpuHandle(
    RhiDescriptorTableHandle tableHandle,
    std::uint32_t descriptorIndex) const noexcept
{
	const DescriptorTableRecord* const record = FindDescriptorTableRecord(tableHandle);
	if (record == nullptr || descriptorIndex >= record->descriptorCount)
	{
		return D3D12_GPU_DESCRIPTOR_HANDLE{};
	}

	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = record->nativeHandle.GetGPU();
	gpuHandle.ptr += static_cast<UINT64>(descriptorIndex) * record->nativeHandle.GetIncrementSize();
	return gpuHandle;
}

D3D12RenderHardwareInterface::ResourceViewRecord* D3D12RenderHardwareInterface::FindResourceViewRecord(
    RhiResourceViewHandle view) noexcept
{
	if (!view || view.Value == 0 || view.Value > m_resourceViewRecords.size())
	{
		return nullptr;
	}

	ResourceViewRecord& record = m_resourceViewRecords[view.Value - 1u];
	return record.IsAllocated() ? &record : nullptr;
}

const D3D12RenderHardwareInterface::ResourceViewRecord* D3D12RenderHardwareInterface::FindResourceViewRecord(
    RhiResourceViewHandle view) const noexcept
{
	if (!view || view.Value == 0 || view.Value > m_resourceViewRecords.size())
	{
		return nullptr;
	}

	const ResourceViewRecord& record = m_resourceViewRecords[view.Value - 1u];
	return record.IsAllocated() ? &record : nullptr;
}

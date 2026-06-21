#include "PCH.h"

#include "D3D12/D3D12RenderHardwareInterface.h"

#include "D3D12/Commands/D3D12RenderCommandList.h"
#include "D3D12/Capture/D3D12CaptureService.h"
#include "D3D12/Device/D3D12ExternalFeatureInteropCapabilities.h"
#include "D3D12/Device/D3D12Rhi.h"
#include "D3D12/SwapChain/D3D12SwapChain.h"
#include "D3D12/D3D12TypeConversions.h"
#include "D3D12/Diagnostics/D3D12PixEvents.h"
#include "D3D12/Diagnostics/D3D12RenderDiagnostics.h"
#include "D3D12/Descriptors/D3D12DescriptorHeap.h"
#include "D3D12/Descriptors/D3D12DescriptorHeapManager.h"
#include "D3D12/Descriptors/D3D12DescriptorService.h"
#include "D3D12/Interop/D3D12InteropService.h"
#include "D3D12/Memory/D3D12GpuMemoryAllocator.h"
#include "D3D12/Pipeline/D3D12PipelineService.h"
#include "D3D12/Presentation/D3D12PresentationService.h"
#include "D3D12/RayTracing/D3D12RayTracingServices.h"
#include "D3D12/Resources/D3D12ConstantBufferManager.h"
#include "D3D12/Resources/D3D12ResourceService.h"
#include "D3D12/Samplers/D3D12SamplerLibrary.h"
#include "D3D12/UI/D3D12ImGuiBackend.h"
#include "Resources/Texture.h"
#include "Shaders/CookedShaderPackage.h"

#include <algorithm>
#include <d3d12.h>
#include <wrl/client.h>
#include <fstream>
#include <string>
#include <vector>

namespace
{
	RhiBackendDiagnosticsSupport BuildBackendDiagnosticsSupport(
	    const RenderDiagnostics* diagnostics,
	    bool validationEnabled,
	    bool supportsDebugLayer,
	    bool supportsCapture) noexcept
	{
		if (diagnostics == nullptr)
		{
			return RhiBackendDiagnosticsSupport{
			    .ValidationEnabled = validationEnabled,
			    .SupportsDebugLayer = supportsDebugLayer,
			    .SupportsCapture = supportsCapture};
		}

		const RhiDiagnosticsCapabilities diagnosticsCapabilities = diagnostics->GetCapabilities();
		return RhiBackendDiagnosticsSupport{
		    .ValidationEnabled = validationEnabled,
		    .SupportsDebugLayer = supportsDebugLayer,
		    .SupportsObjectNames = diagnosticsCapabilities.SupportsObjectNames,
		    .SupportsGpuEvents = diagnosticsCapabilities.SupportsGpuEvents,
		    .SupportsTimestampQueries = diagnosticsCapabilities.SupportsTimestampQueries,
		    .SupportsDebugMessages = diagnosticsCapabilities.SupportsDebugMessages,
		    .SupportsLiveObjectReports = diagnosticsCapabilities.SupportsLiveObjectReports,
		    .SupportsCrashDiagnostics = diagnosticsCapabilities.SupportsCrashDiagnostics,
		    .SupportsCapture = supportsCapture};
	}

	RhiBackendMemorySupport BuildBackendMemorySupport(const RenderDiagnostics* diagnostics) noexcept
	{
		if (diagnostics == nullptr)
		{
			return {};
		}

		const RenderMemoryDiagnostics* const memoryDiagnostics = diagnostics->GetMemoryDiagnostics();
		return RhiBackendMemorySupport{
		    .SupportsMemoryDiagnostics = memoryDiagnostics != nullptr,
		    .SupportsBudgetQueries = memoryDiagnostics != nullptr && memoryDiagnostics->SupportsBudgetQueries(),
		    .SupportsJsonDump = memoryDiagnostics != nullptr && memoryDiagnostics->SupportsJsonDump(),
		    .SupportsAllocationDetails = memoryDiagnostics != nullptr && memoryDiagnostics->SupportsAllocationDetails(),
		    .SupportsDelayedDestructionTracking = memoryDiagnostics != nullptr && memoryDiagnostics->SupportsDelayedDestructionTracking(),
		    .SupportsResidencyPressure = memoryDiagnostics != nullptr && memoryDiagnostics->SupportsBudgetQueries()};
	}

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
	m_rhi(&rhi), m_descriptorHeapManager(&descriptorHeapManager), m_swapChain(&swapChain), m_constantBufferManager(&constantBufferManager)
{
	m_interopService = std::make_unique<D3D12InteropService>(*this);
	m_captureService = std::make_unique<D3D12CaptureService>(*this);
	m_presentationService = std::make_unique<D3D12PresentationService>(*this);
	m_pipelineService = std::make_unique<D3D12PipelineService>(rhi);
	m_descriptorService = std::make_unique<D3D12DescriptorService>(rhi, descriptorHeapManager, m_capabilities);
	m_resourceService =
	    std::make_unique<D3D12ResourceService>(rhi, memoryAllocator, descriptorHeapManager, *m_descriptorService, m_capabilities);
	m_rayTracingServices = std::make_unique<D3D12RayTracingServices>(rhi, memoryAllocator, rhi.GetNvapiRayTracingProvider());
	for (std::uint32_t frameIndex = 0; frameIndex < RhiFrameConstants::FramesInFlight; ++frameIndex)
	{
		m_commandLists[frameIndex] = std::make_unique<D3D12RenderCommandList>(*this, rhi.GetCommandList(frameIndex).Get());
	}

	m_diagnostics = CreateD3D12RenderDiagnostics(rhi);
	m_capabilities = BuildCapabilities();
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

RhiCapabilities D3D12RenderHardwareInterface::BuildCapabilities() const noexcept
{
	RhiCapabilities capabilities{};
	const D3D_FEATURE_LEVEL featureLevel = m_rhi != nullptr ? m_rhi->GetDeviceFeatureLevel() : D3D_FEATURE_LEVEL_1_0_CORE;
	const std::uint32_t featureLevelMajor = featureLevel >= D3D_FEATURE_LEVEL_12_0 ? 12u : featureLevel >= D3D_FEATURE_LEVEL_11_0 ? 11u : 0u;
	const std::uint32_t featureLevelMinor = featureLevel == D3D_FEATURE_LEVEL_12_2 ? 2u
	                                     : featureLevel == D3D_FEATURE_LEVEL_12_1 ? 1u
	                                     : featureLevel == D3D_FEATURE_LEVEL_12_0 ? 0u
	                                     : featureLevel == D3D_FEATURE_LEVEL_11_1 ? 1u
	                                     : featureLevel == D3D_FEATURE_LEVEL_11_0 ? 0u
	                                                                               : 0u;
	capabilities.BackendApi = ERhiBackendApi::D3D12;
	capabilities.RequiredShaderBinaryFormat = CookedShaderBinaryFormat::Dxil;
	capabilities.BackendVersion = RhiBackendVersionInfo{
	    .Semantic = ERhiBackendVersionSemantic::FeatureLevel,
	    .Major = featureLevelMajor,
	    .Minor = featureLevelMinor,
	    .Patch = 0,
	    .PackedValue = static_cast<std::uint32_t>(featureLevel)};
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
	capabilities.Diagnostics = BuildBackendDiagnosticsSupport(
	    m_diagnostics.get(),
	    m_rhi != nullptr && m_rhi->IsValidationEnabled(),
	    m_rhi != nullptr && m_rhi->IsValidationEnabled(),
	    m_captureService != nullptr);
	capabilities.SupportsTimestampQueries = capabilities.Diagnostics.SupportsTimestampQueries;
	capabilities.RayTracing = m_rhi != nullptr ? m_rhi->GetRayTracingCapabilities() : RhiRayTracingCapabilities{};
	capabilities.SupportsMeshShaders = false;
	capabilities.SupportsTaskShaders = false;
	capabilities.Queues = RhiQueueCapabilities{.SupportsGraphics = true, .SupportsCompute = false, .SupportsCopy = false};
	capabilities.SupportsPresent = m_swapChain != nullptr && m_swapChain->GetBackBufferFormat() != PixelFormat::Unknown;
	capabilities.MemoryAllocator = ERhiMemoryAllocatorBackend::D3D12Managed;
	capabilities.MemorySupport = BuildBackendMemorySupport(m_diagnostics.get());
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

RhiResourceService& D3D12RenderHardwareInterface::GetResourceService() noexcept
{
	return *m_resourceService;
}

const RhiResourceService& D3D12RenderHardwareInterface::GetResourceService() const noexcept
{
	return *m_resourceService;
}

RhiDescriptorService& D3D12RenderHardwareInterface::GetDescriptorService() noexcept
{
	return *m_descriptorService;
}

const RhiDescriptorService& D3D12RenderHardwareInterface::GetDescriptorService() const noexcept
{
	return *m_descriptorService;
}

RhiPipelineService& D3D12RenderHardwareInterface::GetPipelineService() noexcept
{
	return *m_pipelineService;
}

RhiUploadService& D3D12RenderHardwareInterface::GetUploadService() noexcept
{
	return *m_constantBufferManager;
}

const RhiUploadService& D3D12RenderHardwareInterface::GetUploadService() const noexcept
{
	return *m_constantBufferManager;
}

RhiRayTracingService& D3D12RenderHardwareInterface::GetRayTracingService() noexcept
{
	return *m_rayTracingServices;
}

const RhiRayTracingService& D3D12RenderHardwareInterface::GetRayTracingService() const noexcept
{
	return *m_rayTracingServices;
}

void D3D12RenderHardwareInterface::WaitForIdle() noexcept
{
	if (m_rhi != nullptr)
	{
		m_rhi->Flush();
		if (m_resourceService != nullptr)
		{
			m_resourceService->FlushDeferredResourceReleases();
		}
	}
}

RhiInteropService& D3D12RenderHardwareInterface::GetInteropService() noexcept
{
	return *m_interopService;
}

const RhiInteropService& D3D12RenderHardwareInterface::GetInteropService() const noexcept
{
	return *m_interopService;
}

RhiCaptureService& D3D12RenderHardwareInterface::GetCaptureService() noexcept
{
	return *m_captureService;
}

RhiPresentationService& D3D12RenderHardwareInterface::GetPresentationService() noexcept
{
	return *m_presentationService;
}

const RhiPresentationService& D3D12RenderHardwareInterface::GetPresentationService() const noexcept
{
	return *m_presentationService;
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
	if (m_resourceService != nullptr)
	{
		m_resourceService->DrainCompletedResourceReleases();
	}
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

RhiImGuiRenderer& D3D12RenderHardwareInterface::GetImGuiRenderer() noexcept
{
	return *m_imguiBackend;
}

std::unique_ptr<RenderBindingSet> D3D12RenderHardwareInterface::CreateBindingSet(const RenderBindingSetDesc& desc)
{
	return m_descriptorService != nullptr ? m_descriptorService->CreateBindingSet(desc) : std::unique_ptr<RenderBindingSet>{};
}

std::unique_ptr<RenderBindingLayout> D3D12RenderHardwareInterface::CreateBindingLayout(const RenderBindingLayoutCompileDesc& desc)
{
	return m_pipelineService != nullptr ? m_pipelineService->CreateBindingLayout(desc) : std::unique_ptr<RenderBindingLayout>{};
}

std::unique_ptr<RenderPipelineState> D3D12RenderHardwareInterface::CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc)
{
	return m_pipelineService != nullptr ? m_pipelineService->CreateGraphicsPipelineState(desc) : std::unique_ptr<RenderPipelineState>{};
}

std::unique_ptr<RenderPipelineState> D3D12RenderHardwareInterface::CreateComputePipelineState(const ComputePipelineStateDesc& desc)
{
	return m_pipelineService != nullptr ? m_pipelineService->CreateComputePipelineState(desc) : std::unique_ptr<RenderPipelineState>{};
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
	return m_descriptorService != nullptr ? m_descriptorService->GetShaderResourceDescriptorHeap() : nullptr;
}

RhiDescriptorAllocation D3D12RenderHardwareInterface::AllocateDescriptor(ERhiDescriptorAllocatorType descriptorType)
{
	return m_descriptorService != nullptr ? m_descriptorService->AllocateDescriptor(descriptorType) : RhiDescriptorAllocation{};
}

void D3D12RenderHardwareInterface::ReleaseDescriptor(ERhiDescriptorAllocatorType descriptorType, const RhiDescriptorAllocation& allocation) noexcept
{
	if (m_descriptorService != nullptr)
	{
		m_descriptorService->ReleaseDescriptor(descriptorType, allocation);
	}
}

RhiDescriptorTableHandle D3D12RenderHardwareInterface::AllocateDescriptorTable(
    ERhiDescriptorAllocatorType descriptorType,
    std::uint32_t descriptorCount)
{
	return m_descriptorService != nullptr ? m_descriptorService->AllocateDescriptorTable(descriptorType, descriptorCount)
	                                      : RhiDescriptorTableHandle{};
}

RhiCpuDescriptorHandle D3D12RenderHardwareInterface::GetDescriptorTableCpuHandle(
    RhiDescriptorTableHandle tableHandle,
    std::uint32_t descriptorIndex) const noexcept
{
	return m_descriptorService != nullptr ? m_descriptorService->GetDescriptorTableCpuHandle(tableHandle, descriptorIndex)
	                                      : RhiCpuDescriptorHandle{};
}

void D3D12RenderHardwareInterface::ReleaseDescriptorTable(RhiDescriptorTableHandle tableHandle) noexcept
{
	if (m_descriptorService != nullptr)
	{
		m_descriptorService->ReleaseDescriptorTable(tableHandle);
	}
}

void D3D12RenderHardwareInterface::AllocateShaderResourceDescriptor(
    RhiCpuDescriptorHandle& outCpuHandle,
    RhiGpuDescriptorHandle& outGpuHandle)
{
	if (m_descriptorService != nullptr)
	{
		m_descriptorService->AllocateShaderResourceDescriptor(outCpuHandle, outGpuHandle);
	}
	else
	{
		outCpuHandle = {};
		outGpuHandle = {};
	}
}

void D3D12RenderHardwareInterface::ReleaseShaderResourceDescriptor(
    RhiCpuDescriptorHandle cpuHandle,
    RhiGpuDescriptorHandle gpuHandle) noexcept
{
	if (m_descriptorService != nullptr)
	{
		m_descriptorService->ReleaseShaderResourceDescriptor(cpuHandle, gpuHandle);
	}
}

RhiGpuVirtualAddress D3D12RenderHardwareInterface::AllocateUniformConstantBuffer(const void* data, std::uint32_t sizeInBytes)
{
	return m_constantBufferManager != nullptr ? m_constantBufferManager->AllocateUniform(data, sizeInBytes) : 0;
}

RhiDescriptorTableBinding D3D12RenderHardwareInterface::GetSharedSamplerBinding(const RhiSamplerDesc& samplerDesc) const noexcept
{
	return m_descriptorService != nullptr ? m_descriptorService->GetSharedSamplerBinding(samplerDesc)
	                                      : RhiDescriptorTableBinding{};
}

RhiViewport D3D12RenderHardwareInterface::GetBackBufferViewport() const noexcept
{
	return m_swapChain != nullptr ? m_swapChain->GetDefaultViewport() : RhiViewport{};
}

RhiRect D3D12RenderHardwareInterface::GetBackBufferScissorRect() const noexcept
{
	return m_swapChain != nullptr ? m_swapChain->GetDefaultScissorRect() : RhiRect{};
}

RhiCpuDescriptorHandle D3D12RenderHardwareInterface::GetBackBufferRenderTargetView() const noexcept
{
	return m_swapChain != nullptr ? RhiCpuDescriptorHandle{m_swapChain->GetCPUHandle().ptr} : RhiCpuDescriptorHandle{};
}

NativeResourceHandle D3D12RenderHardwareInterface::GetBackBufferResource() const noexcept
{
	return NativeResourceHandle{m_swapChain != nullptr ? m_swapChain->GetCurrentResource() : nullptr};
}

std::unique_ptr<Texture> D3D12RenderHardwareInterface::CreateTexture(RhiTextureUploadDesc textureUpload, std::wstring_view debugName)
{
	return m_resourceService != nullptr ? m_resourceService->CreateTexture(std::move(textureUpload), debugName) :
	                                      std::unique_ptr<Texture>{};
}

RhiOwnedResourceHandle D3D12RenderHardwareInterface::CreateTextureResource(
    const RhiTextureResourceDesc& desc,
    ResourceState initialState,
    RhiMemoryCategory category,
    RhiMemoryResidencyClass residencyClass,
    std::wstring_view debugName)
{
	return m_resourceService != nullptr ? m_resourceService->CreateTextureResource(desc, initialState, category, residencyClass, debugName) :
	                                      RhiOwnedResourceHandle{};
}

RhiOwnedResourceHandle D3D12RenderHardwareInterface::CreateBufferResource(
    const RhiBufferResourceDesc& desc,
    ResourceState initialState,
    RhiMemoryCategory category,
    RhiMemoryResidencyClass residencyClass,
    std::wstring_view debugName)
{
	return m_resourceService != nullptr ? m_resourceService->CreateBufferResource(desc, initialState, category, residencyClass, debugName) :
	                                      RhiOwnedResourceHandle{};
}

bool D3D12RenderHardwareInterface::CreateVertexBuffer(
    const void* data,
    std::size_t sizeInBytes,
    std::uint32_t strideInBytes,
    std::wstring_view debugName,
    RhiOwnedResourceHandle& outResource,
    RhiVertexBufferView& outView)
{
	return m_resourceService != nullptr ?
	           m_resourceService->CreateVertexBuffer(data, sizeInBytes, strideInBytes, debugName, outResource, outView) :
	           false;
}

bool D3D12RenderHardwareInterface::CreateStructuredBuffer(
    const void* data,
    std::size_t sizeInBytes,
    std::uint32_t strideInBytes,
    std::wstring_view debugName,
    RhiOwnedResourceHandle& outResource,
    RhiResourceViewHandle& outView)
{
	return m_resourceService != nullptr ?
	           m_resourceService->CreateStructuredBuffer(data, sizeInBytes, strideInBytes, debugName, outResource, outView) :
	           false;
}

bool D3D12RenderHardwareInterface::CreateIndexBuffer(
    const void* data,
    std::size_t sizeInBytes,
    RhiIndexFormat format,
    std::wstring_view debugName,
    RhiOwnedResourceHandle& outResource,
    RhiIndexBufferView& outView)
{
	return m_resourceService != nullptr ? m_resourceService->CreateIndexBuffer(data, sizeInBytes, format, debugName, outResource, outView) :
	                                      false;
}

void D3D12RenderHardwareInterface::ReleaseOwnedResource(RhiOwnedResourceHandle resource) noexcept
{
	if (m_resourceService != nullptr)
	{
		m_resourceService->ReleaseOwnedResource(resource);
	}
}

NativeResourceHandle D3D12RenderHardwareInterface::GetNativeResource(RhiOwnedResourceHandle resource) const noexcept
{
	return m_resourceService != nullptr ? m_resourceService->GetNativeResource(resource) : NativeResourceHandle{};
}

RhiGpuVirtualAddress D3D12RenderHardwareInterface::GetResourceGpuVirtualAddress(RhiOwnedResourceHandle resource) const noexcept
{
	return m_resourceService != nullptr ? m_resourceService->GetResourceGpuVirtualAddress(resource) : 0;
}

RhiRayTracingAccelerationStructurePrebuildInfo D3D12RenderHardwareInterface::GetBottomLevelAccelerationStructurePrebuildInfo(
    const RhiRayTracingGeometryDesc& geometry) const noexcept
{
	return m_rayTracingServices != nullptr ? m_rayTracingServices->GetBottomLevelAccelerationStructurePrebuildInfo(geometry) :
	                                        RhiRayTracingAccelerationStructurePrebuildInfo{};
}

RhiRayTracingAccelerationStructurePrebuildInfo D3D12RenderHardwareInterface::GetTopLevelAccelerationStructurePrebuildInfo(
    std::uint32_t instanceCount,
    ERhiClassicTlasBuildFlags buildFlags) const noexcept
{
	return m_rayTracingServices != nullptr ? m_rayTracingServices->GetTopLevelAccelerationStructurePrebuildInfo(instanceCount, buildFlags) :
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
	return m_resourceService != nullptr ? m_resourceService->GetTextureAllocationInfo(desc) : RhiResourceAllocationInfo{};
}

RhiResourceAllocationInfo D3D12RenderHardwareInterface::GetBufferAllocationInfo(const RhiBufferResourceDesc& desc) const noexcept
{
	return m_resourceService != nullptr ? m_resourceService->GetBufferAllocationInfo(desc) : RhiResourceAllocationInfo{};
}

RhiOwnedMemoryBlockHandle D3D12RenderHardwareInterface::CreateTransientMemoryBlock(
    RhiTransientAllocationPool pool,
    std::uint64_t sizeInBytes,
    std::uint64_t alignment,
    std::wstring_view debugName)
{
	return m_resourceService != nullptr ? m_resourceService->CreateTransientMemoryBlock(pool, sizeInBytes, alignment, debugName) :
	                                      RhiOwnedMemoryBlockHandle{};
}

void D3D12RenderHardwareInterface::ReleaseTransientMemoryBlock(RhiOwnedMemoryBlockHandle memoryBlock) noexcept
{
	if (m_resourceService != nullptr)
	{
		m_resourceService->ReleaseTransientMemoryBlock(memoryBlock);
	}
}

RhiOwnedResourceHandle D3D12RenderHardwareInterface::CreateAliasingTextureResource(
    RhiOwnedMemoryBlockHandle memoryBlock,
    std::uint64_t memoryBlockOffset,
    const RhiTransientTextureAllocationDesc& desc,
    std::wstring_view debugName)
{
	return m_resourceService != nullptr ?
	           m_resourceService->CreateAliasingTextureResource(memoryBlock, memoryBlockOffset, desc, debugName) :
	           RhiOwnedResourceHandle{};
}

RhiOwnedResourceHandle D3D12RenderHardwareInterface::CreateAliasingBufferResource(
    RhiOwnedMemoryBlockHandle memoryBlock,
    std::uint64_t memoryBlockOffset,
    const RhiTransientBufferAllocationDesc& desc,
    std::wstring_view debugName)
{
	return m_resourceService != nullptr ?
	           m_resourceService->CreateAliasingBufferResource(memoryBlock, memoryBlockOffset, desc, debugName) :
	           RhiOwnedResourceHandle{};
}

RhiResourceViewHandle D3D12RenderHardwareInterface::CreateResourceView(const RhiResourceViewDesc& desc)
{
	return m_descriptorService != nullptr ? m_descriptorService->CreateResourceView(desc) : RhiResourceViewHandle{};
}

void D3D12RenderHardwareInterface::ReleaseResourceView(RhiResourceViewHandle view) noexcept
{
	if (m_descriptorService != nullptr)
	{
		m_descriptorService->ReleaseResourceView(view);
	}
}

RhiCpuDescriptorHandle D3D12RenderHardwareInterface::GetResourceViewCpuHandle(RhiResourceViewHandle view) const noexcept
{
	return m_descriptorService != nullptr ? m_descriptorService->GetResourceViewCpuHandle(view) : RhiCpuDescriptorHandle{};
}

RhiGpuDescriptorHandle D3D12RenderHardwareInterface::GetResourceViewGpuHandle(RhiResourceViewHandle view) const noexcept
{
	return m_descriptorService != nullptr ? m_descriptorService->GetResourceViewGpuHandle(view) : RhiGpuDescriptorHandle{};
}

NativeTextureViewInfo D3D12RenderHardwareInterface::GetNativeTextureViewInfo(RhiResourceViewHandle, ResourceState) const noexcept
{
	return {};
}

std::uint64_t D3D12RenderHardwareInterface::ResolveImGuiTextureId(RhiGpuDescriptorHandle shaderResourceView) noexcept
{
	return shaderResourceView.Value;
}

bool D3D12RenderHardwareInterface::SupportsUnorderedAccess(NativeResourceHandle resource) const noexcept
{
	return m_resourceService != nullptr && m_resourceService->SupportsUnorderedAccess(resource);
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

PixelFormat D3D12RenderHardwareInterface::GetPresentColorFormat() const noexcept
{
	return m_swapChain != nullptr ? m_swapChain->GetBackBufferFormat() : PixelFormat::Unknown;
}

PixelFormat D3D12RenderHardwareInterface::GetPresentDepthStencilFormat() const noexcept
{
	return PixelFormat::Unknown;
}

void D3D12RenderHardwareInterface::SetSamplerTableHandle(RhiDescriptorTableHandle samplerTableHandle) noexcept
{
	if (m_descriptorService != nullptr)
	{
		m_descriptorService->SetSamplerTableHandle(samplerTableHandle);
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12RenderHardwareInterface::ResolveDescriptorTableCpuHandle(
    RhiDescriptorTableHandle tableHandle,
    std::uint32_t descriptorIndex) const noexcept
{
	const RhiCpuDescriptorHandle handle =
	    m_descriptorService != nullptr ? m_descriptorService->GetDescriptorTableCpuHandle(tableHandle, descriptorIndex)
	                                   : RhiCpuDescriptorHandle{};
	return D3D12_CPU_DESCRIPTOR_HANDLE{handle.Value};
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12RenderHardwareInterface::ResolveDescriptorTableGpuHandle(
    RhiDescriptorTableHandle tableHandle,
    std::uint32_t descriptorIndex) const noexcept
{
	const RhiGpuDescriptorHandle handle =
	    m_descriptorService != nullptr ? m_descriptorService->GetDescriptorTableGpuHandle(tableHandle, descriptorIndex)
	                                   : RhiGpuDescriptorHandle{};
	return D3D12_GPU_DESCRIPTOR_HANDLE{handle.Value};
}

bool D3D12RenderHardwareInterface::BuildPartitionedTopLevelAccelerationStructure(
    ID3D12GraphicsCommandList7* commandList,
    const RhiPartitionedTlasBuildCommandDesc& desc) const noexcept
{
	return m_rayTracingServices != nullptr && m_rayTracingServices->BuildPartitionedTopLevelAccelerationStructure(commandList, desc);
}

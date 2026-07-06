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

#include <d3d12.h>
#include <string>

namespace
{
	RhiBackendDiagnosticsSupport BuildBackendDiagnosticsSupport(
	    const RenderDiagnostics* diagnostics,
	    bool validationEnabled,
	    bool supportsDebugLayer) noexcept
	{
		if (diagnostics == nullptr)
		{
			return RhiBackendDiagnosticsSupport{
			    .ValidationEnabled = validationEnabled,
			    .SupportsDebugLayer = supportsDebugLayer};
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
		    .SupportsCrashDiagnostics = diagnosticsCapabilities.SupportsCrashDiagnostics};
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
		    .SupportsDelayedDestructionTracking = memoryDiagnostics != nullptr && memoryDiagnostics->SupportsDelayedDestructionTracking(),
		    .SupportsResidencyPressure = memoryDiagnostics != nullptr && memoryDiagnostics->SupportsBudgetQueries()};
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
	    m_rhi != nullptr && m_rhi->IsValidationEnabled());
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
	support.SupportsUnorderedAccess = (data.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE) != 0;
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

ID3D12DescriptorHeap* D3D12RenderHardwareInterface::GetD3D12ShaderResourceDescriptorHeap() const noexcept
{
	return m_descriptorService != nullptr ? m_descriptorService->GetShaderResourceDescriptorHeap() : nullptr;
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

std::uint64_t D3D12RenderHardwareInterface::ResolveImGuiTextureId(RhiGpuDescriptorHandle shaderResourceView) noexcept
{
	return shaderResourceView.Value;
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
	if (m_descriptorService != nullptr)
	{
		m_descriptorService->BindGlobalDescriptorState(commandList);
	}

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
	if (m_descriptorService != nullptr)
	{
		m_descriptorService->BindGlobalDescriptorState(commandList);
	}

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

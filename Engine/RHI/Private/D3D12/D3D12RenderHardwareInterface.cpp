#include "PCH.h"

#include "D3D12/D3D12RenderHardwareInterface.h"

#include "Commands/RenderCommandList.h"
#include "D3D12/Commands/D3D12CommandRecordingContext.h"
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
#include "D3D12/Pipeline/D3D12BindingLayout.h"
#include "D3D12/Pipeline/D3D12Pipeline.h"
#include "D3D12/RayTracing/D3D12RayTracingServices.h"
#include "D3D12/Resources/D3D12ResourceService.h"
#include "D3D12/Resources/D3D12UploadService.h"
#include "D3D12/Samplers/D3D12SamplerLibrary.h"
#include "D3D12/UI/D3D12ImGuiBackend.h"
#include "Presentation/RhiPresentationServiceAdapter.h"
#include "Pipeline/RhiPipelineServiceAdapter.h"
#include "Shaders/ShaderTarget.h"

#include <d3d12.h>
#include <string>

D3D12RenderHardwareInterface::D3D12RenderHardwareInterface(
    D3D12Rhi& rhi,
    D3D12GpuMemoryAllocator& memoryAllocator,
    D3D12DescriptorHeapManager& descriptorHeapManager,
    D3D12SwapChain& swapChain,
    D3D12UploadService& uploadService) noexcept :
    m_rhi(&rhi), m_descriptorHeapManager(&descriptorHeapManager), m_swapChain(&swapChain), m_uploadService(&uploadService)
{
	m_interopService = std::make_unique<D3D12InteropService>(*this);
	m_captureService = std::make_unique<D3D12CaptureService>(rhi);
	m_presentationService = std::make_unique<RhiPresentationServiceAdapter<D3D12RenderHardwareInterface>>(*this);
	m_pipelineService = std::make_unique<
	    RhiPipelineServiceAdapter<D3D12Rhi, D3D12Pipeline, D3D12BindingLayoutCompiler>>(rhi);
	m_descriptorService = std::make_unique<D3D12DescriptorService>(rhi, descriptorHeapManager, m_capabilities);
	m_resourceService = std::make_unique<D3D12ResourceService>(rhi, memoryAllocator, m_capabilities);
	m_rayTracingServices = std::make_unique<D3D12RayTracingServices>(rhi, memoryAllocator, rhi.GetNvapiRayTracingProvider());
	m_diagnostics = CreateD3D12RenderDiagnostics(rhi, swapChain.GetMaximumFramesInFlight());
	m_capabilities = BuildCapabilities();
	m_imguiBackend = std::make_unique<D3D12ImGuiBackend>(*this);
}

D3D12RenderHardwareInterface::~D3D12RenderHardwareInterface() noexcept = default;

D3D12RecordingResourceUseToken D3D12RenderHardwareInterface::BeginResourceTracking(
    RhiResourceHandle resource,
    bool coordinatorRecording) noexcept
{
	return m_resourceService->BeginResourceTracking(resource, coordinatorRecording);
}

void D3D12RenderHardwareInterface::EndResourceTracking(D3D12RecordingResourceUseToken use, RhiSubmissionToken submissionToken) noexcept
{
	m_resourceService->EndResourceTracking(use, submissionToken);
}

RhiCapabilities D3D12RenderHardwareInterface::BuildCapabilities() const noexcept
{
	RhiCapabilities capabilities{};
	const D3D_FEATURE_LEVEL featureLevel = m_rhi->GetDeviceFeatureLevel();
	const std::uint32_t featureLevelMajor = featureLevel >= D3D_FEATURE_LEVEL_12_0   ? 12u
	                                        : featureLevel >= D3D_FEATURE_LEVEL_11_0 ? 11u
	                                                                                 : 0u;
	const std::uint32_t featureLevelMinor = featureLevel == D3D_FEATURE_LEVEL_12_2   ? 2u
	                                        : featureLevel == D3D_FEATURE_LEVEL_12_1 ? 1u
	                                        : featureLevel == D3D_FEATURE_LEVEL_12_0 ? 0u
	                                        : featureLevel == D3D_FEATURE_LEVEL_11_1 ? 1u
	                                        : featureLevel == D3D_FEATURE_LEVEL_11_0 ? 0u
	                                                                                 : 0u;
	capabilities.BackendApi = ERhiBackendApi::D3D12;
	capabilities.RuntimeShaderBinaryFormat = ShaderBinaryFormat::Dxil;
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
	capabilities.DescriptorIndexing = RhiDescriptorIndexingCapabilities{
	    .SupportsSampledImageArrayNonUniformIndexing = true,
	    .SupportsPartiallyBoundDescriptorArrays = true};
	capabilities.UploadReadback =
	    RhiUploadReadbackCapabilities{.SupportsBufferUpload = true, .SupportsTextureUpload = true, .SupportsReadback = true};
	capabilities.Presentation = RhiPresentationCapabilities{
	    .BackBufferCount = m_swapChain->GetBackBufferCount(),
	    .MaximumFramesInFlight = m_swapChain->GetMaximumFramesInFlight(),
	    .Throttle = ERhiPresentationThrottle::FrameLatencyWaitableObject};
	for (std::size_t index = 0; index < capabilities.FormatSupport.size(); ++index)
	{
		capabilities.FormatSupport[index] = QueryFormatSupport(kRhiCapabilityPixelFormats[index]);
	}
	capabilities.Diagnostics = BuildBackendDiagnosticsSupport();
	capabilities.RayTracing = m_rhi->GetRayTracingCapabilities();
	capabilities.SupportsMeshShaders = false;
	capabilities.SupportsTaskShaders = false;
	capabilities.Queues.Set(ERhiQueueType::Graphics, true, true);
	capabilities.Queues.Set(ERhiQueueType::Compute, true, true);
	capabilities.Queues.Set(ERhiQueueType::Copy, true, true);
	capabilities.SupportsPresent = m_swapChain->GetBackBufferFormat() != PixelFormat::Unknown;
	capabilities.MemoryAllocator = ERhiMemoryAllocatorBackend::D3D12Managed;
	capabilities.MemorySupport = BuildBackendMemorySupport();
	capabilities.ExternalFeatureInterop = BuildD3D12ExternalFeatureInteropCapabilities(m_rhi, m_rhi->GetDevice() != nullptr);
	return capabilities;
}

RhiBackendDiagnosticsSupport D3D12RenderHardwareInterface::BuildBackendDiagnosticsSupport() const noexcept
{
	const bool validationEnabled = m_rhi->IsValidationEnabled();
	const RhiDiagnosticsCapabilities diagnosticsCapabilities = m_diagnostics->GetCapabilities();
	return RhiBackendDiagnosticsSupport{
	    .ValidationEnabled = validationEnabled,
	    .SupportsDebugLayer = validationEnabled,
	    .SupportsObjectNames = diagnosticsCapabilities.SupportsObjectNames,
	    .SupportsGpuEvents = diagnosticsCapabilities.SupportsGpuEvents,
	    .SupportsTimestampQueries = diagnosticsCapabilities.SupportsTimestampQueries,
	    .SupportsDebugMessages = diagnosticsCapabilities.SupportsDebugMessages,
	    .SupportsLiveObjectReports = diagnosticsCapabilities.SupportsLiveObjectReports,
	    .SupportsCrashDiagnostics = diagnosticsCapabilities.SupportsCrashDiagnostics};
}

RhiBackendMemorySupport D3D12RenderHardwareInterface::BuildBackendMemorySupport() const noexcept
{
	const RenderMemoryDiagnostics* const memoryDiagnostics = m_diagnostics->GetMemoryDiagnostics();
	return RhiBackendMemorySupport{
	    .SupportsMemoryDiagnostics = memoryDiagnostics != nullptr,
	    .SupportsBudgetQueries = memoryDiagnostics != nullptr && memoryDiagnostics->SupportsBudgetQueries(),
	    .SupportsDelayedDestructionTracking = memoryDiagnostics != nullptr && memoryDiagnostics->SupportsDelayedDestructionTracking(),
	    .SupportsResidencyPressure = memoryDiagnostics != nullptr && memoryDiagnostics->SupportsBudgetQueries()};
}

RhiFormatSupport D3D12RenderHardwareInterface::QueryFormatSupport(PixelFormat format) const noexcept
{
	RhiFormatSupport support{.Format = format};
	if (format == PixelFormat::Unknown)
	{
		return support;
	}

	D3D12_FEATURE_DATA_FORMAT_SUPPORT data{};
	data.Format = D3D12TypeConversions::ToDxgiFormat(format);
	const HRESULT supportResult = m_rhi->GetDevice()->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &data, sizeof(data));
	if (data.Format == DXGI_FORMAT_UNKNOWN || FAILED(supportResult))
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
	return m_rhi->GetCurrentFrameIndex();
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
	return *m_uploadService;
}

const RhiUploadService& D3D12RenderHardwareInterface::GetUploadService() const noexcept
{
	return *m_uploadService;
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
	m_rhi->WaitForIdle();
	m_resourceService->FlushDeferredResourceReleases();
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
	return NativeGraphicsDeviceHandle{m_rhi->GetDevice().Get()};
}

NativeGraphicsQueueHandle D3D12RenderHardwareInterface::GetGraphicsQueueHandle() const noexcept
{
	return NativeGraphicsQueueHandle{m_rhi->GetCommandQueue().Get()};
}

RenderCommandList& D3D12RenderHardwareInterface::GetGraphicsCommandList(std::uint32_t frameIndex) noexcept
{
	return GetCommandList(ERhiQueueType::Graphics, frameIndex);
}

RenderCommandList& D3D12RenderHardwareInterface::GetCommandList(ERhiQueueType queueType, std::uint32_t frameIndex) noexcept
{
	m_resourceService->DrainCompletedResourceReleases();
	return m_commandRecordingContext->GetCurrentCommandList(queueType, frameIndex);
}

void D3D12RenderHardwareInterface::SetCommandRecordingContext(D3D12CommandRecordingContext& commandContext) noexcept
{
	m_commandRecordingContext = &commandContext;
}

RhiRayTracingCapabilities D3D12RenderHardwareInterface::GetRayTracingCapabilities() const noexcept
{
	return m_rayTracingServices->GetCapabilities();
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
	return m_descriptorService->GetShaderResourceDescriptorHeap();
}

RhiViewport D3D12RenderHardwareInterface::GetBackBufferViewport() const noexcept
{
	return m_swapChain->GetDefaultViewport();
}

RhiRect D3D12RenderHardwareInterface::GetBackBufferScissorRect() const noexcept
{
	return m_swapChain->GetDefaultScissorRect();
}

RhiCpuDescriptorHandle D3D12RenderHardwareInterface::GetBackBufferRenderTargetView() const noexcept
{
	return RhiCpuDescriptorHandle{m_swapChain->GetCPUHandle().ptr};
}

RhiResourceHandle D3D12RenderHardwareInterface::GetBackBufferResource() const noexcept
{
	return RhiResourceHandle{m_swapChain->GetCurrentResource()};
}

RhiRayTracingAccelerationStructurePrebuildInfo D3D12RenderHardwareInterface::GetBottomLevelAccelerationStructurePrebuildInfo(
    const RhiRayTracingGeometryDesc& geometry) const noexcept
{
	return m_rayTracingServices->GetBottomLevelAccelerationStructurePrebuildInfo(geometry);
}

RhiRayTracingAccelerationStructurePrebuildInfo D3D12RenderHardwareInterface::GetTopLevelAccelerationStructurePrebuildInfo(
    std::uint32_t instanceCount,
    ERhiClassicTlasBuildFlags buildFlags) const noexcept
{
	return m_rayTracingServices->GetTopLevelAccelerationStructurePrebuildInfo(instanceCount, buildFlags);
}

RhiOwnedResourceHandle D3D12RenderHardwareInterface::CreateRayTracingScratchBuffer(std::uint64_t sizeInBytes, std::wstring_view debugName)
{
	return m_rayTracingServices->CreateScratchBuffer(sizeInBytes, debugName);
}

RhiOwnedResourceHandle D3D12RenderHardwareInterface::CreateRayTracingAccelerationStructureBuffer(
    std::uint64_t sizeInBytes,
    ERhiRayTracingAccelerationStructureType type,
    std::wstring_view debugName)
{
	return m_rayTracingServices->CreateAccelerationStructureBuffer(sizeInBytes, type, debugName);
}

RhiOwnedResourceHandle D3D12RenderHardwareInterface::CreateRayTracingInstanceBuffer(
    const RhiRayTracingInstanceDesc* instances,
    std::uint32_t instanceCount,
    std::wstring_view debugName)
{
	return m_rayTracingServices->CreateInstanceBuffer(instances, instanceCount, debugName);
}

void D3D12RenderHardwareInterface::BeginPresentRenderPass(const float clearColor[4]) noexcept
{
	RhiResourceHandle presentTexture{m_swapChain->GetCurrentResource()};
	RenderCommandList& commandList = GetGraphicsCommandList(GetCurrentFrameIndex());
	commandList.TransitionResource(presentTexture, ResourceState::Present, ResourceState::RenderTarget);
	m_descriptorService->BindGlobalDescriptorState(commandList);

	const RhiCpuDescriptorHandle renderTargetView = GetBackBufferRenderTargetView();
	commandList.SetRenderTarget(renderTargetView);

	static constexpr float defaultClearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	commandList.ClearRenderTarget(renderTargetView, clearColor != nullptr ? clearColor : defaultClearColor);
}

void D3D12RenderHardwareInterface::BeginPresentOverlayPass() noexcept
{
	RhiResourceHandle presentTexture{m_swapChain->GetCurrentResource()};
	RenderCommandList& commandList = GetGraphicsCommandList(GetCurrentFrameIndex());
	commandList.TransitionResource(presentTexture, ResourceState::Present, ResourceState::RenderTarget);
	m_descriptorService->BindGlobalDescriptorState(commandList);

	const RhiCpuDescriptorHandle renderTargetView = GetBackBufferRenderTargetView();
	commandList.SetRenderTarget(renderTargetView);
}

void D3D12RenderHardwareInterface::EndPresentRenderPass() noexcept
{
	RhiResourceHandle presentTexture{m_swapChain->GetCurrentResource()};
	RenderCommandList& commandList = GetGraphicsCommandList(GetCurrentFrameIndex());
	commandList.TransitionResource(presentTexture, ResourceState::RenderTarget, ResourceState::Present);
}

PixelFormat D3D12RenderHardwareInterface::GetPresentColorFormat() const noexcept
{
	return m_swapChain->GetBackBufferFormat();
}

PixelFormat D3D12RenderHardwareInterface::GetPresentDepthStencilFormat() const noexcept
{
	return PixelFormat::Unknown;
}

void D3D12RenderHardwareInterface::SetSamplerTableHandle(RhiDescriptorTableHandle samplerTableHandle) noexcept
{
	m_descriptorService->SetSamplerTableHandle(samplerTableHandle);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12RenderHardwareInterface::ResolveDescriptorTableCpuHandle(
    RhiDescriptorTableHandle tableHandle,
    std::uint32_t descriptorIndex) const noexcept
{
	const RhiCpuDescriptorHandle handle = m_descriptorService->GetDescriptorTableCpuHandle(tableHandle, descriptorIndex);
	return D3D12_CPU_DESCRIPTOR_HANDLE{handle.Value};
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12RenderHardwareInterface::ResolveDescriptorTableGpuHandle(
    RhiDescriptorTableHandle tableHandle,
    std::uint32_t descriptorIndex) const noexcept
{
	const RhiGpuDescriptorHandle handle = m_descriptorService->GetDescriptorTableGpuHandle(tableHandle, descriptorIndex);
	return D3D12_GPU_DESCRIPTOR_HANDLE{handle.Value};
}

bool D3D12RenderHardwareInterface::BuildPartitionedTopLevelAccelerationStructure(
    ID3D12GraphicsCommandList7* commandList,
    const RhiPartitionedTlasBuildCommandDesc& desc) const noexcept
{
	return m_rayTracingServices->BuildPartitionedTopLevelAccelerationStructure(commandList, desc);
}

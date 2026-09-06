#include "Vulkan/VulkanPCH.h"

#include "Vulkan/VulkanRenderHardwareInterface.h"

#include "Shaders/ShaderTarget.h"
#include "Vulkan/Capture/VulkanCaptureService.h"
#include "Vulkan/Commands/VulkanCommandRecordingContext.h"
#include "Vulkan/Commands/VulkanRenderCommandList.h"
#include "Vulkan/Descriptors/VulkanDescriptorAllocator.h"
#include "Vulkan/Descriptors/VulkanDescriptorService.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/Device/VulkanExternalFeatureInteropCapabilities.h"
#include "Vulkan/Diagnostics/VulkanRenderDiagnostics.h"
#include "Vulkan/Interop/VulkanInteropService.h"
#include "Vulkan/Memory/VulkanGpuMemoryAllocator.h"
#include "Vulkan/Pipeline/VulkanBindingLayout.h"
#include "Vulkan/Pipeline/VulkanPipeline.h"
#include "Vulkan/Pipeline/VulkanRayTracingPipeline.h"
#include "Presentation/RhiPresentationServiceAdapter.h"
#include "Pipeline/RhiPipelineServiceAdapter.h"
#include "Vulkan/RayTracing/VulkanRayTracingServices.h"
#include "Vulkan/Resources/VulkanResourceService.h"
#include "Vulkan/Resources/VulkanUploadService.h"
#include "Vulkan/Samplers/VulkanSamplerLibrary.h"
#include "Vulkan/SwapChain/VulkanSwapChain.h"
#include "Vulkan/UI/VulkanImGuiBackend.h"
#include "Vulkan/VulkanTypeConversions.h"

#include <array>

static const auto g_vulkanRenderHardwareInterfaceLogger = Logging::GetOrCreateLogger("RHI.Vulkan.Interface");

VulkanRenderHardwareInterface::VulkanRenderHardwareInterface(
    VulkanRhi& rhi,
    VulkanSwapChain& swapChain,
    VulkanGpuMemoryAllocator& memoryAllocator) noexcept :
    m_rhi(&rhi),
    m_swapChain(&swapChain),
    m_memoryAllocator(&memoryAllocator)
{
	m_interopService = std::make_unique<VulkanInteropService>(*this);
	m_captureService = std::make_unique<VulkanCaptureService>(rhi);
	m_presentationService = std::make_unique<RhiPresentationServiceAdapter<VulkanRenderHardwareInterface>>(*this);
	m_pipelineService =
	    std::make_unique<RhiPipelineServiceAdapter<VulkanRhi, VulkanPipeline, VulkanRayTracingPipeline, VulkanBindingLayoutCompiler>>(rhi);
	m_rayTracingServices = std::make_unique<VulkanRayTracingServices>(rhi, memoryAllocator);
	m_descriptorService = std::make_unique<VulkanDescriptorService>(rhi, memoryAllocator, m_capabilities);
	m_resourceService = std::make_unique<VulkanResourceService>(rhi, memoryAllocator, m_capabilities);
	m_uploadService = std::make_unique<VulkanUploadService>(memoryAllocator);
	m_samplerLibrary = std::make_unique<VulkanSamplerLibrary>(rhi, *m_descriptorService);
	m_descriptorService->SetSamplerLibrary(*m_samplerLibrary);
	m_imguiBackend = std::make_unique<VulkanImGuiBackend>(*this, *m_descriptorService);
	m_diagnostics = CreateVulkanRenderDiagnostics(rhi, memoryAllocator);
	RebuildSwapChainBackBufferViews();
	m_capabilities = BuildCapabilities();
}

VulkanRenderHardwareInterface::~VulkanRenderHardwareInterface() noexcept
{
	m_samplerLibrary.reset();
	m_imguiBackend.reset();
	m_resourceService.reset();
	m_uploadService.reset();
}

std::uint32_t VulkanRenderHardwareInterface::GetCurrentFrameIndex() const noexcept
{
	return m_currentFrameIndex;
}

RhiResourceService& VulkanRenderHardwareInterface::GetResourceService() noexcept
{
	return *m_resourceService;
}

const RhiResourceService& VulkanRenderHardwareInterface::GetResourceService() const noexcept
{
	return *m_resourceService;
}

RhiDescriptorService& VulkanRenderHardwareInterface::GetDescriptorService() noexcept
{
	return *m_descriptorService;
}

const RhiDescriptorService& VulkanRenderHardwareInterface::GetDescriptorService() const noexcept
{
	return *m_descriptorService;
}

RhiPipelineService& VulkanRenderHardwareInterface::GetPipelineService() noexcept
{
	return *m_pipelineService;
}

RhiUploadService& VulkanRenderHardwareInterface::GetUploadService() noexcept
{
	return *m_uploadService;
}

const RhiUploadService& VulkanRenderHardwareInterface::GetUploadService() const noexcept
{
	return *m_uploadService;
}

RhiRayTracingService& VulkanRenderHardwareInterface::GetRayTracingService() noexcept
{
	return *m_rayTracingServices;
}

const RhiRayTracingService& VulkanRenderHardwareInterface::GetRayTracingService() const noexcept
{
	return *m_rayTracingServices;
}

void VulkanRenderHardwareInterface::WaitForIdle() noexcept
{
	m_rhi->WaitForIdle();
	m_resourceService->FlushDeferredResourceReleases();
}

RhiInteropService& VulkanRenderHardwareInterface::GetInteropService() noexcept
{
	return *m_interopService;
}

const RhiInteropService& VulkanRenderHardwareInterface::GetInteropService() const noexcept
{
	return *m_interopService;
}

RhiCaptureService& VulkanRenderHardwareInterface::GetCaptureService() noexcept
{
	return *m_captureService;
}

RhiPresentationService& VulkanRenderHardwareInterface::GetPresentationService() noexcept
{
	return *m_presentationService;
}

const RhiPresentationService& VulkanRenderHardwareInterface::GetPresentationService() const noexcept
{
	return *m_presentationService;
}

NativeGraphicsDeviceHandle VulkanRenderHardwareInterface::GetDeviceHandle() const noexcept
{
	return NativeGraphicsDeviceHandle{m_rhi->GetDevice()};
}

NativeGraphicsQueueHandle VulkanRenderHardwareInterface::GetGraphicsQueueHandle() const noexcept
{
	return NativeGraphicsQueueHandle{m_rhi->GetGraphicsQueue()};
}

RenderCommandList& VulkanRenderHardwareInterface::GetGraphicsCommandList(std::uint32_t) noexcept
{
	return GetCommandList(ERhiQueueType::Graphics, m_currentFrameIndex);
}

RenderCommandList& VulkanRenderHardwareInterface::GetCommandList(ERhiQueueType queueType, std::uint32_t frameIndex) noexcept
{
	m_resourceService->DrainCompletedResourceReleases();
	return m_commandRecordingContext->GetCurrentCommandList(queueType, frameIndex);
}

RhiRayTracingCapabilities VulkanRenderHardwareInterface::GetRayTracingCapabilities() const noexcept
{
	return m_rayTracingServices->GetCapabilities();
}

RhiCapabilities VulkanRenderHardwareInterface::BuildCapabilities() const noexcept
{
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(m_rhi->GetPhysicalDevice(), &properties);

	RhiCapabilities capabilities{};
	capabilities.BackendApi = ERhiBackendApi::Vulkan;
	capabilities.RuntimeShaderBinaryFormat = ShaderBinaryFormat::SpirV;
	const std::uint32_t apiVersion = m_rhi->GetAdapterInfo().ApiVersion;
	capabilities.BackendVersion = RhiBackendVersionInfo{
	    .Semantic = ERhiBackendVersionSemantic::ApiVersion,
	    .Major = VK_VERSION_MAJOR(apiVersion),
	    .Minor = VK_VERSION_MINOR(apiVersion),
	    .Patch = VK_VERSION_PATCH(apiVersion),
	    .PackedValue = apiVersion};
	capabilities.DescriptorModel = ERhiDescriptorModel::DescriptorSets;
	capabilities.BindingLimits = RhiBindingLimits{
	    .MaxDescriptorSets = properties.limits.maxBoundDescriptorSets,
	    .MaxShaderResourceDescriptors = properties.limits.maxDescriptorSetSampledImages + properties.limits.maxDescriptorSetStorageImages
	        + properties.limits.maxDescriptorSetUniformBuffers + properties.limits.maxDescriptorSetStorageBuffers,
	    .MaxSamplerDescriptors = properties.limits.maxDescriptorSetSamplers,
	    .MaxDescriptorTableEntries = properties.limits.maxDescriptorSetSampledImages + properties.limits.maxDescriptorSetStorageImages,
	    .MaxPushConstantBytes = properties.limits.maxPushConstantsSize};
	capabilities.DescriptorIndexing = RhiDescriptorIndexingCapabilities{
	    .SupportsSampledImageArrayNonUniformIndexing = m_rhi->GetFeatureStatus().EnabledSampledImageArrayNonUniformIndexing,
	    .SupportsPartiallyBoundDescriptorArrays = m_rhi->GetFeatureStatus().EnabledPartiallyBoundDescriptorArrays};
	capabilities.UploadReadback =
	    RhiUploadReadbackCapabilities{.SupportsBufferUpload = true, .SupportsTextureUpload = true, .SupportsReadback = true};
	capabilities.Presentation = RhiPresentationCapabilities{
	    .BackBufferCount = m_swapChain->GetBackBufferCount(),
	    .MaximumFramesInFlight = m_swapChain->GetMaximumFramesInFlight(),
	    .Throttle = ERhiPresentationThrottle::SwapChainImageAcquisition};
	for (std::size_t index = 0; index < capabilities.FormatSupport.size(); ++index)
	{
		capabilities.FormatSupport[index] = QueryFormatSupport(kRhiCapabilityPixelFormats[index]);
	}
	capabilities.Diagnostics = BuildBackendDiagnosticsSupport();
	capabilities.RayTracing = m_rhi->GetRayTracingCapabilities();
	capabilities.SupportsMeshShaders = false;
	capabilities.SupportsTaskShaders = false;
	const bool hasComputeQueue = m_rhi->GetQueue(ERhiQueueType::Compute) != VK_NULL_HANDLE;
	const bool hasCopyQueue = m_rhi->GetQueue(ERhiQueueType::Copy) != VK_NULL_HANDLE;
	capabilities.Queues.Set(ERhiQueueType::Graphics, true, true);
	capabilities.Queues.Set(ERhiQueueType::Compute, hasComputeQueue, hasComputeQueue && m_rhi->HasIndependentQueue(ERhiQueueType::Compute));
	capabilities.Queues.Set(ERhiQueueType::Copy, hasCopyQueue, hasCopyQueue && m_rhi->HasIndependentQueue(ERhiQueueType::Copy));
	capabilities.SupportsPresent = m_swapChain->GetBackBufferFormat() != PixelFormat::Unknown;
	capabilities.MemoryAllocator = ERhiMemoryAllocatorBackend::VulkanManaged;
	capabilities.MemorySupport = BuildBackendMemorySupport();
	capabilities.ExternalFeatureInterop = BuildVulkanExternalFeatureInteropCapabilities(m_rhi, m_commandRecordingContext != nullptr);
	return capabilities;
}

RhiBackendDiagnosticsSupport VulkanRenderHardwareInterface::BuildBackendDiagnosticsSupport() const noexcept
{
	const bool validationEnabled = m_rhi->IsValidationEnabled();
	const RhiDiagnosticsCapabilities diagnostics = m_diagnostics->GetCapabilities();
	return RhiBackendDiagnosticsSupport{
	    .ValidationEnabled = validationEnabled,
	    .SupportsDebugLayer = validationEnabled,
	    .SupportsObjectNames = diagnostics.SupportsObjectNames,
	    .SupportsGpuEvents = diagnostics.SupportsGpuEvents,
	    .SupportsTimestampQueries = diagnostics.SupportsTimestampQueries,
	    .SupportsDebugMessages = diagnostics.SupportsDebugMessages,
	    .SupportsLiveObjectReports = diagnostics.SupportsLiveObjectReports,
	    .SupportsCrashDiagnostics = diagnostics.SupportsCrashDiagnostics};
}

RhiBackendMemorySupport VulkanRenderHardwareInterface::BuildBackendMemorySupport() const noexcept
{
	const RenderMemoryDiagnostics* const diagnostics = m_diagnostics->GetMemoryDiagnostics();
	return RhiBackendMemorySupport{
	    .SupportsMemoryDiagnostics = diagnostics != nullptr,
	    .SupportsBudgetQueries = diagnostics != nullptr && diagnostics->SupportsBudgetQueries(),
	    .SupportsDelayedDestructionTracking = diagnostics != nullptr && diagnostics->SupportsDelayedDestructionTracking(),
	    .SupportsResidencyPressure = diagnostics != nullptr && diagnostics->SupportsBudgetQueries()};
}

RhiFormatSupport VulkanRenderHardwareInterface::QueryFormatSupport(PixelFormat format) const noexcept
{
	RhiFormatSupport support{.Format = format};
	if (format == PixelFormat::Unknown)
	{
		return support;
	}

	const VkFormat nativeFormat = VulkanTypeConversions::ToVkFormat(format);
	if (nativeFormat == VK_FORMAT_UNDEFINED)
	{
		return support;
	}

	VkFormatProperties properties{};
	vkGetPhysicalDeviceFormatProperties(m_rhi->GetPhysicalDevice(), nativeFormat, &properties);
	const VkFormatFeatureFlags optimal = properties.optimalTilingFeatures;
	support.SupportsTexture = (optimal & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != 0
	    || (optimal & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) != 0 || (optimal & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0;
	support.SupportsShaderResource = (optimal & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != 0;
	support.SupportsUnorderedAccess = (optimal & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) != 0;
	support.SupportsRenderTarget = (optimal & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) != 0;
	support.SupportsDepthStencil = (optimal & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0;
	return support;
}

RenderDiagnostics& VulkanRenderHardwareInterface::GetDiagnostics() noexcept
{
	return *m_diagnostics;
}

const RenderDiagnostics& VulkanRenderHardwareInterface::GetDiagnostics() const noexcept
{
	return *m_diagnostics;
}

RhiImGuiRenderer& VulkanRenderHardwareInterface::GetImGuiRenderer() noexcept
{
	return *m_imguiBackend;
}

RhiViewport VulkanRenderHardwareInterface::GetBackBufferViewport() const noexcept
{
	return m_swapChain->GetDefaultViewport();
}

RhiRect VulkanRenderHardwareInterface::GetBackBufferScissorRect() const noexcept
{
	return m_swapChain->GetDefaultScissorRect();
}

RhiCpuDescriptorHandle VulkanRenderHardwareInterface::GetBackBufferRenderTargetView() const noexcept
{
	return m_descriptorService->GetResourceViewCpuHandle(GetCurrentBackBufferViewHandle());
}

RhiResourceHandle VulkanRenderHardwareInterface::GetBackBufferResource() const noexcept
{
	return m_swapChain->GetCurrentBackBufferResource();
}

RhiRayTracingAccelerationStructurePrebuildInfo VulkanRenderHardwareInterface::GetBottomLevelAccelerationStructurePrebuildInfo(
    const RhiRayTracingGeometryDesc& geometry) const noexcept
{
	return m_rayTracingServices->GetBottomLevelAccelerationStructurePrebuildInfo(geometry);
}

RhiRayTracingAccelerationStructurePrebuildInfo VulkanRenderHardwareInterface::GetTopLevelAccelerationStructurePrebuildInfo(
    std::uint32_t instanceCount,
    ERhiClassicTlasBuildFlags buildFlags) const noexcept
{
	return m_rayTracingServices->GetTopLevelAccelerationStructurePrebuildInfo(instanceCount, buildFlags);
}

RhiOwnedResourceHandle VulkanRenderHardwareInterface::CreateRayTracingScratchBuffer(std::uint64_t sizeInBytes, std::wstring_view debugName)
{
	return m_rayTracingServices->CreateScratchBuffer(sizeInBytes, debugName);
}

RhiOwnedResourceHandle VulkanRenderHardwareInterface::CreateRayTracingAccelerationStructureBuffer(
    std::uint64_t sizeInBytes,
    ERhiRayTracingAccelerationStructureType type,
    std::wstring_view debugName)
{
	return m_rayTracingServices->CreateAccelerationStructureBuffer(sizeInBytes, type, debugName);
}

RhiOwnedResourceHandle VulkanRenderHardwareInterface::CreateRayTracingInstanceBuffer(
    const RhiRayTracingInstanceDesc* instances,
    std::uint32_t instanceCount,
    std::wstring_view debugName)
{
	return m_rayTracingServices->CreateInstanceBuffer(instances, instanceCount, debugName);
}

void VulkanRenderHardwareInterface::BeginPresentRenderPass(RhiClearColorView clearColor) noexcept
{
	BeginCurrentBackBufferRendering(clearColor.data(), true);
}

void VulkanRenderHardwareInterface::BeginPresentOverlayPass() noexcept
{
	BeginCurrentBackBufferRendering(nullptr, false);
}

void VulkanRenderHardwareInterface::EndPresentRenderPass() noexcept
{
	EndCurrentBackBufferRendering();
}

PixelFormat VulkanRenderHardwareInterface::GetPresentColorFormat() const noexcept
{
	return m_swapChain->GetBackBufferFormat();
}

VkInstance VulkanRenderHardwareInterface::GetVulkanInstance() const noexcept
{
	return m_rhi->GetInstance();
}

VkPhysicalDevice VulkanRenderHardwareInterface::GetVulkanPhysicalDevice() const noexcept
{
	return m_rhi->GetPhysicalDevice();
}

VkDevice VulkanRenderHardwareInterface::GetVulkanDevice() const noexcept
{
	return m_rhi->GetDevice();
}

VkQueue VulkanRenderHardwareInterface::GetVulkanGraphicsQueue() const noexcept
{
	return m_rhi->GetGraphicsQueue();
}

std::uint32_t VulkanRenderHardwareInterface::GetVulkanGraphicsQueueFamilyIndex() const noexcept
{
	return m_rhi->GetGraphicsQueueFamilyIndex();
}

std::uint32_t VulkanRenderHardwareInterface::GetVulkanApiVersion() const noexcept
{
	return m_rhi->GetAdapterInfo().ApiVersion;
}

std::uint32_t VulkanRenderHardwareInterface::GetSwapChainBackBufferCount() const noexcept
{
	return m_swapChain->GetBackBufferCount();
}

VkFormat VulkanRenderHardwareInterface::GetNativeBackBufferFormat() const noexcept
{
	return m_swapChain->GetNativeBackBufferFormat();
}

void VulkanRenderHardwareInterface::SetCurrentFrameIndex(std::uint32_t frameIndex) noexcept
{
	m_currentFrameIndex = frameIndex;
}

void VulkanRenderHardwareInterface::SetCommandRecordingContext(VulkanCommandRecordingContext& commandRecordingContext) noexcept
{
	m_commandRecordingContext = &commandRecordingContext;
	m_capabilities = BuildCapabilities();
}

void VulkanRenderHardwareInterface::ResetTransientFrameResources() noexcept
{
	m_descriptorService->BeginFrame(m_currentFrameIndex);
}

void VulkanRenderHardwareInterface::RebuildSwapChainBackBufferViews() noexcept
{
	m_swapChainBackBufferLayouts.clear();
	const std::uint32_t backBufferCount = m_swapChain->GetBackBufferCount();
	m_swapChainBackBufferLayouts.assign(backBufferCount, VK_IMAGE_LAYOUT_UNDEFINED);
	m_descriptorService->RebuildSwapChainBackBufferViews(*m_swapChain);
}

RhiResourceViewHandle VulkanRenderHardwareInterface::GetCurrentBackBufferViewHandle() const noexcept
{
	const std::uint32_t backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
	return m_descriptorService->GetSwapChainBackBufferView(backBufferIndex);
}

void VulkanRenderHardwareInterface::BeginCurrentBackBufferRendering(const float* clearColor, bool clear) noexcept
{
	if (m_commandRecordingContext == nullptr)
	{
		Diagnostics::Fatal(
		    g_vulkanRenderHardwareInterfaceLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan present rendering began before command recording was initialized.");
	}

	const VkImage backBuffer = m_swapChain->GetCurrentBackBufferImage();
	const VkImageView backBufferView = m_swapChain->GetCurrentBackBufferImageView();
	if (backBuffer == VK_NULL_HANDLE || backBufferView == VK_NULL_HANDLE)
	{
		Diagnostics::Fatal(
		    g_vulkanRenderHardwareInterfaceLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan present rendering has no acquired swap-chain image and view.");
	}

	auto& commandList = static_cast<VulkanRenderCommandList&>(GetGraphicsCommandList(m_currentFrameIndex));
	commandList.SetViewport(GetBackBufferViewport());
	commandList.SetScissorRect(GetBackBufferScissorRect());
	commandList.SetRenderTarget(GetBackBufferRenderTargetView());

	const VkCommandBuffer commandBuffer = commandList.GetVulkanCommandBuffer();
	TransitionCurrentBackBuffer(commandBuffer, ResourceState::RenderTarget);

	VkClearValue nativeClearValue = {};
	if (clear && clearColor != nullptr)
	{
		nativeClearValue.color.float32[0] = clearColor[0];
		nativeClearValue.color.float32[1] = clearColor[1];
		nativeClearValue.color.float32[2] = clearColor[2];
		nativeClearValue.color.float32[3] = clearColor[3];
	}

	const RhiRect scissorRect = GetBackBufferScissorRect();
	const VkRenderingAttachmentInfo colorAttachment{
	    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
	    .pNext = nullptr,
	    .imageView = backBufferView,
	    .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	    .resolveMode = VK_RESOLVE_MODE_NONE,
	    .resolveImageView = VK_NULL_HANDLE,
	    .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	    .loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
	    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
	    .clearValue = nativeClearValue};

	const VkRenderingInfo renderingInfo{
	    .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .renderArea =
	        VkRect2D{
	            .offset = VkOffset2D{.x = scissorRect.Left, .y = scissorRect.Top},
	            .extent =
	                VkExtent2D{
	                    .width = static_cast<std::uint32_t>(scissorRect.Right - scissorRect.Left),
	                    .height = static_cast<std::uint32_t>(scissorRect.Bottom - scissorRect.Top)}},
	    .layerCount = 1,
	    .viewMask = 0,
	    .colorAttachmentCount = 1,
	    .pColorAttachments = &colorAttachment,
	    .pDepthAttachment = nullptr,
	    .pStencilAttachment = nullptr};

	vkCmdBeginRendering(commandBuffer, &renderingInfo);
}

void VulkanRenderHardwareInterface::EndCurrentBackBufferRendering() noexcept
{
	if (m_commandRecordingContext == nullptr)
	{
		Diagnostics::Fatal(
		    g_vulkanRenderHardwareInterfaceLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan present rendering ended before command recording was initialized.");
	}

	const auto& commandList = static_cast<const VulkanRenderCommandList&>(
	    m_commandRecordingContext->GetCurrentCommandList(ERhiQueueType::Graphics, m_currentFrameIndex));
	const VkCommandBuffer commandBuffer = commandList.GetVulkanCommandBuffer();
	vkCmdEndRendering(commandBuffer);
	TransitionCurrentBackBuffer(commandBuffer, ResourceState::Present);
}

void VulkanRenderHardwareInterface::PrepareCurrentBackBufferForPresentation(VulkanRenderCommandList& commandList) noexcept
{
	const RhiResourceHandle backBuffer = m_swapChain->GetCurrentBackBufferResource();
	const std::span<const RhiResourceHandle> trackedResources = commandList.GetTrackedResources();
	const bool transitionedByCommandList = std::any_of(
	    trackedResources.begin(),
	    trackedResources.end(),
	    [backBuffer](RhiResourceHandle resource) { return resource.Value == backBuffer.Value; });
	if (!transitionedByCommandList)
	{
		TransitionCurrentBackBuffer(commandList.GetVulkanCommandBuffer(), ResourceState::Present);
		return;
	}

	const std::uint32_t backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
	if (backBufferIndex < m_swapChainBackBufferLayouts.size())
	{
		m_swapChainBackBufferLayouts[backBufferIndex] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		return;
	}
	Diagnostics::Fatal(
	    g_vulkanRenderHardwareInterfaceLogger,
	    __FILE__,
	    __LINE__,
	    "Vulkan present preparation addressed an invalid swap-chain image index.");
}

void VulkanRenderHardwareInterface::TransitionCurrentBackBuffer(VkCommandBuffer commandBuffer, ResourceState newState) noexcept
{
	if (commandBuffer == VK_NULL_HANDLE)
	{
		Diagnostics::Fatal(
		    g_vulkanRenderHardwareInterfaceLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan back-buffer transition has no command buffer.");
	}

	const std::uint32_t backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
	if (backBufferIndex >= m_swapChainBackBufferLayouts.size())
	{
		Diagnostics::Fatal(
		    g_vulkanRenderHardwareInterfaceLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan back-buffer transition addressed an invalid swap-chain image index.");
	}

	const VulkanResourceStateMapping destinationState = VulkanTypeConversions::ToResourceStateMapping(newState);
	const VkImageLayout newLayout = destinationState.ImageLayout;
	VkImageLayout& currentLayout = m_swapChainBackBufferLayouts[backBufferIndex];
	if (currentLayout == newLayout)
	{
		return;
	}

	const ResourceState currentState = currentLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ? ResourceState::Present
	    : currentLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL                     ? ResourceState::RenderTarget
	                                                                                    : ResourceState::Common;
	const VulkanResourceStateMapping sourceState = currentLayout == VK_IMAGE_LAYOUT_UNDEFINED
	    ? VulkanResourceStateMapping{}
	    : VulkanTypeConversions::ToResourceStateMapping(currentState);
	const VkImageMemoryBarrier2 imageBarrier{
	    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
	    .pNext = nullptr,
	    .srcStageMask = sourceState.StageMask,
	    .srcAccessMask = sourceState.AccessMask,
	    .dstStageMask = destinationState.StageMask,
	    .dstAccessMask = destinationState.AccessMask,
	    .oldLayout = currentLayout,
	    .newLayout = newLayout,
	    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	    .image = m_swapChain->GetCurrentBackBufferImage(),
	    .subresourceRange = VkImageSubresourceRange{
	        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	        .baseMipLevel = 0,
	        .levelCount = 1,
	        .baseArrayLayer = 0,
	        .layerCount = 1}};

	const VkDependencyInfo dependencyInfo{
	    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
	    .pNext = nullptr,
	    .dependencyFlags = 0,
	    .memoryBarrierCount = 0,
	    .pMemoryBarriers = nullptr,
	    .bufferMemoryBarrierCount = 0,
	    .pBufferMemoryBarriers = nullptr,
	    .imageMemoryBarrierCount = 1,
	    .pImageMemoryBarriers = &imageBarrier};
	vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
	currentLayout = newLayout;
}

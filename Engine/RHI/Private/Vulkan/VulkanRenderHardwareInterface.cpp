#include "Vulkan/VulkanPCH.h"

#include "Vulkan/VulkanRenderHardwareInterface.h"

#include "Frame/RhiFrameConstants.h"
#include "Resources/Texture.h"
#include "Shaders/CookedShaderPackage.h"
#include "Vulkan/Capture/VulkanCaptureService.h"
#include "Vulkan/Commands/VulkanCommandContext.h"
#include "Vulkan/Commands/VulkanRenderCommandList.h"
#include "Vulkan/Core/VulkanResult.h"
#include "Vulkan/Descriptors/VulkanDescriptorAllocator.h"
#include "Vulkan/Descriptors/VulkanDescriptorManager.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/Device/VulkanExternalFeatureInteropCapabilities.h"
#include "Vulkan/Diagnostics/VulkanRenderDiagnostics.h"
#include "Vulkan/Interop/VulkanInteropService.h"
#include "Vulkan/Memory/VulkanGpuMemoryAllocator.h"
#include "Vulkan/Pipeline/VulkanPipelineService.h"
#include "Vulkan/Presentation/VulkanPresentationService.h"
#include "Vulkan/RayTracing/VulkanRayTracingServices.h"
#include "Vulkan/Resources/VulkanConstantBufferManager.h"
#include "Vulkan/Resources/VulkanResourceService.h"
#include "Vulkan/Samplers/VulkanSamplerLibrary.h"
#include "Vulkan/SwapChain/VulkanSwapChain.h"
#include "Vulkan/UI/VulkanImGuiBackend.h"
#include "Vulkan/VulkanTypeConversions.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <format>
#include <vector>

static const auto g_vulkanRenderHardwareInterfaceLogger = Logging::GetOrCreateLogger("RHI.Vulkan.Interface");

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

	bool WriteRgbaFloatDiagnosticBmp(
	    const std::filesystem::path& outputPath,
	    const std::byte* sourcePixels,
	    std::uint32_t width,
	    std::uint32_t height) noexcept
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
			for (std::uint32_t x = 0; x < width; ++x)
			{
				const float* rgba = reinterpret_cast<const float*>(
				    sourcePixels + (static_cast<std::size_t>(y) * width + x) * sizeof(float) * 4u);
				std::byte* outputPixel = outputPixels.data() + (static_cast<std::size_t>(y) * width + x) * 4u;
				outputPixel[0] = ToByte(rgba[2]);
				outputPixel[1] = ToByte(rgba[1]);
				outputPixel[2] = ToByte(rgba[0]);
				outputPixel[3] = ToByte(rgba[3]);
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

	std::uint32_t FindVulkanMemoryType(
	    VkPhysicalDevice physicalDevice,
	    std::uint32_t typeBits,
	    VkMemoryPropertyFlags requiredFlags) noexcept
	{
		VkPhysicalDeviceMemoryProperties memoryProperties{};
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
		for (std::uint32_t index = 0; index < memoryProperties.memoryTypeCount; ++index)
		{
			const bool typeAvailable = (typeBits & (1u << index)) != 0;
			const bool flagsMatch = (memoryProperties.memoryTypes[index].propertyFlags & requiredFlags) == requiredFlags;
			if (typeAvailable && flagsMatch)
			{
				return index;
			}
		}
		return UINT32_MAX;
	}

}

VulkanRenderHardwareInterface::VulkanRenderHardwareInterface(
    VulkanRhi& rhi,
    VulkanSwapChain& swapChain,
    VulkanCommandContext& commandContext,
    VulkanGpuMemoryAllocator& memoryAllocator) noexcept :
    m_rhi(&rhi), m_swapChain(&swapChain), m_commandContext(&commandContext)
{
	m_interopService = std::make_unique<VulkanInteropService>(*this);
	m_captureService = std::make_unique<VulkanCaptureService>(*this);
	m_presentationService = std::make_unique<VulkanPresentationService>(*this);
	m_pipelineService = std::make_unique<VulkanPipelineService>(rhi);
	m_rayTracingServices = std::make_unique<VulkanRayTracingServices>(rhi, memoryAllocator);
	m_descriptorManager = std::make_unique<VulkanDescriptorManager>(rhi, memoryAllocator, m_capabilities);
	m_resourceService = std::make_unique<VulkanResourceService>(rhi, commandContext, memoryAllocator, *m_descriptorManager, m_capabilities);
	m_constantBufferManager = std::make_unique<VulkanConstantBufferManager>(memoryAllocator);
	m_samplerLibrary = std::make_unique<VulkanSamplerLibrary>(rhi, *m_descriptorManager);
	m_descriptorManager->SetSamplerLibrary(*m_samplerLibrary);
	m_imguiBackend = std::make_unique<VulkanImGuiBackend>(*this);
	for (std::uint32_t frameIndex = 0; frameIndex < RhiFrameConstants::FramesInFlight; ++frameIndex)
	{
		commandContext.GetCommandList(frameIndex).SetRhi(&rhi);
		commandContext.GetCommandList(frameIndex).SetMemoryAllocator(&memoryAllocator);
		commandContext.GetCommandList(frameIndex).SetDescriptorAllocator(&m_descriptorManager->GetAllocator());
	}
	m_diagnostics = CreateVulkanRenderDiagnostics(rhi, memoryAllocator);
	RebuildSwapChainBackBufferViews();
	m_capabilities = BuildCapabilities();
}

VulkanRenderHardwareInterface::~VulkanRenderHardwareInterface() noexcept
{
	m_samplerLibrary.reset();
	m_imguiBackend.reset();
	m_resourceService.reset();
	m_constantBufferManager.reset();
}

ERhiBackendApi VulkanRenderHardwareInterface::GetBackendApi() const noexcept
{
	return ERhiBackendApi::Vulkan;
}

CookedShaderBinaryFormat VulkanRenderHardwareInterface::GetRequiredShaderBinaryFormat() const noexcept
{
	return CookedShaderBinaryFormat::SpirV;
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
	return *m_descriptorManager;
}

const RhiDescriptorService& VulkanRenderHardwareInterface::GetDescriptorService() const noexcept
{
	return *m_descriptorManager;
}

RhiPipelineService& VulkanRenderHardwareInterface::GetPipelineService() noexcept
{
	return *m_pipelineService;
}

RhiUploadService& VulkanRenderHardwareInterface::GetUploadService() noexcept
{
	return *m_constantBufferManager;
}

const RhiUploadService& VulkanRenderHardwareInterface::GetUploadService() const noexcept
{
	return *m_constantBufferManager;
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
	if (m_commandContext != nullptr)
	{
		m_commandContext->WaitForIdle();
	}
	if (m_rhi != nullptr)
	{
		m_rhi->WaitForIdle();
	}
	if (m_resourceService != nullptr)
	{
		m_resourceService->FlushDeferredResourceReleases();
	}
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
	return NativeGraphicsDeviceHandle{m_rhi != nullptr ? m_rhi->GetDevice() : nullptr};
}

NativeGraphicsQueueHandle VulkanRenderHardwareInterface::GetGraphicsQueueHandle() const noexcept
{
	return NativeGraphicsQueueHandle{m_rhi != nullptr ? m_rhi->GetGraphicsQueue() : nullptr};
}

bool VulkanRenderHardwareInterface::UpgradePresentationInterface(RhiNativeInterfaceUpgradeCallback, void*) noexcept
{
	return false;
}

bool VulkanRenderHardwareInterface::CaptureTextureToBmp(
    NativeResourceHandle resource,
    std::uint32_t width,
    std::uint32_t height,
    const std::filesystem::path& outputPath) noexcept
{
	if (m_rhi == nullptr || resource.Value == nullptr || width == 0 || height == 0)
	{
		return false;
	}

	const VkDevice device = m_rhi->GetDevice();
	const VkPhysicalDevice physicalDevice = m_rhi->GetPhysicalDevice();
	const VkQueue queue = m_rhi->GetGraphicsQueue();
	const std::uint32_t queueFamilyIndex = m_rhi->GetGraphicsQueueFamilyIndex();
	const VkImage sourceImage = static_cast<VkImage>(resource.Value);
	if (device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE || queue == VK_NULL_HANDLE || sourceImage == VK_NULL_HANDLE)
	{
		return false;
	}

	const VkDeviceSize bytesPerPixel = sizeof(float) * 4u;
	const VkDeviceSize readbackSize = static_cast<VkDeviceSize>(width) * height * bytesPerPixel;
	const VkBufferCreateInfo bufferInfo{
	    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .size = readbackSize,
	    .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	    .queueFamilyIndexCount = 0,
	    .pQueueFamilyIndices = nullptr};

	VkBuffer readbackBuffer = VK_NULL_HANDLE;
	if (vkCreateBuffer(device, &bufferInfo, nullptr, &readbackBuffer) != VK_SUCCESS || readbackBuffer == VK_NULL_HANDLE)
	{
		return false;
	}

	VkMemoryRequirements memoryRequirements{};
	vkGetBufferMemoryRequirements(device, readbackBuffer, &memoryRequirements);
	const std::uint32_t memoryTypeIndex = FindVulkanMemoryType(
	    physicalDevice,
	    memoryRequirements.memoryTypeBits,
	    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	if (memoryTypeIndex == UINT32_MAX)
	{
		vkDestroyBuffer(device, readbackBuffer, nullptr);
		return false;
	}

	const VkMemoryAllocateInfo allocateInfo{
	    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
	    .pNext = nullptr,
	    .allocationSize = memoryRequirements.size,
	    .memoryTypeIndex = memoryTypeIndex};
	VkDeviceMemory readbackMemory = VK_NULL_HANDLE;
	if (vkAllocateMemory(device, &allocateInfo, nullptr, &readbackMemory) != VK_SUCCESS || readbackMemory == VK_NULL_HANDLE ||
	    vkBindBufferMemory(device, readbackBuffer, readbackMemory, 0) != VK_SUCCESS)
	{
		if (readbackMemory != VK_NULL_HANDLE)
		{
			vkFreeMemory(device, readbackMemory, nullptr);
		}
		vkDestroyBuffer(device, readbackBuffer, nullptr);
		return false;
	}

	const VkCommandPoolCreateInfo poolInfo{
	    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
	    .queueFamilyIndex = queueFamilyIndex};
	VkCommandPool commandPool = VK_NULL_HANDLE;
	if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS || commandPool == VK_NULL_HANDLE)
	{
		vkFreeMemory(device, readbackMemory, nullptr);
		vkDestroyBuffer(device, readbackBuffer, nullptr);
		return false;
	}

	const VkCommandBufferAllocateInfo commandBufferInfo{
	    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
	    .pNext = nullptr,
	    .commandPool = commandPool,
	    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
	    .commandBufferCount = 1};
	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	if (vkAllocateCommandBuffers(device, &commandBufferInfo, &commandBuffer) != VK_SUCCESS || commandBuffer == VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(device, commandPool, nullptr);
		vkFreeMemory(device, readbackMemory, nullptr);
		vkDestroyBuffer(device, readbackBuffer, nullptr);
		return false;
	}

	const VkCommandBufferBeginInfo beginInfo{
	    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	    .pNext = nullptr,
	    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	    .pInheritanceInfo = nullptr};
	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
	{
		vkDestroyCommandPool(device, commandPool, nullptr);
		vkFreeMemory(device, readbackMemory, nullptr);
		vkDestroyBuffer(device, readbackBuffer, nullptr);
		return false;
	}

	const VkImageMemoryBarrier2 toTransferSource{
	    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
	    .pNext = nullptr,
	    .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
	    .srcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
	    .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
	    .dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
	    .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
	    .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	    .image = sourceImage,
	    .subresourceRange = VkImageSubresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}};
	const VkDependencyInfo toTransferDependency{
	    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
	    .pNext = nullptr,
	    .dependencyFlags = 0,
	    .memoryBarrierCount = 0,
	    .pMemoryBarriers = nullptr,
	    .bufferMemoryBarrierCount = 0,
	    .pBufferMemoryBarriers = nullptr,
	    .imageMemoryBarrierCount = 1,
	    .pImageMemoryBarriers = &toTransferSource};
	vkCmdPipelineBarrier2(commandBuffer, &toTransferDependency);

	const VkBufferImageCopy copyRegion{
	    .bufferOffset = 0,
	    .bufferRowLength = 0,
	    .bufferImageHeight = 0,
	    .imageSubresource = VkImageSubresourceLayers{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1},
	    .imageOffset = VkOffset3D{.x = 0, .y = 0, .z = 0},
	    .imageExtent = VkExtent3D{.width = width, .height = height, .depth = 1}};
	vkCmdCopyImageToBuffer(commandBuffer, sourceImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readbackBuffer, 1, &copyRegion);

	VkImageMemoryBarrier2 toGeneral = toTransferSource;
	toGeneral.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
	toGeneral.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
	toGeneral.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	toGeneral.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
	toGeneral.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	toGeneral.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	const VkDependencyInfo toGeneralDependency{
	    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
	    .pNext = nullptr,
	    .dependencyFlags = 0,
	    .memoryBarrierCount = 0,
	    .pMemoryBarriers = nullptr,
	    .bufferMemoryBarrierCount = 0,
	    .pBufferMemoryBarriers = nullptr,
	    .imageMemoryBarrierCount = 1,
	    .pImageMemoryBarriers = &toGeneral};
	vkCmdPipelineBarrier2(commandBuffer, &toGeneralDependency);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
	{
		vkDestroyCommandPool(device, commandPool, nullptr);
		vkFreeMemory(device, readbackMemory, nullptr);
		vkDestroyBuffer(device, readbackBuffer, nullptr);
		return false;
	}

	const VkSubmitInfo submitInfo{
	    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
	    .pNext = nullptr,
	    .waitSemaphoreCount = 0,
	    .pWaitSemaphores = nullptr,
	    .pWaitDstStageMask = nullptr,
	    .commandBufferCount = 1,
	    .pCommandBuffers = &commandBuffer,
	    .signalSemaphoreCount = 0,
	    .pSignalSemaphores = nullptr};
	VkFence fence = VK_NULL_HANDLE;
	const VkFenceCreateInfo fenceInfo{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
	if (vkCreateFence(device, &fenceInfo, nullptr, &fence) != VK_SUCCESS || vkQueueSubmit(queue, 1, &submitInfo, fence) != VK_SUCCESS ||
	    vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX) != VK_SUCCESS)
	{
		if (fence != VK_NULL_HANDLE)
		{
			vkDestroyFence(device, fence, nullptr);
		}
		vkDestroyCommandPool(device, commandPool, nullptr);
		vkFreeMemory(device, readbackMemory, nullptr);
		vkDestroyBuffer(device, readbackBuffer, nullptr);
		return false;
	}
	vkDestroyFence(device, fence, nullptr);

	void* mappedData = nullptr;
	const bool mapped = vkMapMemory(device, readbackMemory, 0, readbackSize, 0, &mappedData) == VK_SUCCESS && mappedData != nullptr;
	const bool wroteCapture = mapped ? WriteRgbaFloatDiagnosticBmp(outputPath, static_cast<const std::byte*>(mappedData), width, height) : false;
	if (mapped)
	{
		vkUnmapMemory(device, readbackMemory);
	}

	vkDestroyCommandPool(device, commandPool, nullptr);
	vkFreeMemory(device, readbackMemory, nullptr);
	vkDestroyBuffer(device, readbackBuffer, nullptr);
	return wroteCapture;
}

RenderCommandList& VulkanRenderHardwareInterface::GetGraphicsCommandList(std::uint32_t) noexcept
{
	if (m_resourceService != nullptr)
	{
		m_resourceService->DrainCompletedResourceReleases();
	}
	return m_commandContext->GetCommandList(m_currentFrameIndex);
}

RhiRayTracingCapabilities VulkanRenderHardwareInterface::GetRayTracingCapabilities() const noexcept
{
	return m_rayTracingServices != nullptr ? m_rayTracingServices->GetCapabilities() : RhiRayTracingCapabilities{};
}

RhiCapabilities VulkanRenderHardwareInterface::BuildCapabilities() const noexcept
{
	VkPhysicalDeviceProperties properties{};
	if (m_rhi != nullptr && m_rhi->GetPhysicalDevice() != VK_NULL_HANDLE)
	{
		vkGetPhysicalDeviceProperties(m_rhi->GetPhysicalDevice(), &properties);
	}

	RhiCapabilities capabilities{};
	capabilities.BackendApi = ERhiBackendApi::Vulkan;
	capabilities.RequiredShaderBinaryFormat = CookedShaderBinaryFormat::SpirV;
	const std::uint32_t apiVersion = m_rhi != nullptr ? m_rhi->GetAdapterInfo().ApiVersion : 0u;
	capabilities.BackendVersion = RhiBackendVersionInfo{
	    .Semantic = ERhiBackendVersionSemantic::ApiVersion,
	    .Major = VK_VERSION_MAJOR(apiVersion),
	    .Minor = VK_VERSION_MINOR(apiVersion),
	    .Patch = VK_VERSION_PATCH(apiVersion),
	    .PackedValue = apiVersion};
	capabilities.DescriptorModel = ERhiDescriptorModel::DescriptorSets;
	capabilities.BindingLimits = RhiBindingLimits{
	    .MaxDescriptorSets = properties.limits.maxBoundDescriptorSets,
	    .MaxShaderResourceDescriptors = properties.limits.maxDescriptorSetSampledImages + properties.limits.maxDescriptorSetStorageImages +
	                                     properties.limits.maxDescriptorSetUniformBuffers + properties.limits.maxDescriptorSetStorageBuffers,
	    .MaxSamplerDescriptors = properties.limits.maxDescriptorSetSamplers,
	    .MaxDescriptorTableEntries = properties.limits.maxDescriptorSetSampledImages + properties.limits.maxDescriptorSetStorageImages,
	    .MaxPushConstantBytes = properties.limits.maxPushConstantsSize};
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
	capabilities.MemoryAllocator = ERhiMemoryAllocatorBackend::VulkanManaged;
	capabilities.MemorySupport = BuildBackendMemorySupport(m_diagnostics.get());
	capabilities.ExternalFeatureInterop = BuildVulkanExternalFeatureInteropCapabilities(m_rhi, m_commandContext != nullptr);
	return capabilities;
}

RhiFormatSupport VulkanRenderHardwareInterface::QueryFormatSupport(PixelFormat format) const noexcept
{
	RhiFormatSupport support{.Format = format};
	if (m_rhi == nullptr || m_rhi->GetPhysicalDevice() == VK_NULL_HANDLE || format == PixelFormat::Unknown)
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
	support.SupportsTexture = (optimal & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != 0 ||
	                          (optimal & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) != 0 ||
	                          (optimal & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0;
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

std::unique_ptr<RenderBindingSet> VulkanRenderHardwareInterface::CreateBindingSet(const RenderBindingSetDesc& desc)
{
	return m_descriptorManager != nullptr ? m_descriptorManager->CreateBindingSet(desc) : std::unique_ptr<RenderBindingSet>{};
}

std::unique_ptr<RenderBindingLayout> VulkanRenderHardwareInterface::CreateBindingLayout(const RenderBindingLayoutCompileDesc& desc)
{
	return m_pipelineService != nullptr ? m_pipelineService->CreateBindingLayout(desc) : std::unique_ptr<RenderBindingLayout>{};
}

std::unique_ptr<RenderPipelineState> VulkanRenderHardwareInterface::CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc)
{
	return m_pipelineService != nullptr ? m_pipelineService->CreateGraphicsPipelineState(desc) : std::unique_ptr<RenderPipelineState>{};
}

std::unique_ptr<RenderPipelineState> VulkanRenderHardwareInterface::CreateComputePipelineState(const ComputePipelineStateDesc& desc)
{
	return m_pipelineService != nullptr ? m_pipelineService->CreateComputePipelineState(desc) : std::unique_ptr<RenderPipelineState>{};
}

void VulkanRenderHardwareInterface::BindGlobalDescriptorState(RenderCommandList&) const noexcept {}

RhiDescriptorAllocation VulkanRenderHardwareInterface::AllocateDescriptor(ERhiDescriptorAllocatorType descriptorType)
{
	return m_descriptorManager != nullptr ? m_descriptorManager->AllocateDescriptor(descriptorType) : RhiDescriptorAllocation{};
}

void VulkanRenderHardwareInterface::ReleaseDescriptor(
    ERhiDescriptorAllocatorType descriptorType,
    const RhiDescriptorAllocation& allocation) noexcept
{
	if (m_descriptorManager != nullptr)
	{
		m_descriptorManager->ReleaseDescriptor(descriptorType, allocation);
	}
}

RhiDescriptorTableHandle VulkanRenderHardwareInterface::AllocateDescriptorTable(
    ERhiDescriptorAllocatorType descriptorType,
    std::uint32_t descriptorCount)
{
	return m_descriptorManager != nullptr ? m_descriptorManager->AllocateDescriptorTable(descriptorType, descriptorCount)
	                                        : RhiDescriptorTableHandle{};
}

RhiCpuDescriptorHandle VulkanRenderHardwareInterface::GetDescriptorTableCpuHandle(
    RhiDescriptorTableHandle tableHandle,
    std::uint32_t descriptorIndex) const noexcept
{
	return m_descriptorManager != nullptr ? m_descriptorManager->GetDescriptorTableCpuHandle(tableHandle, descriptorIndex)
	                                        : RhiCpuDescriptorHandle{};
}

void VulkanRenderHardwareInterface::ReleaseDescriptorTable(RhiDescriptorTableHandle tableHandle) noexcept
{
	if (m_descriptorManager != nullptr)
	{
		m_descriptorManager->ReleaseDescriptorTable(tableHandle);
	}
}

void VulkanRenderHardwareInterface::AllocateShaderResourceDescriptor(
    RhiCpuDescriptorHandle& outCpuHandle,
    RhiGpuDescriptorHandle& outGpuHandle)
{
	const RhiDescriptorAllocation allocation = AllocateDescriptor(ERhiDescriptorAllocatorType::ShaderResource);
	outCpuHandle = allocation.CpuHandle;
	outGpuHandle = allocation.GpuHandle;
}

void VulkanRenderHardwareInterface::ReleaseShaderResourceDescriptor(
    RhiCpuDescriptorHandle cpuHandle,
    RhiGpuDescriptorHandle gpuHandle) noexcept
{
	ReleaseDescriptor(ERhiDescriptorAllocatorType::ShaderResource, RhiDescriptorAllocation{.CpuHandle = cpuHandle, .GpuHandle = gpuHandle});
}

RhiGpuVirtualAddress VulkanRenderHardwareInterface::AllocateUniformConstantBuffer(const void* data, std::uint32_t sizeInBytes)
{
	return m_constantBufferManager != nullptr ? m_constantBufferManager->AllocateUniform(data, sizeInBytes) : RhiGpuVirtualAddress{};
}

RhiDescriptorTableBinding VulkanRenderHardwareInterface::GetSharedSamplerBinding(const RhiSamplerDesc& samplerDesc) const noexcept
{
	return m_samplerLibrary != nullptr ? m_samplerLibrary->GetSharedSamplerBinding(samplerDesc) : RhiDescriptorTableBinding{};
}

RhiViewport VulkanRenderHardwareInterface::GetBackBufferViewport() const noexcept
{
	return m_swapChain != nullptr ? m_swapChain->GetDefaultViewport() : RhiViewport{};
}

RhiRect VulkanRenderHardwareInterface::GetBackBufferScissorRect() const noexcept
{
	return m_swapChain != nullptr ? m_swapChain->GetDefaultScissorRect() : RhiRect{};
}

RhiCpuDescriptorHandle VulkanRenderHardwareInterface::GetBackBufferRenderTargetView() const noexcept
{
	return GetResourceViewCpuHandle(GetCurrentBackBufferViewHandle());
}

NativeResourceHandle VulkanRenderHardwareInterface::GetBackBufferResource() const noexcept
{
	return m_swapChain != nullptr ? m_swapChain->GetCurrentBackBufferResource() : NativeResourceHandle{};
}

std::unique_ptr<Texture> VulkanRenderHardwareInterface::CreateTexture(RhiTextureUploadDesc textureUpload, std::wstring_view debugName)
{
	return m_resourceService != nullptr ? m_resourceService->CreateTexture(std::move(textureUpload), debugName) :
	                                      std::unique_ptr<Texture>{};
}

RhiOwnedResourceHandle VulkanRenderHardwareInterface::CreateTextureResource(
    const RhiTextureResourceDesc& desc,
    ResourceState initialState,
    RhiMemoryCategory category,
    RhiMemoryResidencyClass residencyClass,
    std::wstring_view debugName)
{
	return m_resourceService != nullptr ? m_resourceService->CreateTextureResource(desc, initialState, category, residencyClass, debugName) :
	                                      RhiOwnedResourceHandle{};
}

RhiOwnedResourceHandle VulkanRenderHardwareInterface::CreateBufferResource(
    const RhiBufferResourceDesc& desc,
    ResourceState initialState,
    RhiMemoryCategory category,
    RhiMemoryResidencyClass residencyClass,
    std::wstring_view debugName)
{
	return m_resourceService != nullptr ? m_resourceService->CreateBufferResource(desc, initialState, category, residencyClass, debugName) :
	                                      RhiOwnedResourceHandle{};
}

bool VulkanRenderHardwareInterface::CreateVertexBuffer(
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

bool VulkanRenderHardwareInterface::CreateStructuredBuffer(
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

bool VulkanRenderHardwareInterface::CreateIndexBuffer(
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

void VulkanRenderHardwareInterface::ReleaseOwnedResource(RhiOwnedResourceHandle resource) noexcept
{
	if (m_resourceService != nullptr)
	{
		m_resourceService->ReleaseOwnedResource(resource);
	}
}

NativeResourceHandle VulkanRenderHardwareInterface::GetNativeResource(RhiOwnedResourceHandle resource) const noexcept
{
	return m_resourceService != nullptr ? m_resourceService->GetNativeResource(resource) : NativeResourceHandle{};
}

RhiGpuVirtualAddress VulkanRenderHardwareInterface::GetResourceGpuVirtualAddress(RhiOwnedResourceHandle resource) const noexcept
{
	return m_resourceService != nullptr ? m_resourceService->GetResourceGpuVirtualAddress(resource) : 0;
}

RhiRayTracingAccelerationStructurePrebuildInfo VulkanRenderHardwareInterface::GetBottomLevelAccelerationStructurePrebuildInfo(
    const RhiRayTracingGeometryDesc& geometry) const noexcept
{
	return m_rayTracingServices != nullptr ? m_rayTracingServices->GetBottomLevelAccelerationStructurePrebuildInfo(geometry) :
	                                        RhiRayTracingAccelerationStructurePrebuildInfo{};
}

RhiRayTracingAccelerationStructurePrebuildInfo VulkanRenderHardwareInterface::GetTopLevelAccelerationStructurePrebuildInfo(
    std::uint32_t instanceCount,
    ERhiClassicTlasBuildFlags buildFlags) const noexcept
{
	return m_rayTracingServices != nullptr ? m_rayTracingServices->GetTopLevelAccelerationStructurePrebuildInfo(instanceCount, buildFlags) :
	                                        RhiRayTracingAccelerationStructurePrebuildInfo{};
}

RhiOwnedResourceHandle VulkanRenderHardwareInterface::CreateRayTracingScratchBuffer(std::uint64_t sizeInBytes, std::wstring_view debugName)
{
	return m_rayTracingServices != nullptr ? m_rayTracingServices->CreateScratchBuffer(sizeInBytes, debugName) : RhiOwnedResourceHandle{};
}

RhiOwnedResourceHandle VulkanRenderHardwareInterface::CreateRayTracingAccelerationStructureBuffer(
    std::uint64_t sizeInBytes,
    ERhiRayTracingAccelerationStructureType type,
    std::wstring_view debugName)
{
	return m_rayTracingServices != nullptr ? m_rayTracingServices->CreateAccelerationStructureBuffer(sizeInBytes, type, debugName) :
	                                        RhiOwnedResourceHandle{};
}

RhiOwnedResourceHandle VulkanRenderHardwareInterface::CreateRayTracingInstanceBuffer(
    const RhiRayTracingInstanceDesc* instances,
    std::uint32_t instanceCount,
    std::wstring_view debugName)
{
	return m_rayTracingServices != nullptr ? m_rayTracingServices->CreateInstanceBuffer(instances, instanceCount, debugName) :
	                                        RhiOwnedResourceHandle{};
}

RhiResourceAllocationInfo VulkanRenderHardwareInterface::GetTextureAllocationInfo(const RhiTextureResourceDesc& desc) const noexcept
{
	return m_resourceService != nullptr ? m_resourceService->GetTextureAllocationInfo(desc) : RhiResourceAllocationInfo{};
}

RhiResourceAllocationInfo VulkanRenderHardwareInterface::GetBufferAllocationInfo(const RhiBufferResourceDesc& desc) const noexcept
{
	return m_resourceService != nullptr ? m_resourceService->GetBufferAllocationInfo(desc) : RhiResourceAllocationInfo{};
}

RhiOwnedMemoryBlockHandle VulkanRenderHardwareInterface::CreateTransientMemoryBlock(
    RhiTransientAllocationPool pool,
    std::uint64_t sizeInBytes,
    std::uint64_t alignment,
    std::wstring_view debugName)
{
	return m_resourceService != nullptr ? m_resourceService->CreateTransientMemoryBlock(pool, sizeInBytes, alignment, debugName) :
	                                      RhiOwnedMemoryBlockHandle{};
}

void VulkanRenderHardwareInterface::ReleaseTransientMemoryBlock(RhiOwnedMemoryBlockHandle memoryBlock) noexcept
{
	if (m_resourceService != nullptr)
	{
		m_resourceService->ReleaseTransientMemoryBlock(memoryBlock);
	}
}

RhiOwnedResourceHandle VulkanRenderHardwareInterface::CreateAliasingTextureResource(
    RhiOwnedMemoryBlockHandle memoryBlock,
    std::uint64_t memoryBlockOffset,
    const RhiTransientTextureAllocationDesc& desc,
    std::wstring_view debugName)
{
	return m_resourceService != nullptr ?
	           m_resourceService->CreateAliasingTextureResource(memoryBlock, memoryBlockOffset, desc, debugName) :
	           RhiOwnedResourceHandle{};
}

RhiOwnedResourceHandle VulkanRenderHardwareInterface::CreateAliasingBufferResource(
    RhiOwnedMemoryBlockHandle memoryBlock,
    std::uint64_t memoryBlockOffset,
    const RhiTransientBufferAllocationDesc& desc,
    std::wstring_view debugName)
{
	return m_resourceService != nullptr ?
	           m_resourceService->CreateAliasingBufferResource(memoryBlock, memoryBlockOffset, desc, debugName) :
	           RhiOwnedResourceHandle{};
}

RhiResourceViewHandle VulkanRenderHardwareInterface::CreateResourceView(const RhiResourceViewDesc& desc)
{
	return m_descriptorManager != nullptr ? m_descriptorManager->CreateResourceView(desc) : RhiResourceViewHandle{};
}

void VulkanRenderHardwareInterface::ReleaseResourceView(RhiResourceViewHandle view) noexcept
{
	if (m_descriptorManager != nullptr)
	{
		m_descriptorManager->ReleaseResourceView(view);
	}
}

RhiCpuDescriptorHandle VulkanRenderHardwareInterface::GetResourceViewCpuHandle(RhiResourceViewHandle view) const noexcept
{
	return m_descriptorManager != nullptr ? m_descriptorManager->GetResourceViewCpuHandle(view) : RhiCpuDescriptorHandle{};
}

RhiGpuDescriptorHandle VulkanRenderHardwareInterface::GetResourceViewGpuHandle(RhiResourceViewHandle view) const noexcept
{
	return m_descriptorManager != nullptr ? m_descriptorManager->GetResourceViewGpuHandle(view) : RhiGpuDescriptorHandle{};
}

NativeTextureViewInfo VulkanRenderHardwareInterface::GetNativeTextureViewInfo(RhiResourceViewHandle view, ResourceState state) const noexcept
{
	return m_descriptorManager != nullptr ? m_descriptorManager->GetNativeTextureViewInfo(view, state) : NativeTextureViewInfo{};
}

std::uint64_t VulkanRenderHardwareInterface::ResolveImGuiTextureId(RhiGpuDescriptorHandle shaderResourceView) noexcept
{
	if (m_descriptorManager == nullptr || m_imguiBackend == nullptr)
	{
		return 0;
	}

	const VkImageView imageView = m_descriptorManager->GetRegisteredImageView(shaderResourceView);
	return m_imguiBackend->GetTextureId(imageView);
}

bool VulkanRenderHardwareInterface::SupportsUnorderedAccess(NativeResourceHandle resource) const noexcept
{
	return m_resourceService != nullptr && m_resourceService->SupportsUnorderedAccess(resource);
}

void VulkanRenderHardwareInterface::BeginPresentRenderPass(const float clearColor[4]) noexcept
{
	static constexpr float defaultClearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	BeginCurrentBackBufferRendering(clearColor != nullptr ? clearColor : defaultClearColor, true);
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
	return m_swapChain != nullptr ? m_swapChain->GetBackBufferFormat() : PixelFormat::Unknown;
}

VkInstance VulkanRenderHardwareInterface::GetVulkanInstance() const noexcept
{
	return m_rhi != nullptr ? m_rhi->GetInstance() : VK_NULL_HANDLE;
}

VkPhysicalDevice VulkanRenderHardwareInterface::GetVulkanPhysicalDevice() const noexcept
{
	return m_rhi != nullptr ? m_rhi->GetPhysicalDevice() : VK_NULL_HANDLE;
}

VkDevice VulkanRenderHardwareInterface::GetVulkanDevice() const noexcept
{
	return m_rhi != nullptr ? m_rhi->GetDevice() : VK_NULL_HANDLE;
}

VkQueue VulkanRenderHardwareInterface::GetVulkanGraphicsQueue() const noexcept
{
	return m_rhi != nullptr ? m_rhi->GetGraphicsQueue() : VK_NULL_HANDLE;
}

std::uint32_t VulkanRenderHardwareInterface::GetVulkanGraphicsQueueFamilyIndex() const noexcept
{
	return m_rhi != nullptr ? m_rhi->GetGraphicsQueueFamilyIndex() : UINT32_MAX;
}

std::uint32_t VulkanRenderHardwareInterface::GetVulkanApiVersion() const noexcept
{
	return m_rhi != nullptr ? m_rhi->GetAdapterInfo().ApiVersion : VK_API_VERSION_1_3;
}

std::uint32_t VulkanRenderHardwareInterface::GetSwapChainBackBufferCount() const noexcept
{
	return m_swapChain != nullptr ? m_swapChain->GetBackBufferCount() : 0;
}

VkFormat VulkanRenderHardwareInterface::GetNativeBackBufferFormat() const noexcept
{
	return m_swapChain != nullptr ? m_swapChain->GetNativeBackBufferFormat() : VK_FORMAT_UNDEFINED;
}

void VulkanRenderHardwareInterface::SetCurrentFrameIndex(std::uint32_t frameIndex) noexcept
{
	m_currentFrameIndex = frameIndex;
}

void VulkanRenderHardwareInterface::ResetTransientFrameResources() noexcept
{
	if (m_descriptorManager != nullptr)
	{
		m_descriptorManager->BeginFrame(m_currentFrameIndex);
	}
	if (m_constantBufferManager != nullptr)
	{
		m_constantBufferManager->BeginFrame(m_currentFrameIndex);
	}
}

void VulkanRenderHardwareInterface::RebuildSwapChainBackBufferViews() noexcept
{
	m_swapChainBackBufferLayouts.clear();
	m_isPresentRendering = false;
	if (m_swapChain == nullptr)
	{
		return;
	}

	const std::uint32_t backBufferCount = m_swapChain->GetBackBufferCount();
	m_swapChainBackBufferLayouts.assign(backBufferCount, VK_IMAGE_LAYOUT_UNDEFINED);
	if (m_descriptorManager != nullptr)
	{
		m_descriptorManager->RebuildSwapChainBackBufferViews(*m_swapChain);
	}
}

RhiResourceViewHandle VulkanRenderHardwareInterface::GetCurrentBackBufferViewHandle() const noexcept
{
	if (m_swapChain == nullptr)
	{
		return {};
	}

	const std::uint32_t backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
	return m_descriptorManager != nullptr ? m_descriptorManager->GetSwapChainBackBufferView(backBufferIndex) : RhiResourceViewHandle{};
}

void VulkanRenderHardwareInterface::BeginCurrentBackBufferRendering(const float* clearColor, bool clear) noexcept
{
	if (m_swapChain == nullptr || m_commandContext == nullptr || m_isPresentRendering)
	{
		return;
	}

	const VkImage backBuffer = m_swapChain->GetCurrentBackBufferImage();
	const VkImageView backBufferView = m_swapChain->GetCurrentBackBufferImageView();
	if (backBuffer == VK_NULL_HANDLE || backBufferView == VK_NULL_HANDLE)
	{
		return;
	}

	RenderCommandList& commandList = GetGraphicsCommandList(m_currentFrameIndex);
	commandList.SetViewport(GetBackBufferViewport());
	commandList.SetScissorRect(GetBackBufferScissorRect());
	commandList.SetRenderTarget(GetBackBufferRenderTargetView());

	VkCommandBuffer commandBuffer = m_commandContext->GetCommandBuffer(m_currentFrameIndex);
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
	m_isPresentRendering = true;
}

void VulkanRenderHardwareInterface::EndCurrentBackBufferRendering() noexcept
{
	if (m_commandContext == nullptr || !m_isPresentRendering)
	{
		return;
	}

	VkCommandBuffer commandBuffer = m_commandContext->GetCommandBuffer(m_currentFrameIndex);
	vkCmdEndRendering(commandBuffer);
	TransitionCurrentBackBuffer(commandBuffer, ResourceState::Present);
	m_isPresentRendering = false;
}

void VulkanRenderHardwareInterface::TransitionCurrentBackBuffer(VkCommandBuffer commandBuffer, ResourceState newState) noexcept
{
	if (m_swapChain == nullptr || commandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	const std::uint32_t backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
	if (backBufferIndex >= m_swapChainBackBufferLayouts.size())
	{
		return;
	}

	const VulkanResourceStateMapping destinationState = VulkanTypeConversions::ToResourceStateMapping(newState);
	const VkImageLayout newLayout = destinationState.ImageLayout;
	VkImageLayout& currentLayout = m_swapChainBackBufferLayouts[backBufferIndex];
	if (currentLayout == newLayout)
	{
		return;
	}

	const ResourceState currentState = currentLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR            ? ResourceState::Present
	                                   : currentLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ? ResourceState::RenderTarget
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

#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Device/VulkanRhi.h"

#include "CVars/RHICVars.h"

#include <limits>

static const auto g_vulkanRhiLogger = Logging::GetOrCreateLogger("RHI.Vulkan");

RhiRayTracingCapabilities VulkanRhi::GetRayTracingCapabilities() const noexcept
{
	return m_rayTracingCapabilities;
}

PFN_vkGetBufferDeviceAddress VulkanRhi::GetGetBufferDeviceAddress() const noexcept
{
	return m_getBufferDeviceAddress;
}

PFN_vkCreateAccelerationStructureKHR VulkanRhi::GetCreateAccelerationStructure() const noexcept
{
	return m_createAccelerationStructure;
}

PFN_vkDestroyAccelerationStructureKHR VulkanRhi::GetDestroyAccelerationStructure() const noexcept
{
	return m_destroyAccelerationStructure;
}

PFN_vkGetAccelerationStructureBuildSizesKHR VulkanRhi::GetAccelerationStructureBuildSizes() const noexcept
{
	return m_getAccelerationStructureBuildSizes;
}

PFN_vkCmdBuildAccelerationStructuresKHR VulkanRhi::GetCmdBuildAccelerationStructures() const noexcept
{
	return m_cmdBuildAccelerationStructures;
}

PFN_vkGetAccelerationStructureDeviceAddressKHR VulkanRhi::GetAccelerationStructureDeviceAddress() const noexcept
{
	return m_getAccelerationStructureDeviceAddress;
}

PFN_vkGetPartitionedAccelerationStructuresBuildSizesNV VulkanRhi::GetPartitionedAccelerationStructureBuildSizes() const noexcept
{
	return m_getPartitionedAccelerationStructureBuildSizes;
}

PFN_vkCmdBuildPartitionedAccelerationStructuresNV VulkanRhi::GetCmdBuildPartitionedAccelerationStructures() const noexcept
{
	return m_cmdBuildPartitionedAccelerationStructures;
}

PFN_vkCreateRayTracingPipelinesKHR VulkanRhi::GetCreateRayTracingPipelines() const noexcept
{
	return m_createRayTracingPipelines;
}

PFN_vkGetRayTracingShaderGroupHandlesKHR VulkanRhi::GetRayTracingShaderGroupHandles() const noexcept
{
	return m_getRayTracingShaderGroupHandles;
}

PFN_vkCmdTraceRaysKHR VulkanRhi::GetCmdTraceRays() const noexcept
{
	return m_cmdTraceRays;
}

void VulkanRhi::LoadRayTracingFunctions() noexcept
{
	if (m_device == VK_NULL_HANDLE || !m_featureStatus.RayTracing.EnabledAccelerationStructure)
	{
		return;
	}

	m_getBufferDeviceAddress = reinterpret_cast<PFN_vkGetBufferDeviceAddress>(vkGetDeviceProcAddr(m_device, "vkGetBufferDeviceAddress"));
	if (m_getBufferDeviceAddress == nullptr)
	{
		m_getBufferDeviceAddress =
		    reinterpret_cast<PFN_vkGetBufferDeviceAddress>(vkGetDeviceProcAddr(m_device, "vkGetBufferDeviceAddressKHR"));
	}
	m_createAccelerationStructure =
	    reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(m_device, "vkCreateAccelerationStructureKHR"));
	m_destroyAccelerationStructure =
	    reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(m_device, "vkDestroyAccelerationStructureKHR"));
	m_getAccelerationStructureBuildSizes = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(
	    vkGetDeviceProcAddr(m_device, "vkGetAccelerationStructureBuildSizesKHR"));
	m_cmdBuildAccelerationStructures =
	    reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(m_device, "vkCmdBuildAccelerationStructuresKHR"));
	m_getAccelerationStructureDeviceAddress = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(
	    vkGetDeviceProcAddr(m_device, "vkGetAccelerationStructureDeviceAddressKHR"));
	if (m_featureStatus.RayTracing.EnabledRayTracingPipeline)
	{
		m_createRayTracingPipelines =
		    reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(m_device, "vkCreateRayTracingPipelinesKHR"));
		m_getRayTracingShaderGroupHandles = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(
		    vkGetDeviceProcAddr(m_device, "vkGetRayTracingShaderGroupHandlesKHR"));
		m_cmdTraceRays = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(m_device, "vkCmdTraceRaysKHR"));
	}
	if (m_featureStatus.RayTracing.EnabledPartitionedAccelerationStructure)
	{
		m_getPartitionedAccelerationStructureBuildSizes = reinterpret_cast<PFN_vkGetPartitionedAccelerationStructuresBuildSizesNV>(
		    vkGetDeviceProcAddr(m_device, "vkGetPartitionedAccelerationStructuresBuildSizesNV"));
		m_cmdBuildPartitionedAccelerationStructures = reinterpret_cast<PFN_vkCmdBuildPartitionedAccelerationStructuresNV>(
		    vkGetDeviceProcAddr(m_device, "vkCmdBuildPartitionedAccelerationStructuresNV"));
	}

	const bool loaded = m_getBufferDeviceAddress != nullptr && m_createAccelerationStructure != nullptr
	    && m_destroyAccelerationStructure != nullptr && m_getAccelerationStructureBuildSizes != nullptr
	    && m_cmdBuildAccelerationStructures != nullptr && m_getAccelerationStructureDeviceAddress != nullptr;
	if (!loaded)
	{
		m_featureStatus.RayTracing.EnabledAccelerationStructure = false;
		m_featureStatus.RayTracing.EnabledInlineRayQuery = false;
		m_featureStatus.RayTracing.EnabledRayTracingPipeline = false;
		m_featureStatus.RayTracing.EnabledPartitionedAccelerationStructure = false;
	}
	if (m_createRayTracingPipelines == nullptr || m_getRayTracingShaderGroupHandles == nullptr || m_cmdTraceRays == nullptr)
	{
		m_featureStatus.RayTracing.EnabledRayTracingPipeline = false;
	}
	if (m_featureStatus.RayTracing.EnabledPartitionedAccelerationStructure
	    && (m_getPartitionedAccelerationStructureBuildSizes == nullptr || m_cmdBuildPartitionedAccelerationStructures == nullptr))
	{
		m_featureStatus.RayTracing.EnabledPartitionedAccelerationStructure = false;
	}
}

void VulkanRhi::BuildRayTracingCapabilities() noexcept
{
	m_rayTracingCapabilities = {};
	m_rayTracingCapabilities.Groups.PartitionedTlas = RhiPartitionedTlasCapabilities{
	    .Supported = false,
	    .Provider = ERhiPartitionedTlasProvider::VulkanNvPartitionedAccelerationStructure,
	    .NvidiaDeviceOnly = true,
	    .CurrentDeviceIsNvidia = m_adapterInfo.VendorId == NvidiaVendorId,
	    .SupportsDescriptorAccess = false,
	    .CapabilityStatusReason = m_adapterInfo.VendorId != NvidiaVendorId
	        ? "vulkan-nv-ptlas-requires-nvidia-device"
	        : (!m_featureStatus.RayTracing.SupportsPartitionedAccelerationStructureExtension
	                  ? "vulkan-nv-partitioned-acceleration-structure-extension-not-present"
	                  : (!m_featureStatus.RayTracing.SupportsPartitionedAccelerationStructureFeature
	                            ? "vulkan-nv-partitioned-acceleration-structure-feature-not-present"
	                            : "vulkan-ray-tracing-backend-not-enabled"))};
	m_rayTracingCapabilities.Groups.Provider = RhiRayTracingProviderCapabilities{
	    .SelectedTopLevelProvider = ERhiRayTracingTopLevelProvider::None,
	    .SelectedTopLevelProviderReason = "ray-tracing-not-enabled"};
	if (m_physicalDevice == VK_NULL_HANDLE || !m_featureStatus.RayTracing.EnabledAccelerationStructure)
	{
		return;
	}

	VkPhysicalDeviceAccelerationStructurePropertiesKHR accelerationStructureProperties{
	    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR pipelineProperties{
	    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
	VkPhysicalDeviceProperties2 properties{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
	properties.pNext = &accelerationStructureProperties;
	if (m_featureStatus.RayTracing.EnabledRayTracingPipeline)
	{
		pipelineProperties.pNext = properties.pNext;
		properties.pNext = &pipelineProperties;
	}
	vkGetPhysicalDeviceProperties2(m_physicalDevice, &properties);
	const bool pipelineReady = m_featureStatus.RayTracing.EnabledRayTracingPipeline && m_createRayTracingPipelines != nullptr
	    && m_getRayTracingShaderGroupHandles != nullptr && m_cmdTraceRays != nullptr && pipelineProperties.shaderGroupHandleSize != 0
	    && pipelineProperties.shaderGroupBaseAlignment != 0 && pipelineProperties.shaderGroupHandleAlignment != 0
	    && pipelineProperties.maxShaderGroupStride != 0;

	m_rayTracingCapabilities = RhiRayTracingCapabilities{
	    .SupportsAccelerationStructure = true,
	    .SupportsInlineRayQuery = m_featureStatus.RayTracing.EnabledInlineRayQuery,
	    .SupportsRayTracingPipeline = pipelineReady,
	    .MaxTraceRecursionDepth = pipelineReady ? pipelineProperties.maxRayRecursionDepth : 0,
	    .MaxRayPayloadSizeInBytes = pipelineReady ? kRhiRayTracingMaxPayloadSizeInBytes : 0u,
	    .MaxRayAttributeSizeInBytes = pipelineReady ? pipelineProperties.maxRayHitAttributeSize : 0,
	    .ShaderGroupHandleSizeInBytes = pipelineReady ? pipelineProperties.shaderGroupHandleSize : 0,
	    .ShaderTableAlignmentInBytes = pipelineReady ? pipelineProperties.shaderGroupBaseAlignment : 0,
	    .ShaderTableRecordAlignmentInBytes = pipelineReady ? pipelineProperties.shaderGroupHandleAlignment : 0,
	    .MaxShaderTableRecordStrideInBytes = pipelineReady ? pipelineProperties.maxShaderGroupStride : 0,
	    .AccelerationStructureByteAlignment = 256,
	    .ScratchBufferByteAlignment = accelerationStructureProperties.minAccelerationStructureScratchOffsetAlignment,
	    .InstanceDescSizeInBytes = static_cast<std::uint32_t>(sizeof(VkAccelerationStructureInstanceKHR))};
	PopulateStandardRayTracingCapabilityGroups(m_rayTracingCapabilities);
	m_rayTracingCapabilities.Groups.PartitionedTlas = RhiPartitionedTlasCapabilities{
	    .Supported = m_featureStatus.RayTracing.EnabledPartitionedAccelerationStructure
	        && m_getPartitionedAccelerationStructureBuildSizes != nullptr && m_cmdBuildPartitionedAccelerationStructures != nullptr,
	    .Provider = ERhiPartitionedTlasProvider::VulkanNvPartitionedAccelerationStructure,
	    .NvidiaDeviceOnly = true,
	    .CurrentDeviceIsNvidia = m_adapterInfo.VendorId == NvidiaVendorId,
	    .SupportsDescriptorAccess = false,
	    .SupportsCpuPackedOperations = m_featureStatus.RayTracing.EnabledPartitionedAccelerationStructure,
	    .SupportsGpuDrivenOperations = m_featureStatus.RayTracing.EnabledPartitionedAccelerationStructure,
	    .SupportsGpuOperationCount = m_featureStatus.RayTracing.EnabledPartitionedAccelerationStructure,
	    .SupportsGpuWrittenInstanceRecords = m_featureStatus.RayTracing.EnabledPartitionedAccelerationStructure,
	    .SupportsGpuWrittenPartitionRecords = m_featureStatus.RayTracing.EnabledPartitionedAccelerationStructure,
	    .SupportsPartitionTranslation = m_featureStatus.RayTracing.EnabledPartitionedAccelerationStructure,
	    .SupportsGlobalPartition = true,
	    .SupportsExplicitInstanceAabb = true,
	    .MaxOperationsPerBuild = std::numeric_limits<std::uint32_t>::max(),
	    .InstanceWriteDataSizeInBytes = static_cast<std::uint32_t>(sizeof(VkPartitionedAccelerationStructureWriteInstanceDataNV)),
	    .InstanceUpdateDataSizeInBytes = static_cast<std::uint32_t>(sizeof(VkPartitionedAccelerationStructureUpdateInstanceDataNV)),
	    .PartitionWriteDataSizeInBytes =
	        static_cast<std::uint32_t>(sizeof(VkPartitionedAccelerationStructureWritePartitionTranslationDataNV)),
	    .OperationDataSizeInBytes = static_cast<std::uint32_t>(sizeof(VkBuildPartitionedAccelerationStructureIndirectCommandNV)),
	    .OperationCountDataSizeInBytes = static_cast<std::uint32_t>(sizeof(std::uint32_t)),
	    .CapabilityStatusReason = m_adapterInfo.VendorId != NvidiaVendorId
	        ? "vulkan-nv-ptlas-requires-nvidia-device"
	        : (!m_featureStatus.RayTracing.SupportsPartitionedAccelerationStructureExtension
	                  ? "vulkan-nv-partitioned-acceleration-structure-extension-not-present"
	                  : (!m_featureStatus.RayTracing.SupportsPartitionedAccelerationStructureFeature
	                            ? "vulkan-nv-partitioned-acceleration-structure-feature-not-present"
	                            : (!m_featureStatus.RayTracing.EnabledPartitionedAccelerationStructure
	                                      ? "vulkan-nv-partitioned-acceleration-structure-not-enabled"
	                                      : (m_getPartitionedAccelerationStructureBuildSizes == nullptr
	                                                    || m_cmdBuildPartitionedAccelerationStructures == nullptr
	                                                ? "vulkan-nv-ptlas-functions-not-loaded"
	                                                : "vulkan-nv-ptlas-provider-ready"))))};
	SelectRayTracingTopLevelProvider();
}

void VulkanRhi::SelectRayTracingTopLevelProvider() noexcept
{
	RhiRayTracingProviderCapabilities& provider = m_rayTracingCapabilities.Groups.Provider;
	if (!m_rayTracingCapabilities.SupportsAccelerationStructure)
	{
		provider = RhiRayTracingProviderCapabilities{
		    .SelectedTopLevelProvider = ERhiRayTracingTopLevelProvider::None,
		    .SelectedTopLevelProviderReason = "ray-tracing-unavailable"};
		return;
	}

	const RhiPartitionedTlasCapabilities& partitionedTlas = m_rayTracingCapabilities.Groups.PartitionedTlas;
	const bool partitionedTlasRequested = CVarRayTracingPreferPartitionedTlas.Get();
	const bool partitionedTlasSelected = partitionedTlasRequested && partitionedTlas.Supported && partitionedTlas.SupportsDescriptorAccess;
	const char* selectionReason = "classic-tlas-selected";
	if (partitionedTlasSelected)
	{
		selectionReason = "vulkan-nv-ptlas-selected";
	}
	else if (partitionedTlasRequested && partitionedTlas.Supported)
	{
		selectionReason = "vulkan-ptlas-descriptor-path-unavailable";
	}

	provider = RhiRayTracingProviderCapabilities{
	    .SelectedTopLevelProvider =
	        partitionedTlasSelected ? ERhiRayTracingTopLevelProvider::PartitionedTlas : ERhiRayTracingTopLevelProvider::ClassicTlas,
	    .SelectedTopLevelProviderReason = selectionReason};
}

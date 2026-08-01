#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Device/VulkanRhi.h"

#include "CVars/RHICVars.h"
#include "Vulkan/Commands/VulkanCommandQueue.h"
#include "Vulkan/Core/VulkanResult.h"
#include "Vulkan/Diagnostics/VulkanDebugNames.h"
#include "Core/Public/Diagnostics/Verify.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <format>
#include <utility>

static const auto g_vulkanRhiLogger = Logging::GetOrCreateLogger("RHI.Vulkan");

bool VulkanRhi::QueryMutableDescriptorTypeFeature(VkPhysicalDevice physicalDevice) noexcept
{
	if (!IsDeviceExtensionAvailable(physicalDevice, VK_EXT_MUTABLE_DESCRIPTOR_TYPE_EXTENSION_NAME))
	{
		return false;
	}

	VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT mutableDescriptorFeatures{
	    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT};
	VkPhysicalDeviceFeatures2 features{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &mutableDescriptorFeatures};
	vkGetPhysicalDeviceFeatures2(physicalDevice, &features);
	return mutableDescriptorFeatures.mutableDescriptorType == VK_TRUE;
}

bool VulkanRhi::AppendAvailableDeviceExtension(
    VkPhysicalDevice physicalDevice,
    std::vector<const char*>& extensions,
    const char* extensionName) noexcept
{
	if (extensionName == nullptr || std::find_if(
	                                    extensions.begin(),
	                                    extensions.end(),
	                                    [extensionName](const char* enabled) noexcept
	                                    {
		                                    return std::strcmp(enabled, extensionName) == 0;
	                                    }) != extensions.end())
	{
		return false;
	}

	std::uint32_t extensionCount = 0;
	VkResult result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
	if (!VulkanResult::Succeeded(result))
	{
		return false;
	}

	std::vector<VkExtensionProperties> available(extensionCount);
	result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, available.data());
	if (!VulkanResult::Succeeded(result))
	{
		return false;
	}

	const bool availableOnDevice = std::any_of(
	    available.begin(),
	    available.end(),
	    [extensionName](const VkExtensionProperties& properties) noexcept
	    {
		    return std::strcmp(properties.extensionName, extensionName) == 0;
	    });
	if (!availableOnDevice)
	{
		return false;
	}

	extensions.push_back(extensionName);
	return true;
}

void VulkanRhi::CreateInstance() noexcept
{
	std::uint32_t loaderApiVersion = VK_API_VERSION_1_0;
	if (vkEnumerateInstanceVersion != nullptr)
	{
		(void) vkEnumerateInstanceVersion(&loaderApiVersion);
	}
	if (loaderApiVersion < VK_API_VERSION_1_3)
	{
		Diagnostics::Fatal(
		    g_vulkanRhiLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan 1.3 is required by the engine's SPIR-V 1.6 runtime shader contract.");
	}

	std::vector<const char*> instanceExtensions;
	if (IsInstanceExtensionAvailable(VK_KHR_SURFACE_EXTENSION_NAME))
	{
		instanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	}
	if (IsInstanceExtensionAvailable(VK_KHR_WIN32_SURFACE_EXTENSION_NAME))
	{
		instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
	}

#if ENGINE_GPU_VALIDATION
	if (IsLayerAvailable("VK_LAYER_KHRONOS_validation"))
	{
		m_enabledLayers.emplace_back("VK_LAYER_KHRONOS_validation");
		m_validationEnabled = true;
	}
	else
	{
		SPDLOG_LOGGER_WARN(g_vulkanRhiLogger, "Vulkan validation requested, but VK_LAYER_KHRONOS_validation is unavailable.");
	}

	if (IsInstanceExtensionAvailable(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
	{
		instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	else
	{
		SPDLOG_LOGGER_WARN(g_vulkanRhiLogger, "Vulkan debug utils extension is unavailable.");
	}
#endif

	for (const char* extension : instanceExtensions)
	{
		m_enabledInstanceExtensions.emplace_back(extension);
	}

	std::vector<const char*> layerNames;
	layerNames.reserve(m_enabledLayers.size());
	for (const std::string& layer : m_enabledLayers)
	{
		layerNames.push_back(layer.c_str());
	}

	const VkApplicationInfo applicationInfo{
	    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
	    .pNext = nullptr,
	    .pApplicationName = "Sparkle",
	    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
	    .pEngineName = "SparkleEngine",
	    .engineVersion = VK_MAKE_VERSION(1, 0, 0),
	    .apiVersion = VK_API_VERSION_1_3};

	const VkInstanceCreateInfo createInfo{
	    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .pApplicationInfo = &applicationInfo,
	    .enabledLayerCount = static_cast<std::uint32_t>(layerNames.size()),
	    .ppEnabledLayerNames = layerNames.empty() ? nullptr : layerNames.data(),
	    .enabledExtensionCount = static_cast<std::uint32_t>(instanceExtensions.size()),
	    .ppEnabledExtensionNames = instanceExtensions.empty() ? nullptr : instanceExtensions.data()};

	const VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fatal(g_vulkanRhiLogger, __FILE__, __LINE__, VulkanResult::FormatFailure("vkCreateInstance", result));
	}
}

void VulkanRhi::CreateDebugMessenger() noexcept
{
	m_debugLayer.Initialize(m_instance, &VulkanRhi::DebugUtilsCallback, this);
}

void VulkanRhi::SelectPhysicalDevice() noexcept
{
	std::uint32_t deviceCount = 0;
	VkResult result = vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
	if (!VulkanResult::Succeeded(result) || deviceCount == 0)
	{
		Diagnostics::Fatal(g_vulkanRhiLogger, __FILE__, __LINE__, "No Vulkan physical devices were found.");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	result = vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fatal(g_vulkanRhiLogger, __FILE__, __LINE__, VulkanResult::FormatFailure("vkEnumeratePhysicalDevices", result));
	}

	std::vector<PhysicalDeviceCandidate> candidates;
	for (VkPhysicalDevice device : devices)
	{
		PhysicalDeviceCandidate candidate;
		candidate.Device = device;
		candidate.Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		candidate.Features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		candidate.Features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		vkGetPhysicalDeviceProperties(device, &candidate.Properties);
		if (candidate.Properties.apiVersion < VK_API_VERSION_1_3)
		{
			continue;
		}
		candidate.Features.pNext = &candidate.Features12;
		candidate.Features12.pNext = &candidate.Features13;
		vkGetPhysicalDeviceFeatures2(device, &candidate.Features);
		if (candidate.Features.features.shaderStorageImageReadWithoutFormat != VK_TRUE ||
		    candidate.Features.features.shaderStorageImageWriteWithoutFormat != VK_TRUE)
		{
			continue;
		}
		if (candidate.Features12.timelineSemaphore != VK_TRUE)
		{
			continue;
		}
		candidate.QueueTopology = VulkanQueueTopology::Select(device);
		if (!candidate.QueueTopology.Supports(ERhiQueueType::Graphics) || !candidate.QueueTopology.Supports(ERhiQueueType::Compute) ||
		    !candidate.QueueTopology.Supports(ERhiQueueType::Copy))
		{
			continue;
		}
		candidate.Score = ScorePhysicalDevice(candidate.Properties);
		candidates.push_back(candidate);
	}

	if (candidates.empty())
	{
		Diagnostics::Fatal(
		    g_vulkanRhiLogger,
		    __FILE__,
		    __LINE__,
		    "No Vulkan 1.3 physical device exposes a graphics queue and formatless storage-image reads and writes.");
	}

	std::sort(
	    candidates.begin(),
	    candidates.end(),
	    [](const PhysicalDeviceCandidate& lhs, const PhysicalDeviceCandidate& rhs) noexcept
	    {
		    return lhs.Score > rhs.Score;
	    });

	const PhysicalDeviceCandidate& selected = candidates.front();
	m_physicalDevice = selected.Device;
	m_queueTopology = selected.QueueTopology;
	m_adapterInfo = BuildAdapterInfo(selected.Properties);
	m_featureStatus.SupportsSynchronization2 = selected.Features13.synchronization2 == VK_TRUE;
	m_featureStatus.SupportsTimelineSemaphore = selected.Features12.timelineSemaphore == VK_TRUE;
	m_featureStatus.SupportsDynamicRendering = selected.Features13.dynamicRendering == VK_TRUE;
	m_featureStatus.SupportsSamplerAnisotropy = selected.Features.features.samplerAnisotropy == VK_TRUE;
	m_featureStatus.SupportsFillModeNonSolid = selected.Features.features.fillModeNonSolid == VK_TRUE;
	m_featureStatus.SupportsShaderInt64 = selected.Features.features.shaderInt64 == VK_TRUE;
	m_featureStatus.SupportsStorageImageReadWithoutFormat = selected.Features.features.shaderStorageImageReadWithoutFormat == VK_TRUE;
	m_featureStatus.SupportsStorageImageWriteWithoutFormat = selected.Features.features.shaderStorageImageWriteWithoutFormat == VK_TRUE;
	m_featureStatus.SupportsSampledImageArrayNonUniformIndexing = selected.Features12.shaderSampledImageArrayNonUniformIndexing == VK_TRUE;
	m_featureStatus.SupportsPartiallyBoundDescriptorArrays = selected.Features12.descriptorBindingPartiallyBound == VK_TRUE;
	m_featureStatus.SupportsShaderDemoteToHelperInvocation = selected.Features13.shaderDemoteToHelperInvocation == VK_TRUE;
	m_featureStatus.SupportsMutableDescriptorType = QueryMutableDescriptorTypeFeature(m_physicalDevice);
	m_featureStatus.RayTracing = VulkanRayTracingFeatureQuery::Query(m_physicalDevice);
}

void VulkanRhi::CreateLogicalDevice() noexcept
{
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::vector<std::vector<float>> queuePriorities;
	const std::span<const VulkanQueueFamilyRequest> familyRequests = m_queueTopology.GetFamilyRequests();
	queueCreateInfos.reserve(familyRequests.size());
	queuePriorities.reserve(familyRequests.size());
	for (const VulkanQueueFamilyRequest& familyRequest : familyRequests)
	{
		queuePriorities.emplace_back(familyRequest.QueueCount, 1.0f);
		queueCreateInfos.push_back(
		    VkDeviceQueueCreateInfo{
		        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		        .pNext = nullptr,
		        .flags = 0,
		        .queueFamilyIndex = familyRequest.FamilyIndex,
		        .queueCount = familyRequest.QueueCount,
		        .pQueuePriorities = queuePriorities.back().data()});
	}

	std::vector<const char*> deviceExtensions;
	if (IsDeviceExtensionAvailable(m_physicalDevice, VK_KHR_SWAPCHAIN_EXTENSION_NAME))
	{
		deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	}
	if (IsDeviceExtensionAvailable(m_physicalDevice, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME))
	{
		deviceExtensions.push_back(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
	}
	if (m_featureStatus.SupportsMutableDescriptorType)
	{
		deviceExtensions.push_back(VK_EXT_MUTABLE_DESCRIPTOR_TYPE_EXTENSION_NAME);
		m_featureStatus.EnabledMutableDescriptorType = true;
	}
	if (m_featureStatus.RayTracing.EnabledBackend)
	{
		deviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
		deviceExtensions.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
		deviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
		if (m_featureStatus.RayTracing.SupportsRayTracingPipelineExtension && m_featureStatus.RayTracing.SupportsRayTracingPipelineFeature)
		{
			deviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
		}
		if (m_adapterInfo.ApiVersion < VK_API_VERSION_1_2 && m_featureStatus.RayTracing.SupportsBufferDeviceAddressExtension)
		{
			deviceExtensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
		}
		if (m_adapterInfo.VendorId == NvidiaVendorId && m_featureStatus.RayTracing.SupportsPartitionedAccelerationStructureExtension &&
		    m_featureStatus.RayTracing.SupportsPartitionedAccelerationStructureFeature)
		{
			deviceExtensions.push_back(VK_NV_PARTITIONED_ACCELERATION_STRUCTURE_EXTENSION_NAME);
			m_featureStatus.RayTracing.EnabledPartitionedAccelerationStructure = true;
		}
	}
	AppendAvailableDeviceExtension(m_physicalDevice, deviceExtensions, NvidiaBinaryImportExtensionName);
	AppendAvailableDeviceExtension(m_physicalDevice, deviceExtensions, NvidiaImageViewHandleExtensionName);
	for (const char* extension : deviceExtensions)
	{
		m_enabledDeviceExtensions.emplace_back(extension);
	}

	VkPhysicalDeviceFeatures2 enabledFeatures{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
	enabledFeatures.features.samplerAnisotropy = m_featureStatus.SupportsSamplerAnisotropy ? VK_TRUE : VK_FALSE;
	enabledFeatures.features.fillModeNonSolid = m_featureStatus.SupportsFillModeNonSolid ? VK_TRUE : VK_FALSE;
	enabledFeatures.features.shaderInt64 = m_featureStatus.SupportsShaderInt64 ? VK_TRUE : VK_FALSE;
	enabledFeatures.features.shaderStorageImageReadWithoutFormat =
	    m_featureStatus.SupportsStorageImageReadWithoutFormat ? VK_TRUE : VK_FALSE;
	enabledFeatures.features.shaderStorageImageWriteWithoutFormat =
	    m_featureStatus.SupportsStorageImageWriteWithoutFormat ? VK_TRUE : VK_FALSE;
	m_featureStatus.EnabledSamplerAnisotropy = enabledFeatures.features.samplerAnisotropy == VK_TRUE;
	m_featureStatus.EnabledFillModeNonSolid = enabledFeatures.features.fillModeNonSolid == VK_TRUE;
	m_featureStatus.EnabledShaderInt64 = enabledFeatures.features.shaderInt64 == VK_TRUE;
	m_featureStatus.EnabledStorageImageReadWithoutFormat = enabledFeatures.features.shaderStorageImageReadWithoutFormat == VK_TRUE;
	m_featureStatus.EnabledStorageImageWriteWithoutFormat = enabledFeatures.features.shaderStorageImageWriteWithoutFormat == VK_TRUE;
	VkPhysicalDeviceVulkan13Features enabledFeatures13{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
	VkPhysicalDeviceVulkan12Features enabledFeatures12{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
	void** enabledNext = &enabledFeatures.pNext;
	if (m_adapterInfo.ApiVersion >= VK_API_VERSION_1_2)
	{
		enabledFeatures12.timelineSemaphore = m_featureStatus.SupportsTimelineSemaphore ? VK_TRUE : VK_FALSE;
		enabledFeatures12.shaderSampledImageArrayNonUniformIndexing =
		    m_featureStatus.SupportsSampledImageArrayNonUniformIndexing ? VK_TRUE : VK_FALSE;
		enabledFeatures12.descriptorBindingPartiallyBound = m_featureStatus.SupportsPartiallyBoundDescriptorArrays ? VK_TRUE : VK_FALSE;
		*enabledNext = &enabledFeatures12;
		enabledNext = &enabledFeatures12.pNext;
		m_featureStatus.EnabledSampledImageArrayNonUniformIndexing = enabledFeatures12.shaderSampledImageArrayNonUniformIndexing == VK_TRUE;
		m_featureStatus.EnabledPartiallyBoundDescriptorArrays = enabledFeatures12.descriptorBindingPartiallyBound == VK_TRUE;
		m_featureStatus.EnabledTimelineSemaphore = enabledFeatures12.timelineSemaphore == VK_TRUE;
	}
	if (m_adapterInfo.ApiVersion >= VK_API_VERSION_1_3)
	{
		enabledFeatures13.synchronization2 = m_featureStatus.SupportsSynchronization2 ? VK_TRUE : VK_FALSE;
		enabledFeatures13.dynamicRendering = m_featureStatus.SupportsDynamicRendering ? VK_TRUE : VK_FALSE;
		enabledFeatures13.shaderDemoteToHelperInvocation = m_featureStatus.SupportsShaderDemoteToHelperInvocation ? VK_TRUE : VK_FALSE;
		*enabledNext = &enabledFeatures13;
		enabledNext = &enabledFeatures13.pNext;
		m_featureStatus.EnabledSynchronization2 = enabledFeatures13.synchronization2 == VK_TRUE;
		m_featureStatus.EnabledDynamicRendering = enabledFeatures13.dynamicRendering == VK_TRUE;

		m_featureStatus.EnabledShaderDemoteToHelperInvocation = enabledFeatures13.shaderDemoteToHelperInvocation == VK_TRUE;
	}
	VkPhysicalDeviceAccelerationStructureFeaturesKHR enabledAccelerationStructureFeatures{
	    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
	VkPhysicalDeviceRayTracingPipelineFeaturesKHR enabledRayTracingPipelineFeatures{
	    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
	VkPhysicalDeviceRayQueryFeaturesKHR enabledRayQueryFeatures{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR};
	VkPhysicalDevicePartitionedAccelerationStructureFeaturesNV enabledPartitionedAccelerationStructureFeatures{
	    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PARTITIONED_ACCELERATION_STRUCTURE_FEATURES_NV};
	VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT enabledMutableDescriptorTypeFeatures{
	    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT};
	if (m_featureStatus.RayTracing.EnabledBackend)
	{
		enabledFeatures12.bufferDeviceAddress = VK_TRUE;
		enabledAccelerationStructureFeatures.accelerationStructure = VK_TRUE;
		enabledRayQueryFeatures.rayQuery = VK_TRUE;
		*enabledNext = &enabledAccelerationStructureFeatures;
		enabledNext = &enabledAccelerationStructureFeatures.pNext;
		if (m_featureStatus.RayTracing.SupportsRayTracingPipelineExtension && m_featureStatus.RayTracing.SupportsRayTracingPipelineFeature)
		{
			enabledRayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
			*enabledNext = &enabledRayTracingPipelineFeatures;
			enabledNext = &enabledRayTracingPipelineFeatures.pNext;
		}
		*enabledNext = &enabledRayQueryFeatures;
		enabledNext = &enabledRayQueryFeatures.pNext;
		if (m_featureStatus.RayTracing.EnabledPartitionedAccelerationStructure)
		{
			enabledPartitionedAccelerationStructureFeatures.partitionedAccelerationStructure = VK_TRUE;
			*enabledNext = &enabledPartitionedAccelerationStructureFeatures;
			enabledNext = &enabledPartitionedAccelerationStructureFeatures.pNext;
		}
	}
	if (m_featureStatus.EnabledMutableDescriptorType)
	{
		enabledMutableDescriptorTypeFeatures.mutableDescriptorType = VK_TRUE;
		*enabledNext = &enabledMutableDescriptorTypeFeatures;
		enabledNext = &enabledMutableDescriptorTypeFeatures.pNext;
	}

	const VkDeviceCreateInfo createInfo{
	    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
	    .pNext = &enabledFeatures,
	    .flags = 0,
	    .queueCreateInfoCount = static_cast<std::uint32_t>(queueCreateInfos.size()),
	    .pQueueCreateInfos = queueCreateInfos.data(),
	    .enabledLayerCount = 0,
	    .ppEnabledLayerNames = nullptr,
	    .enabledExtensionCount = static_cast<std::uint32_t>(deviceExtensions.size()),
	    .ppEnabledExtensionNames = deviceExtensions.empty() ? nullptr : deviceExtensions.data(),
	    .pEnabledFeatures = nullptr};

	const VkResult result = vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device);
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fatal(g_vulkanRhiLogger, __FILE__, __LINE__, VulkanResult::FormatFailure("vkCreateDevice", result));
	}

	std::array<std::shared_ptr<VulkanNativeQueue>, RhiQueueTypeCount> nativeQueues{};
	for (std::size_t queueIndex = 0; queueIndex < RhiQueueTypeCount; ++queueIndex)
	{
		const ERhiQueueType queueType = static_cast<ERhiQueueType>(queueIndex);
		const VulkanQueueLocation& queueLocation = m_queueTopology.Get(queueType);
		for (std::size_t previousQueueIndex = 0; previousQueueIndex < queueIndex; ++previousQueueIndex)
		{
			if (m_queueTopology.Get(static_cast<ERhiQueueType>(previousQueueIndex)) == queueLocation)
			{
				nativeQueues[queueIndex] = nativeQueues[previousQueueIndex];
				break;
			}
		}
		if (nativeQueues[queueIndex] == nullptr)
		{
			VkQueue nativeQueue = VK_NULL_HANDLE;
			vkGetDeviceQueue(m_device, queueLocation.FamilyIndex, queueLocation.QueueIndex, &nativeQueue);
			nativeQueues[queueIndex] = std::make_shared<VulkanNativeQueue>();
			nativeQueues[queueIndex]->Queue = nativeQueue;
		}
		m_queues[queueIndex] = std::make_unique<VulkanCommandQueue>(*this, queueType, nativeQueues[queueIndex]);
	}
	if (GetGraphicsQueue() == VK_NULL_HANDLE)
	{
		Diagnostics::Fatal(g_vulkanRhiLogger, __FILE__, __LINE__, "vkGetDeviceQueue returned a null graphics queue.");
	}
}

void VulkanRhi::NameBootstrapObjects() noexcept
{
	if (m_setDebugUtilsObjectName == nullptr || m_device == VK_NULL_HANDLE)
	{
		return;
	}

	(void) VulkanDebugNames::SetObjectName(
	    m_setDebugUtilsObjectName,
	    m_device,
	    VK_OBJECT_TYPE_DEVICE,
	    reinterpret_cast<std::uint64_t>(m_device),
	    "Sparkle Vulkan Device");
	for (std::size_t queueIndex = 0; queueIndex < RhiQueueTypeCount; ++queueIndex)
	{
		const ERhiQueueType queueType = static_cast<ERhiQueueType>(queueIndex);
		const VulkanCommandQueue& queue = *m_queues[queueIndex];
		if (queue.GetNativeQueue() == VK_NULL_HANDLE)
		{
			continue;
		}
		const std::string queueName = std::format("Sparkle Vulkan {} Queue", RhiQueueTypeToString(queueType));
		(void) VulkanDebugNames::SetObjectName(
		    m_setDebugUtilsObjectName,
		    m_device,
		    VK_OBJECT_TYPE_QUEUE,
		    reinterpret_cast<std::uint64_t>(queue.GetNativeQueue()),
		    queueName);
		const std::string timelineName = std::format("Sparkle Vulkan {} Queue Timeline", RhiQueueTypeToString(queueType));
		(void) VulkanDebugNames::SetObjectName(
		    m_setDebugUtilsObjectName,
		    m_device,
		    VK_OBJECT_TYPE_SEMAPHORE,
		    reinterpret_cast<std::uint64_t>(queue.GetTimelineSemaphore()),
		    timelineName);
	}
}

void VulkanRhi::LogBootstrapSummary() noexcept
{
	const std::string instanceExtensions = std::format("Enabled Vulkan instance extensions: {}", m_enabledInstanceExtensions.size());
	const std::string deviceExtensions = std::format("Enabled Vulkan device extensions: {}", m_enabledDeviceExtensions.size());
	const std::string featureSummary = std::format(
	    "Vulkan features: validation={}, synchronization2 supported/enabled={}/{}, timelineSemaphore supported/enabled={}/{}, "
	    "dynamicRendering supported/enabled={}/{}, "
	    "mutableDescriptorType supported/enabled={}/{}, "
	    "samplerAnisotropy supported/enabled={}/{}, fillModeNonSolid supported/enabled={}/{}, "
	    "rtExtensions(as={}, pipeline={}, rayQuery={}, deferredHostOps={}, bda={}, partitionedTlasNv={}), "
	    "rtFeatures(as={}, pipeline={}, rayQuery={}, bda={}, partitionedTlasNv={}), rtBackendEnabled={}, ptlasNvEnabled={}",
	    m_validationEnabled,
	    m_featureStatus.SupportsSynchronization2,
	    m_featureStatus.EnabledSynchronization2,
	    m_featureStatus.SupportsTimelineSemaphore,
	    m_featureStatus.EnabledTimelineSemaphore,
	    m_featureStatus.SupportsDynamicRendering,
	    m_featureStatus.EnabledDynamicRendering,
	    m_featureStatus.SupportsMutableDescriptorType,
	    m_featureStatus.EnabledMutableDescriptorType,
	    m_featureStatus.SupportsSamplerAnisotropy,
	    m_featureStatus.EnabledSamplerAnisotropy,
	    m_featureStatus.SupportsFillModeNonSolid,
	    m_featureStatus.EnabledFillModeNonSolid,
	    m_featureStatus.RayTracing.SupportsAccelerationStructureExtension,
	    m_featureStatus.RayTracing.SupportsRayTracingPipelineExtension,
	    m_featureStatus.RayTracing.SupportsRayQueryExtension,
	    m_featureStatus.RayTracing.SupportsDeferredHostOperationsExtension,
	    m_featureStatus.RayTracing.SupportsBufferDeviceAddressExtension,
	    m_featureStatus.RayTracing.SupportsPartitionedAccelerationStructureExtension,
	    m_featureStatus.RayTracing.SupportsAccelerationStructureFeature,
	    m_featureStatus.RayTracing.SupportsRayTracingPipelineFeature,
	    m_featureStatus.RayTracing.SupportsRayQueryFeature,
	    m_featureStatus.RayTracing.SupportsBufferDeviceAddressFeature,
	    m_featureStatus.RayTracing.SupportsPartitionedAccelerationStructureFeature,
	    m_featureStatus.RayTracing.EnabledBackend,
	    m_featureStatus.RayTracing.EnabledPartitionedAccelerationStructure);
	const std::string adapterSummary = std::format(
	    "Vulkan adapter: name='{}', api={}, driver={}, queues(graphics={}:{}, compute={}:{}, copy={}:{}), independent(compute={}, copy={})",
	    m_adapterInfo.Name,
	    FormatApiVersion(m_adapterInfo.ApiVersion),
	    m_adapterInfo.Driver,
	    GetQueueFamilyIndex(ERhiQueueType::Graphics),
	    GetQueueIndex(ERhiQueueType::Graphics),
	    GetQueueFamilyIndex(ERhiQueueType::Compute),
	    GetQueueIndex(ERhiQueueType::Compute),
	    GetQueueFamilyIndex(ERhiQueueType::Copy),
	    GetQueueIndex(ERhiQueueType::Copy),
	    HasIndependentQueue(ERhiQueueType::Compute),
	    HasIndependentQueue(ERhiQueueType::Copy));
	const std::string providerSummary = std::format(
	    "Vulkan ray tracing provider: topLevel={}({}), partitionedTlasProvider={} supported={} reason={}",
	    RhiRayTracingTopLevelProviderToString(m_rayTracingCapabilities.Groups.Provider.SelectedTopLevelProvider),
	    m_rayTracingCapabilities.Groups.Provider.SelectedTopLevelProviderReason,
	    RhiPartitionedTlasProviderToString(m_rayTracingCapabilities.Groups.PartitionedTlas.Provider),
	    m_rayTracingCapabilities.Groups.PartitionedTlas.Supported,
	    m_rayTracingCapabilities.Groups.PartitionedTlas.CapabilityStatusReason);

	PushDiagnosticMessage(ERhiDiagnosticMessageSeverity::Info, ERhiDiagnosticMessageCategory::Driver, adapterSummary);
	PushDiagnosticMessage(ERhiDiagnosticMessageSeverity::Info, ERhiDiagnosticMessageCategory::Validation, featureSummary);
	PushDiagnosticMessage(ERhiDiagnosticMessageSeverity::Info, ERhiDiagnosticMessageCategory::Validation, providerSummary);
	PushDiagnosticMessage(ERhiDiagnosticMessageSeverity::Info, ERhiDiagnosticMessageCategory::Driver, instanceExtensions);
	PushDiagnosticMessage(ERhiDiagnosticMessageSeverity::Info, ERhiDiagnosticMessageCategory::Driver, deviceExtensions);

	if (!m_featureStatus.RayTracing.EnabledBackend)
	{
		const std::string rayTracingSummary = std::format(
		    "Vulkan ray query backend disabled: hardwareExtensions(as={}, pipeline={}, rayQuery={}, deferredHostOps={}, bda={}) "
		    "hardwareFeatures(as={}, pipeline={}, rayQuery={}, bda={}) functionsLoaded={}.",
		    m_featureStatus.RayTracing.SupportsAccelerationStructureExtension,
		    m_featureStatus.RayTracing.SupportsRayTracingPipelineExtension,
		    m_featureStatus.RayTracing.SupportsRayQueryExtension,
		    m_featureStatus.RayTracing.SupportsDeferredHostOperationsExtension,
		    m_featureStatus.RayTracing.SupportsBufferDeviceAddressExtension,
		    m_featureStatus.RayTracing.SupportsAccelerationStructureFeature,
		    m_featureStatus.RayTracing.SupportsRayTracingPipelineFeature,
		    m_featureStatus.RayTracing.SupportsRayQueryFeature,
		    m_featureStatus.RayTracing.SupportsBufferDeviceAddressFeature,
		    m_getBufferDeviceAddress != nullptr && m_createAccelerationStructure != nullptr && m_destroyAccelerationStructure != nullptr &&
		        m_getAccelerationStructureBuildSizes != nullptr && m_cmdBuildAccelerationStructures != nullptr &&
		        m_getAccelerationStructureDeviceAddress != nullptr);
		SPDLOG_LOGGER_WARN(g_vulkanRhiLogger, "{}", rayTracingSummary);
		PushDiagnosticMessage(ERhiDiagnosticMessageSeverity::Warning, ERhiDiagnosticMessageCategory::Validation, rayTracingSummary);
	}
}

bool VulkanRhi::IsLayerAvailable(const char* layerName) noexcept
{
	std::uint32_t layerCount = 0;
	if (!VulkanResult::Succeeded(vkEnumerateInstanceLayerProperties(&layerCount, nullptr)))
	{
		return false;
	}

	std::vector<VkLayerProperties> layers(layerCount);
	if (!VulkanResult::Succeeded(vkEnumerateInstanceLayerProperties(&layerCount, layers.data())))
	{
		return false;
	}

	return std::any_of(
	    layers.begin(),
	    layers.end(),
	    [layerName](const VkLayerProperties& layer) noexcept
	    {
		    return std::string_view(layer.layerName) == layerName;
	    });
}

bool VulkanRhi::IsInstanceExtensionAvailable(const char* extensionName) noexcept
{
	std::uint32_t extensionCount = 0;
	if (!VulkanResult::Succeeded(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr)))
	{
		return false;
	}

	std::vector<VkExtensionProperties> extensions(extensionCount);
	if (!VulkanResult::Succeeded(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data())))
	{
		return false;
	}

	return std::any_of(
	    extensions.begin(),
	    extensions.end(),
	    [extensionName](const VkExtensionProperties& extension) noexcept
	    {
		    return std::string_view(extension.extensionName) == extensionName;
	    });
}

bool VulkanRhi::IsDeviceExtensionAvailable(VkPhysicalDevice device, const char* extensionName) noexcept
{
	std::uint32_t extensionCount = 0;
	if (!VulkanResult::Succeeded(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr)))
	{
		return false;
	}

	std::vector<VkExtensionProperties> extensions(extensionCount);
	if (!VulkanResult::Succeeded(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data())))
	{
		return false;
	}

	return std::any_of(
	    extensions.begin(),
	    extensions.end(),
	    [extensionName](const VkExtensionProperties& extension) noexcept
	    {
		    return std::string_view(extension.extensionName) == extensionName;
	    });
}

std::uint32_t VulkanRhi::ScorePhysicalDevice(const VkPhysicalDeviceProperties& properties) noexcept
{
	if (CVarPreferHighPerformanceAdapter.Get())
	{
		if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			return 300;
		}
		if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
		{
			return 200;
		}
	}
	else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
	{
		return 300;
	}

	return properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU ? 10u : 100u;
}

VulkanAdapterInfo VulkanRhi::BuildAdapterInfo(const VkPhysicalDeviceProperties& properties)
{
	return VulkanAdapterInfo{
	    .Name = properties.deviceName,
	    .Driver = FormatDriverVersion(properties),
	    .Vendor = std::format("{:#06x}", properties.vendorID),
	    .VendorId = properties.vendorID,
	    .DeviceId = properties.deviceID,
	    .ApiVersion = properties.apiVersion,
	    .DriverVersion = properties.driverVersion,
	    .DeviceType = properties.deviceType};
}

std::string VulkanRhi::FormatApiVersion(std::uint32_t version)
{
	return std::format("{}.{}.{}", VK_API_VERSION_MAJOR(version), VK_API_VERSION_MINOR(version), VK_API_VERSION_PATCH(version));
}

std::string VulkanRhi::FormatDriverVersion(const VkPhysicalDeviceProperties& properties)
{
	return std::format("{} ({:#010x})", FormatApiVersion(properties.driverVersion), properties.driverVersion);
}

std::string VulkanRhi::PhysicalDeviceTypeToString(VkPhysicalDeviceType type)
{
	switch (type)
	{
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
			return "IntegratedGpu";
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
			return "DiscreteGpu";
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
			return "VirtualGpu";
		case VK_PHYSICAL_DEVICE_TYPE_CPU:
			return "Cpu";
		case VK_PHYSICAL_DEVICE_TYPE_OTHER:
		default:
			return "Other";
	}
}

#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Device/VulkanRhi.h"

#include "CVars/RHICVars.h"
#include "Vulkan/Core/VulkanResult.h"
#include "Vulkan/Diagnostics/VulkanDebugNames.h"

#include "Core/Public/Diagnostics/Trace.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <format>
#include <limits>
#include <utility>

static const auto g_vulkanRhiLogger = Logging::GetOrCreateLogger("RHI.Vulkan");

namespace
{
	constexpr std::uint32_t kNvidiaVendorId = 0x10DE;
	constexpr const char* kNvidiaBinaryImportExtensionName = "VK_NVX_binary_import";
	constexpr const char* kNvidiaImageViewHandleExtensionName = "VK_NVX_image_view_handle";

	bool AppendAvailableDeviceExtension(
	    VkPhysicalDevice physicalDevice,
	    std::vector<const char*>& extensions,
	    const char* extensionName) noexcept
	{
		if (extensionName == nullptr ||
		    std::find_if(
		        extensions.begin(),
		        extensions.end(),
		        [extensionName](const char* enabled) noexcept { return std::strcmp(enabled, extensionName) == 0; }) != extensions.end())
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
		    [extensionName](const VkExtensionProperties& properties) noexcept {
			    return std::strcmp(properties.extensionName, extensionName) == 0;
		    });
		if (!availableOnDevice)
		{
			return false;
		}

		extensions.push_back(extensionName);
		return true;
	}
}

VulkanRhi::VulkanRhi() noexcept
{
	{
		SPARKLE_CPU_SCOPE("RHI.Vulkan.CreateInstance");
		CreateInstance();
	}
	{
		SPARKLE_CPU_SCOPE("RHI.Vulkan.CreateDebugMessenger");
		CreateDebugMessenger();
	}
	{
		SPARKLE_CPU_SCOPE("RHI.Vulkan.SelectPhysicalDevice");
		SelectPhysicalDevice();
	}
	{
		SPARKLE_CPU_SCOPE("RHI.Vulkan.CreateLogicalDevice");
		CreateLogicalDevice();
	}
	LoadDeviceDebugFunctions();
	LoadRayTracingFunctions();
	BuildRayTracingCapabilities();
	NameBootstrapObjects();
	LogBootstrapSummary();
}

VulkanRhi::~VulkanRhi() noexcept
{
	WaitForIdle();

	if (m_device != VK_NULL_HANDLE)
	{
		vkDestroyDevice(m_device, nullptr);
		m_device = VK_NULL_HANDLE;
	}

	m_debugLayer.Shutdown();

	if (m_instance != VK_NULL_HANDLE)
	{
		vkDestroyInstance(m_instance, nullptr);
		m_instance = VK_NULL_HANDLE;
	}
}

void VulkanRhi::WaitForIdle() noexcept
{
	if (m_device != VK_NULL_HANDLE)
	{
		const VkResult result = vkDeviceWaitIdle(m_device);
		if (!VulkanResult::Succeeded(result))
		{
			SPDLOG_LOGGER_WARN(g_vulkanRhiLogger, "vkDeviceWaitIdle failed: {}", VulkanResult::ToString(result));
		}
	}
}

bool VulkanRhi::TryPopDiagnosticMessage(RhiDiagnosticMessage& outMessage) noexcept
{
	std::lock_guard lock(m_diagnosticMessagesMutex);
	if (m_diagnosticMessages.empty())
	{
		return false;
	}

	outMessage = std::move(m_diagnosticMessages.front());
	m_diagnosticMessages.erase(m_diagnosticMessages.begin());
	return true;
}

void VulkanRhi::ClearDiagnosticMessages() noexcept
{
	std::lock_guard lock(m_diagnosticMessagesMutex);
	m_diagnosticMessages.clear();
}

void VulkanRhi::CreateInstance() noexcept
{
	std::uint32_t loaderApiVersion = VK_API_VERSION_1_0;
	if (vkEnumerateInstanceVersion != nullptr)
	{
		(void)vkEnumerateInstanceVersion(&loaderApiVersion);
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
	    .apiVersion = std::min(loaderApiVersion, VK_API_VERSION_1_3)};

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
		Diagnostics::Fail(g_vulkanRhiLogger, __FILE__, __LINE__, VulkanResult::FormatFailure("vkCreateInstance", result));
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
		Diagnostics::Fail(g_vulkanRhiLogger, __FILE__, __LINE__, "No Vulkan physical devices were found.");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	result = vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fail(g_vulkanRhiLogger, __FILE__, __LINE__, VulkanResult::FormatFailure("vkEnumeratePhysicalDevices", result));
	}

	std::vector<PhysicalDeviceCandidate> candidates;
	for (VkPhysicalDevice device : devices)
	{
		PhysicalDeviceCandidate candidate;
		candidate.Device = device;
		candidate.Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		candidate.Features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		vkGetPhysicalDeviceProperties(device, &candidate.Properties);
		if (candidate.Properties.apiVersion >= VK_API_VERSION_1_3)
		{
			candidate.Features.pNext = &candidate.Features13;
		}
		vkGetPhysicalDeviceFeatures2(device, &candidate.Features);
		candidate.GraphicsQueueFamilyIndex = FindGraphicsQueueFamily(device);
		if (candidate.GraphicsQueueFamilyIndex == UINT32_MAX)
		{
			continue;
		}
		candidate.Score = ScorePhysicalDevice(candidate.Properties);
		candidates.push_back(candidate);
	}

	if (candidates.empty())
	{
		Diagnostics::Fail(g_vulkanRhiLogger, __FILE__, __LINE__, "No Vulkan physical device exposes a graphics queue family.");
	}

	std::sort(
	    candidates.begin(),
	    candidates.end(),
	    [](const PhysicalDeviceCandidate& lhs, const PhysicalDeviceCandidate& rhs) noexcept { return lhs.Score > rhs.Score; });

	const PhysicalDeviceCandidate& selected = candidates.front();
	m_physicalDevice = selected.Device;
	m_graphicsQueueFamilyIndex = selected.GraphicsQueueFamilyIndex;
	m_adapterInfo = BuildAdapterInfo(selected.Properties);
	m_featureStatus.SupportsSynchronization2 = selected.Features13.synchronization2 == VK_TRUE;
	m_featureStatus.SupportsDynamicRendering = selected.Features13.dynamicRendering == VK_TRUE;
	m_featureStatus.SupportsSamplerAnisotropy = selected.Features.features.samplerAnisotropy == VK_TRUE;
	m_featureStatus.SupportsFillModeNonSolid = selected.Features.features.fillModeNonSolid == VK_TRUE;
	m_featureStatus.RayTracing = VulkanRayTracingFeatureQuery::Query(m_physicalDevice);

	SPDLOG_LOGGER_INFO(
	    g_vulkanRhiLogger,
	    "Selected Vulkan adapter: name='{}', type={}, vendorId={:#06x}, deviceId={:#06x}, api={}, driver={}",
	    m_adapterInfo.Name,
	    PhysicalDeviceTypeToString(m_adapterInfo.DeviceType),
	    m_adapterInfo.VendorId,
	    m_adapterInfo.DeviceId,
	    FormatApiVersion(m_adapterInfo.ApiVersion),
	    m_adapterInfo.Driver);
}

void VulkanRhi::CreateLogicalDevice() noexcept
{
	const float queuePriority = 1.0f;
	const VkDeviceQueueCreateInfo queueCreateInfo{
	    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .queueFamilyIndex = m_graphicsQueueFamilyIndex,
	    .queueCount = 1,
	    .pQueuePriorities = &queuePriority};

	std::vector<const char*> deviceExtensions;
	if (IsDeviceExtensionAvailable(m_physicalDevice, VK_KHR_SWAPCHAIN_EXTENSION_NAME))
	{
		deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	}
	if (IsDeviceExtensionAvailable(m_physicalDevice, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME))
	{
		deviceExtensions.push_back(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
	}
	if (m_featureStatus.RayTracing.EnabledBackend)
	{
		deviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
		deviceExtensions.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
		deviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
		if (m_adapterInfo.ApiVersion < VK_API_VERSION_1_2 && m_featureStatus.RayTracing.SupportsBufferDeviceAddressExtension)
		{
			deviceExtensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
		}
	}
	const bool enabledNvidiaBinaryImport =
	    AppendAvailableDeviceExtension(m_physicalDevice, deviceExtensions, kNvidiaBinaryImportExtensionName);
	const bool enabledNvidiaImageViewHandle =
	    AppendAvailableDeviceExtension(m_physicalDevice, deviceExtensions, kNvidiaImageViewHandleExtensionName);
	if (enabledNvidiaBinaryImport || enabledNvidiaImageViewHandle)
	{
		SPDLOG_LOGGER_INFO(
		    g_vulkanRhiLogger,
		    "Enabled Vulkan NVIDIA external-feature interop extensions: binaryImport={}, imageViewHandle={}",
		    enabledNvidiaBinaryImport,
		    enabledNvidiaImageViewHandle);
	}
	for (const char* extension : deviceExtensions)
	{
		m_enabledDeviceExtensions.emplace_back(extension);
	}

	VkPhysicalDeviceFeatures2 enabledFeatures{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
	enabledFeatures.features.samplerAnisotropy = m_featureStatus.SupportsSamplerAnisotropy ? VK_TRUE : VK_FALSE;
	enabledFeatures.features.fillModeNonSolid = m_featureStatus.SupportsFillModeNonSolid ? VK_TRUE : VK_FALSE;
	m_featureStatus.EnabledSamplerAnisotropy = enabledFeatures.features.samplerAnisotropy == VK_TRUE;
	m_featureStatus.EnabledFillModeNonSolid = enabledFeatures.features.fillModeNonSolid == VK_TRUE;
	VkPhysicalDeviceVulkan13Features enabledFeatures13{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
	void** enabledNext = &enabledFeatures.pNext;
	if (m_adapterInfo.ApiVersion >= VK_API_VERSION_1_3)
	{
		enabledFeatures13.synchronization2 = m_featureStatus.SupportsSynchronization2 ? VK_TRUE : VK_FALSE;
		enabledFeatures13.dynamicRendering = m_featureStatus.SupportsDynamicRendering ? VK_TRUE : VK_FALSE;
		*enabledNext = &enabledFeatures13;
		enabledNext = &enabledFeatures13.pNext;
		m_featureStatus.EnabledSynchronization2 = enabledFeatures13.synchronization2 == VK_TRUE;
		m_featureStatus.EnabledDynamicRendering = enabledFeatures13.dynamicRendering == VK_TRUE;
	}
	VkPhysicalDeviceBufferDeviceAddressFeatures enabledBufferDeviceAddressFeatures{
	    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES};
	VkPhysicalDeviceAccelerationStructureFeaturesKHR enabledAccelerationStructureFeatures{
	    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
	VkPhysicalDeviceRayQueryFeaturesKHR enabledRayQueryFeatures{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR};
	if (m_featureStatus.RayTracing.EnabledBackend)
	{
		enabledBufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
		enabledAccelerationStructureFeatures.accelerationStructure = VK_TRUE;
		enabledRayQueryFeatures.rayQuery = VK_TRUE;
		*enabledNext = &enabledBufferDeviceAddressFeatures;
		enabledNext = &enabledBufferDeviceAddressFeatures.pNext;
		*enabledNext = &enabledAccelerationStructureFeatures;
		enabledNext = &enabledAccelerationStructureFeatures.pNext;
		*enabledNext = &enabledRayQueryFeatures;
	}

	const VkDeviceCreateInfo createInfo{
	    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
	    .pNext = &enabledFeatures,
	    .flags = 0,
	    .queueCreateInfoCount = 1,
	    .pQueueCreateInfos = &queueCreateInfo,
	    .enabledLayerCount = 0,
	    .ppEnabledLayerNames = nullptr,
	    .enabledExtensionCount = static_cast<std::uint32_t>(deviceExtensions.size()),
	    .ppEnabledExtensionNames = deviceExtensions.empty() ? nullptr : deviceExtensions.data(),
	    .pEnabledFeatures = nullptr};

	const VkResult result = vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device);
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fail(g_vulkanRhiLogger, __FILE__, __LINE__, VulkanResult::FormatFailure("vkCreateDevice", result));
	}

	vkGetDeviceQueue(m_device, m_graphicsQueueFamilyIndex, 0, &m_graphicsQueue);
	if (m_graphicsQueue == VK_NULL_HANDLE)
	{
		Diagnostics::Fail(g_vulkanRhiLogger, __FILE__, __LINE__, "vkGetDeviceQueue returned a null graphics queue.");
	}
}

void VulkanRhi::LoadDeviceDebugFunctions() noexcept
{
	if (m_device == VK_NULL_HANDLE)
	{
		return;
	}

	m_setDebugUtilsObjectName = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
	    vkGetDeviceProcAddr(m_device, "vkSetDebugUtilsObjectNameEXT"));
	m_cmdBeginDebugUtilsLabel = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(
	    vkGetDeviceProcAddr(m_device, "vkCmdBeginDebugUtilsLabelEXT"));
	m_cmdEndDebugUtilsLabel = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(
	    vkGetDeviceProcAddr(m_device, "vkCmdEndDebugUtilsLabelEXT"));
	m_cmdInsertDebugUtilsLabel = reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(
	    vkGetDeviceProcAddr(m_device, "vkCmdInsertDebugUtilsLabelEXT"));
}

void VulkanRhi::LoadRayTracingFunctions() noexcept
{
	if (m_device == VK_NULL_HANDLE || !m_featureStatus.RayTracing.EnabledBackend)
	{
		return;
	}

	m_getBufferDeviceAddress = reinterpret_cast<PFN_vkGetBufferDeviceAddress>(vkGetDeviceProcAddr(m_device, "vkGetBufferDeviceAddress"));
	if (m_getBufferDeviceAddress == nullptr)
	{
		m_getBufferDeviceAddress = reinterpret_cast<PFN_vkGetBufferDeviceAddress>(
		    vkGetDeviceProcAddr(m_device, "vkGetBufferDeviceAddressKHR"));
	}
	m_createAccelerationStructure = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(
	    vkGetDeviceProcAddr(m_device, "vkCreateAccelerationStructureKHR"));
	m_destroyAccelerationStructure = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(
	    vkGetDeviceProcAddr(m_device, "vkDestroyAccelerationStructureKHR"));
	m_getAccelerationStructureBuildSizes = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(
	    vkGetDeviceProcAddr(m_device, "vkGetAccelerationStructureBuildSizesKHR"));
	m_cmdBuildAccelerationStructures = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(
	    vkGetDeviceProcAddr(m_device, "vkCmdBuildAccelerationStructuresKHR"));
	m_getAccelerationStructureDeviceAddress = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(
	    vkGetDeviceProcAddr(m_device, "vkGetAccelerationStructureDeviceAddressKHR"));

	const bool loaded = m_getBufferDeviceAddress != nullptr && m_createAccelerationStructure != nullptr &&
	                    m_destroyAccelerationStructure != nullptr && m_getAccelerationStructureBuildSizes != nullptr &&
	                    m_cmdBuildAccelerationStructures != nullptr && m_getAccelerationStructureDeviceAddress != nullptr;
	if (!loaded)
	{
		m_featureStatus.RayTracing.EnabledBackend = false;
	}
}

void VulkanRhi::BuildRayTracingCapabilities() noexcept
{
	m_rayTracingCapabilities = {};
	m_rayTracingCapabilities.Groups.PartitionedTlas = RhiPartitionedTlasCapabilities{
	    .Supported = false,
	    .Provider = ERhiPartitionedTlasProvider::VulkanNvPartitionedAccelerationStructure,
	    .RequiresNvidiaDevice = true,
	    .RunsOnNvidiaDevice = m_adapterInfo.VendorId == kNvidiaVendorId,
	    .SupportsVulkanNativePartitionedAccelerationStructure =
	        m_featureStatus.RayTracing.SupportsPartitionedAccelerationStructureExtension,
	    .SupportsVulkanExtension = m_featureStatus.RayTracing.SupportsPartitionedAccelerationStructureExtension,
	    .SupportsVulkanFeatureQuery = false,
	    .SupportsVulkanFunctionLoading = false,
	    .SupportsVulkanDescriptorPath = false,
	    .CapabilityStatusReason = m_featureStatus.RayTracing.SupportsPartitionedAccelerationStructureExtension
	                                  ? "vulkan-nv-ptlas-provider-not-implemented"
	                                  : "vulkan-nv-partitioned-acceleration-structure-extension-not-present"};
	m_rayTracingCapabilities.Groups.Provider = RhiRayTracingProviderCapabilities{
	    .SelectedTopLevelProvider = ERhiRayTracingTopLevelProvider::None,
	    .SelectedTopLevelProviderReason = "ray-tracing-not-enabled"};
	if (m_physicalDevice == VK_NULL_HANDLE || !m_featureStatus.RayTracing.EnabledBackend)
	{
		return;
	}

	VkPhysicalDeviceAccelerationStructurePropertiesKHR accelerationStructureProperties{
	    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};
	VkPhysicalDeviceProperties2 properties{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
	properties.pNext = &accelerationStructureProperties;
	vkGetPhysicalDeviceProperties2(m_physicalDevice, &properties);

	m_rayTracingCapabilities = RhiRayTracingCapabilities{
	    .SupportsRayTracing = true,
	    .SupportsInlineRayQuery = true,
	    .MaxTraceRecursionDepth = 1,
	    .MaxRayPayloadSizeInBytes = 0,
	    .MaxRayAttributeSizeInBytes = 0,
	    .ShaderGroupHandleSizeInBytes = 0,
	    .ShaderTableAlignmentInBytes = 0,
	    .ShaderTableRecordAlignmentInBytes = 0,
	    .AccelerationStructureByteAlignment = 256,
	    .ScratchBufferByteAlignment = accelerationStructureProperties.minAccelerationStructureScratchOffsetAlignment,
	    .InstanceDescSizeInBytes = static_cast<std::uint32_t>(sizeof(VkAccelerationStructureInstanceKHR))};
	m_rayTracingCapabilities.Groups.AccelerationStructures = RhiAccelerationStructureCapabilities{
	    .SupportsRayTracing = m_rayTracingCapabilities.SupportsRayTracing,
	    .SupportsInlineRayQuery = m_rayTracingCapabilities.SupportsInlineRayQuery,
	    .SupportsAccelerationStructureShaderBinding = m_rayTracingCapabilities.SupportsRayTracing,
	    .MaxTraceRecursionDepth = m_rayTracingCapabilities.MaxTraceRecursionDepth,
	    .MaxRayPayloadSizeInBytes = m_rayTracingCapabilities.MaxRayPayloadSizeInBytes,
	    .MaxRayAttributeSizeInBytes = m_rayTracingCapabilities.MaxRayAttributeSizeInBytes,
	    .ShaderGroupHandleSizeInBytes = m_rayTracingCapabilities.ShaderGroupHandleSizeInBytes,
	    .ShaderTableAlignmentInBytes = m_rayTracingCapabilities.ShaderTableAlignmentInBytes,
	    .ShaderTableRecordAlignmentInBytes = m_rayTracingCapabilities.ShaderTableRecordAlignmentInBytes,
	    .AccelerationStructureByteAlignment = m_rayTracingCapabilities.AccelerationStructureByteAlignment,
	    .ScratchBufferByteAlignment = m_rayTracingCapabilities.ScratchBufferByteAlignment};
	m_rayTracingCapabilities.Groups.ClassicTlas = RhiClassicTlasCapabilities{
	    .SupportsClassicTlasBuild = true,
	    .SupportsClassicTlasUpdate = false,
	    .SupportsGpuReadableInstanceBuffer = true,
	    .InstanceDescSizeInBytes = m_rayTracingCapabilities.InstanceDescSizeInBytes};
	m_rayTracingCapabilities.Groups.PartitionedTlas = RhiPartitionedTlasCapabilities{
	    .Supported = false,
	    .Provider = ERhiPartitionedTlasProvider::VulkanNvPartitionedAccelerationStructure,
	    .RequiresNvidiaDevice = true,
	    .RunsOnNvidiaDevice = m_adapterInfo.VendorId == kNvidiaVendorId,
	    .SupportsVulkanNativePartitionedAccelerationStructure =
	        m_featureStatus.RayTracing.SupportsPartitionedAccelerationStructureExtension,
	    .SupportsVulkanExtension = m_featureStatus.RayTracing.SupportsPartitionedAccelerationStructureExtension,
	    .SupportsVulkanFeatureQuery = false,
	    .SupportsVulkanFunctionLoading = false,
	    .SupportsVulkanDescriptorPath = false,
	    .CapabilityStatusReason = m_featureStatus.RayTracing.SupportsPartitionedAccelerationStructureExtension
	                                  ? "vulkan-nv-ptlas-provider-not-implemented"
	                                  : "vulkan-nv-partitioned-acceleration-structure-extension-not-present"};
	m_rayTracingCapabilities.Groups.Provider = RhiRayTracingProviderCapabilities{
	    .SelectedTopLevelProvider = ERhiRayTracingTopLevelProvider::ClassicTlas,
	    .SelectedTopLevelProviderReason = "classic-tlas-baseline-selected"};
}

void VulkanRhi::NameBootstrapObjects() noexcept
{
	if (m_setDebugUtilsObjectName == nullptr || m_device == VK_NULL_HANDLE)
	{
		return;
	}

	(void)VulkanDebugNames::SetObjectName(
	    m_setDebugUtilsObjectName,
	    m_device,
	    VK_OBJECT_TYPE_DEVICE,
	    reinterpret_cast<std::uint64_t>(m_device),
	    "Sparkle Vulkan Device");
	(void)VulkanDebugNames::SetObjectName(
	    m_setDebugUtilsObjectName,
	    m_device,
	    VK_OBJECT_TYPE_QUEUE,
	    reinterpret_cast<std::uint64_t>(m_graphicsQueue),
	    "Sparkle Vulkan Graphics Queue");
}

void VulkanRhi::LogBootstrapSummary() noexcept
{
	const std::string instanceExtensions = std::format("Enabled Vulkan instance extensions: {}", m_enabledInstanceExtensions.size());
	const std::string deviceExtensions = std::format("Enabled Vulkan device extensions: {}", m_enabledDeviceExtensions.size());
	const std::string featureSummary = std::format(
	    "Vulkan features: validation={}, synchronization2 supported/enabled={}/{}, dynamicRendering supported/enabled={}/{}, "
	    "samplerAnisotropy supported/enabled={}/{}, fillModeNonSolid supported/enabled={}/{}, "
	    "rtExtensions(as={}, pipeline={}, rayQuery={}, deferredHostOps={}, bda={}, partitionedTlasNv={}), "
	    "rtFeatures(as={}, pipeline={}, rayQuery={}, bda={}), rtBackendEnabled={}",
	    m_validationEnabled,
	    m_featureStatus.SupportsSynchronization2,
	    m_featureStatus.EnabledSynchronization2,
	    m_featureStatus.SupportsDynamicRendering,
	    m_featureStatus.EnabledDynamicRendering,
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
	    m_featureStatus.RayTracing.EnabledBackend);
	const std::string adapterSummary = std::format(
	    "Vulkan adapter: name='{}', api={}, driver={}, queueFamily={}",
	    m_adapterInfo.Name,
	    FormatApiVersion(m_adapterInfo.ApiVersion),
	    m_adapterInfo.Driver,
	    m_graphicsQueueFamilyIndex);
	const std::string providerSummary = std::format(
	    "Vulkan ray tracing provider: topLevel={}({}), partitionedTlasProvider={} supported={} reason={}",
	    RhiRayTracingTopLevelProviderToString(m_rayTracingCapabilities.Groups.Provider.SelectedTopLevelProvider),
	    m_rayTracingCapabilities.Groups.Provider.SelectedTopLevelProviderReason,
	    RhiPartitionedTlasProviderToString(m_rayTracingCapabilities.Groups.PartitionedTlas.Provider),
	    m_rayTracingCapabilities.Groups.PartitionedTlas.Supported,
	    m_rayTracingCapabilities.Groups.PartitionedTlas.CapabilityStatusReason);

	SPDLOG_LOGGER_INFO(g_vulkanRhiLogger, "{}", adapterSummary);
	SPDLOG_LOGGER_INFO(g_vulkanRhiLogger, "{}", featureSummary);
	SPDLOG_LOGGER_INFO(g_vulkanRhiLogger, "{}", providerSummary);
	SPDLOG_LOGGER_INFO(g_vulkanRhiLogger, "{}", instanceExtensions);
	SPDLOG_LOGGER_INFO(g_vulkanRhiLogger, "{}", deviceExtensions);
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
		    m_getBufferDeviceAddress != nullptr && m_createAccelerationStructure != nullptr &&
		        m_destroyAccelerationStructure != nullptr && m_getAccelerationStructureBuildSizes != nullptr &&
		        m_cmdBuildAccelerationStructures != nullptr && m_getAccelerationStructureDeviceAddress != nullptr);
		SPDLOG_LOGGER_WARN(g_vulkanRhiLogger, "{}", rayTracingSummary);
		PushDiagnosticMessage(ERhiDiagnosticMessageSeverity::Warning, ERhiDiagnosticMessageCategory::Validation, rayTracingSummary);
	}
}

void VulkanRhi::PushDiagnosticMessage(
	ERhiDiagnosticMessageSeverity severity,
	ERhiDiagnosticMessageCategory category,
	std::string text) noexcept
{
	std::lock_guard lock(m_diagnosticMessagesMutex);
	m_diagnosticMessages.push_back(RhiDiagnosticMessage{.Severity = severity, .Category = category, .Text = std::move(text)});
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

	return std::any_of(layers.begin(), layers.end(), [layerName](const VkLayerProperties& layer) noexcept {
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

	return std::any_of(extensions.begin(), extensions.end(), [extensionName](const VkExtensionProperties& extension) noexcept {
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

	return std::any_of(extensions.begin(), extensions.end(), [extensionName](const VkExtensionProperties& extension) noexcept {
		return std::string_view(extension.extensionName) == extensionName;
	});
}

std::uint32_t VulkanRhi::FindGraphicsQueueFamily(VkPhysicalDevice device) noexcept
{
	std::uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	for (std::uint32_t familyIndex = 0; familyIndex < queueFamilies.size(); ++familyIndex)
	{
		if ((queueFamilies[familyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
		{
			return familyIndex;
		}
	}

	return UINT32_MAX;
}

std::uint32_t VulkanRhi::ScorePhysicalDevice(const VkPhysicalDeviceProperties& properties) noexcept
{
	if (CVarRhiPreferHighPerformanceAdapter.Get())
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

VkBool32 VKAPI_PTR VulkanRhi::DebugUtilsCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
	void* userData) noexcept
{
	auto* rhi = static_cast<VulkanRhi*>(userData);
	if (rhi == nullptr || callbackData == nullptr)
	{
		return VK_FALSE;
	}

	ERhiDiagnosticMessageSeverity sparkleSeverity = ERhiDiagnosticMessageSeverity::Info;
	if ((severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0)
	{
		sparkleSeverity = ERhiDiagnosticMessageSeverity::Error;
	}
	else if ((severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) != 0)
	{
		sparkleSeverity = ERhiDiagnosticMessageSeverity::Warning;
	}
	else if ((severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) != 0)
	{
		sparkleSeverity = ERhiDiagnosticMessageSeverity::Verbose;
	}

	ERhiDiagnosticMessageCategory sparkleCategory = ERhiDiagnosticMessageCategory::General;
	if ((messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) != 0)
	{
		sparkleCategory = ERhiDiagnosticMessageCategory::Validation;
	}
	else if ((messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) != 0)
	{
		sparkleCategory = ERhiDiagnosticMessageCategory::Performance;
	}

	const char* const message = callbackData->pMessage != nullptr ? callbackData->pMessage : "";
	if (sparkleSeverity == ERhiDiagnosticMessageSeverity::Error)
	{
		SPDLOG_LOGGER_ERROR(g_vulkanRhiLogger, "{}", message);
	}
	else if (sparkleSeverity == ERhiDiagnosticMessageSeverity::Warning)
	{
		SPDLOG_LOGGER_WARN(g_vulkanRhiLogger, "{}", message);
	}
	rhi->PushDiagnosticMessage(sparkleSeverity, sparkleCategory, message);
	return VK_FALSE;
}

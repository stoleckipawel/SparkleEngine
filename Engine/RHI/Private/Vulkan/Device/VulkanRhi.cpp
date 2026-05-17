#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Device/VulkanRhi.h"

#include "CVars/RHICVars.h"
#include "Vulkan/Core/VulkanResult.h"
#include "Vulkan/Diagnostics/VulkanDebugNames.h"

#include "Core/Public/Diagnostics/Trace.h"

#include <algorithm>
#include <array>
#include <format>
#include <limits>
#include <utility>

static const auto g_vulkanRhiLogger = Logging::GetOrCreateLogger("RHI.Vulkan");

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

	if (m_debugMessenger != VK_NULL_HANDLE && m_destroyDebugUtilsMessenger != nullptr)
	{
		m_destroyDebugUtilsMessenger(m_instance, m_debugMessenger, nullptr);
		m_debugMessenger = VK_NULL_HANDLE;
	}

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
	if (m_instance == VK_NULL_HANDLE)
	{
		return;
	}

	auto createDebugUtilsMessenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
	    vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT"));
	m_destroyDebugUtilsMessenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
	    vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT"));
	if (createDebugUtilsMessenger == nullptr || m_destroyDebugUtilsMessenger == nullptr)
	{
		return;
	}

#if ENGINE_GPU_VALIDATION
	const VkDebugUtilsMessengerCreateInfoEXT createInfo{
	    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
	    .pNext = nullptr,
	    .flags = 0,
	    .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
	                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
	    .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
	                   VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
	    .pfnUserCallback = &VulkanRhi::DebugUtilsCallback,
	    .pUserData = this};

	const VkResult result = createDebugUtilsMessenger(m_instance, &createInfo, nullptr, &m_debugMessenger);
	if (!VulkanResult::Succeeded(result))
	{
		SPDLOG_LOGGER_WARN(g_vulkanRhiLogger, "vkCreateDebugUtilsMessengerEXT failed: {}", VulkanResult::ToString(result));
	}
#endif
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
	for (const char* extension : deviceExtensions)
	{
		m_enabledDeviceExtensions.emplace_back(extension);
	}

	VkPhysicalDeviceFeatures2 enabledFeatures{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
	VkPhysicalDeviceVulkan13Features enabledFeatures13{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
	if (m_adapterInfo.ApiVersion >= VK_API_VERSION_1_3)
	{
		enabledFeatures13.synchronization2 = m_featureStatus.SupportsSynchronization2 ? VK_TRUE : VK_FALSE;
		enabledFeatures13.dynamicRendering = m_featureStatus.SupportsDynamicRendering ? VK_TRUE : VK_FALSE;
		enabledFeatures.pNext = &enabledFeatures13;
		m_featureStatus.EnabledSynchronization2 = enabledFeatures13.synchronization2 == VK_TRUE;
		m_featureStatus.EnabledDynamicRendering = enabledFeatures13.dynamicRendering == VK_TRUE;
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
	    "Vulkan features: validation={}, synchronization2 supported/enabled={}/{}, dynamicRendering supported/enabled={}/{}",
	    m_validationEnabled,
	    m_featureStatus.SupportsSynchronization2,
	    m_featureStatus.EnabledSynchronization2,
	    m_featureStatus.SupportsDynamicRendering,
	    m_featureStatus.EnabledDynamicRendering);
	const std::string adapterSummary = std::format(
	    "Vulkan adapter: name='{}', api={}, driver={}, queueFamily={}",
	    m_adapterInfo.Name,
	    FormatApiVersion(m_adapterInfo.ApiVersion),
	    m_adapterInfo.Driver,
	    m_graphicsQueueFamilyIndex);

	SPDLOG_LOGGER_INFO(g_vulkanRhiLogger, "{}", adapterSummary);
	SPDLOG_LOGGER_INFO(g_vulkanRhiLogger, "{}", featureSummary);
	SPDLOG_LOGGER_INFO(g_vulkanRhiLogger, "{}", instanceExtensions);
	SPDLOG_LOGGER_INFO(g_vulkanRhiLogger, "{}", deviceExtensions);
	PushDiagnosticMessage(ERhiDiagnosticMessageSeverity::Info, ERhiDiagnosticMessageCategory::Driver, adapterSummary);
	PushDiagnosticMessage(ERhiDiagnosticMessageSeverity::Info, ERhiDiagnosticMessageCategory::Validation, featureSummary);
	PushDiagnosticMessage(ERhiDiagnosticMessageSeverity::Info, ERhiDiagnosticMessageCategory::Driver, instanceExtensions);
	PushDiagnosticMessage(ERhiDiagnosticMessageSeverity::Info, ERhiDiagnosticMessageCategory::Driver, deviceExtensions);
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

	rhi->PushDiagnosticMessage(sparkleSeverity, sparkleCategory, callbackData->pMessage != nullptr ? callbackData->pMessage : "");
	return VK_FALSE;
}
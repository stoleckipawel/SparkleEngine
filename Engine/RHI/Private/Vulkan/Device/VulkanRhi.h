#pragma once

#include "Vulkan/VulkanIncludes.h"

#include "Diagnostics/RhiDiagnostics.h"
#include "RayTracing/RhiRayTracingDesc.h"

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

struct VulkanAdapterInfo final
{
	std::string Name;
	std::string Driver;
	std::string Vendor;
	std::uint32_t VendorId = 0;
	std::uint32_t DeviceId = 0;
	std::uint32_t ApiVersion = 0;
	std::uint32_t DriverVersion = 0;
	VkPhysicalDeviceType DeviceType = VK_PHYSICAL_DEVICE_TYPE_OTHER;
};

struct VulkanFeatureStatus final
{
	bool SupportsSynchronization2 = false;
	bool SupportsDynamicRendering = false;
	bool EnabledSynchronization2 = false;
	bool EnabledDynamicRendering = false;
};

class VulkanRhi final
{
  public:
	VulkanRhi() noexcept;
	~VulkanRhi() noexcept;

	VulkanRhi(const VulkanRhi&) = delete;
	VulkanRhi& operator=(const VulkanRhi&) = delete;
	VulkanRhi(VulkanRhi&&) = delete;
	VulkanRhi& operator=(VulkanRhi&&) = delete;

	void WaitForIdle() noexcept;
	bool TryPopDiagnosticMessage(RhiDiagnosticMessage& outMessage) noexcept;
	void ClearDiagnosticMessages() noexcept;

	VkInstance GetInstance() const noexcept { return m_instance; }
	VkPhysicalDevice GetPhysicalDevice() const noexcept { return m_physicalDevice; }
	VkDevice GetDevice() const noexcept { return m_device; }
	VkQueue GetGraphicsQueue() const noexcept { return m_graphicsQueue; }
	std::uint32_t GetGraphicsQueueFamilyIndex() const noexcept { return m_graphicsQueueFamilyIndex; }
	PFN_vkSetDebugUtilsObjectNameEXT GetSetDebugUtilsObjectName() const noexcept { return m_setDebugUtilsObjectName; }
	PFN_vkCmdBeginDebugUtilsLabelEXT GetCmdBeginDebugUtilsLabel() const noexcept { return m_cmdBeginDebugUtilsLabel; }
	PFN_vkCmdEndDebugUtilsLabelEXT GetCmdEndDebugUtilsLabel() const noexcept { return m_cmdEndDebugUtilsLabel; }
	PFN_vkCmdInsertDebugUtilsLabelEXT GetCmdInsertDebugUtilsLabel() const noexcept { return m_cmdInsertDebugUtilsLabel; }
	const VulkanAdapterInfo& GetAdapterInfo() const noexcept { return m_adapterInfo; }
	const VulkanFeatureStatus& GetFeatureStatus() const noexcept { return m_featureStatus; }
	const std::vector<std::string>& GetEnabledInstanceExtensions() const noexcept { return m_enabledInstanceExtensions; }
	const std::vector<std::string>& GetEnabledDeviceExtensions() const noexcept { return m_enabledDeviceExtensions; }
	bool IsValidationEnabled() const noexcept { return m_validationEnabled; }
	RhiRayTracingCapabilities GetRayTracingCapabilities() const noexcept { return {}; }

  private:
	struct PhysicalDeviceCandidate final
	{
		VkPhysicalDevice Device = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties Properties = {};
		VkPhysicalDeviceFeatures2 Features = {};
		VkPhysicalDeviceVulkan13Features Features13 = {};
		std::uint32_t GraphicsQueueFamilyIndex = UINT32_MAX;
		std::uint32_t Score = 0;
	};

	void CreateInstance() noexcept;
	void CreateDebugMessenger() noexcept;
	void SelectPhysicalDevice() noexcept;
	void CreateLogicalDevice() noexcept;
	void LoadDeviceDebugFunctions() noexcept;
	void NameBootstrapObjects() noexcept;
	void LogBootstrapSummary() noexcept;
	void PushDiagnosticMessage(ERhiDiagnosticMessageSeverity severity, ERhiDiagnosticMessageCategory category, std::string text) noexcept;

	static bool IsLayerAvailable(const char* layerName) noexcept;
	static bool IsInstanceExtensionAvailable(const char* extensionName) noexcept;
	static bool IsDeviceExtensionAvailable(VkPhysicalDevice device, const char* extensionName) noexcept;
	static std::uint32_t FindGraphicsQueueFamily(VkPhysicalDevice device) noexcept;
	static std::uint32_t ScorePhysicalDevice(const VkPhysicalDeviceProperties& properties) noexcept;
	static VulkanAdapterInfo BuildAdapterInfo(const VkPhysicalDeviceProperties& properties);
	static std::string FormatApiVersion(std::uint32_t version);
	static std::string FormatDriverVersion(const VkPhysicalDeviceProperties& properties);
	static std::string PhysicalDeviceTypeToString(VkPhysicalDeviceType type);
	static VkBool32 VKAPI_PTR DebugUtilsCallback(
	    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
	    void* userData) noexcept;

	VkInstance m_instance = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkDevice m_device = VK_NULL_HANDLE;
	VkQueue m_graphicsQueue = VK_NULL_HANDLE;
	std::uint32_t m_graphicsQueueFamilyIndex = UINT32_MAX;
	PFN_vkSetDebugUtilsObjectNameEXT m_setDebugUtilsObjectName = nullptr;
	PFN_vkCmdBeginDebugUtilsLabelEXT m_cmdBeginDebugUtilsLabel = nullptr;
	PFN_vkCmdEndDebugUtilsLabelEXT m_cmdEndDebugUtilsLabel = nullptr;
	PFN_vkCmdInsertDebugUtilsLabelEXT m_cmdInsertDebugUtilsLabel = nullptr;
	PFN_vkDestroyDebugUtilsMessengerEXT m_destroyDebugUtilsMessenger = nullptr;
	VulkanAdapterInfo m_adapterInfo;
	VulkanFeatureStatus m_featureStatus;
	std::vector<std::string> m_enabledInstanceExtensions;
	std::vector<std::string> m_enabledDeviceExtensions;
	std::vector<std::string> m_enabledLayers;
	std::vector<RhiDiagnosticMessage> m_diagnosticMessages;
	std::mutex m_diagnosticMessagesMutex;
	bool m_validationEnabled = false;
};
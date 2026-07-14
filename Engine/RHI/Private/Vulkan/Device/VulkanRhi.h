#pragma once

#include "Vulkan/VulkanIncludes.h"

#include "Diagnostics/RhiDiagnostics.h"
#include "RayTracing/RhiRayTracingDesc.h"
#include "Vulkan/Diagnostics/VulkanDebugLayer.h"
#include "Vulkan/Device/VulkanRayTracingFeatureQuery.h"

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
	bool SupportsSamplerAnisotropy = false;
	bool SupportsFillModeNonSolid = false;
	bool SupportsShaderInt64 = false;
	bool SupportsStorageImageReadWithoutFormat = false;
	bool SupportsStorageImageWriteWithoutFormat = false;
	bool SupportsSampledImageArrayNonUniformIndexing = false;
	bool SupportsShaderDemoteToHelperInvocation = false;
	bool SupportsMutableDescriptorType = false;
	bool EnabledSynchronization2 = false;
	bool EnabledDynamicRendering = false;
	bool EnabledSamplerAnisotropy = false;
	bool EnabledFillModeNonSolid = false;
	bool EnabledShaderInt64 = false;
	bool EnabledStorageImageReadWithoutFormat = false;
	bool EnabledStorageImageWriteWithoutFormat = false;
	bool EnabledSampledImageArrayNonUniformIndexing = false;
	bool EnabledShaderDemoteToHelperInvocation = false;
	bool EnabledMutableDescriptorType = false;
	VulkanRayTracingFeatureStatus RayTracing;
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

	VkInstance GetInstance() const noexcept;
	VkPhysicalDevice GetPhysicalDevice() const noexcept;
	VkDevice GetDevice() const noexcept;
	VkQueue GetGraphicsQueue() const noexcept;
	std::uint32_t GetGraphicsQueueFamilyIndex() const noexcept;
	PFN_vkSetDebugUtilsObjectNameEXT GetSetDebugUtilsObjectName() const noexcept;
	PFN_vkCmdBeginDebugUtilsLabelEXT GetCmdBeginDebugUtilsLabel() const noexcept;
	PFN_vkCmdEndDebugUtilsLabelEXT GetCmdEndDebugUtilsLabel() const noexcept;
	PFN_vkCmdInsertDebugUtilsLabelEXT GetCmdInsertDebugUtilsLabel() const noexcept;
	const VulkanAdapterInfo& GetAdapterInfo() const noexcept;
	const VulkanFeatureStatus& GetFeatureStatus() const noexcept;
	const std::vector<std::string>& GetEnabledInstanceExtensions() const noexcept;
	const std::vector<std::string>& GetEnabledDeviceExtensions() const noexcept;
	bool IsValidationEnabled() const noexcept;
	RhiRayTracingCapabilities GetRayTracingCapabilities() const noexcept;
	PFN_vkGetBufferDeviceAddress GetGetBufferDeviceAddress() const noexcept;
	PFN_vkCreateAccelerationStructureKHR GetCreateAccelerationStructure() const noexcept;
	PFN_vkDestroyAccelerationStructureKHR GetDestroyAccelerationStructure() const noexcept;
	PFN_vkGetAccelerationStructureBuildSizesKHR GetAccelerationStructureBuildSizes() const noexcept;
	PFN_vkCmdBuildAccelerationStructuresKHR GetCmdBuildAccelerationStructures() const noexcept;
	PFN_vkGetAccelerationStructureDeviceAddressKHR GetAccelerationStructureDeviceAddress() const noexcept;
	PFN_vkGetPartitionedAccelerationStructuresBuildSizesNV GetPartitionedAccelerationStructureBuildSizes() const noexcept;
	PFN_vkCmdBuildPartitionedAccelerationStructuresNV GetCmdBuildPartitionedAccelerationStructures() const noexcept;

  private:
	struct PhysicalDeviceCandidate final
	{
		VkPhysicalDevice Device = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties Properties = {};
		VkPhysicalDeviceFeatures2 Features = {};
		VkPhysicalDeviceVulkan12Features Features12 = {};
		VkPhysicalDeviceVulkan13Features Features13 = {};
		std::uint32_t GraphicsQueueFamilyIndex = UINT32_MAX;
		std::uint32_t Score = 0;
	};

	void CreateInstance() noexcept;
	void CreateDebugMessenger() noexcept;
	void SelectPhysicalDevice() noexcept;
	void CreateLogicalDevice() noexcept;
	void LoadDeviceDebugFunctions() noexcept;
	void LoadRayTracingFunctions() noexcept;
	void BuildRayTracingCapabilities() noexcept;
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
	VulkanDebugLayer m_debugLayer;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkDevice m_device = VK_NULL_HANDLE;
	VkQueue m_graphicsQueue = VK_NULL_HANDLE;
	std::uint32_t m_graphicsQueueFamilyIndex = UINT32_MAX;
	PFN_vkSetDebugUtilsObjectNameEXT m_setDebugUtilsObjectName = nullptr;
	PFN_vkCmdBeginDebugUtilsLabelEXT m_cmdBeginDebugUtilsLabel = nullptr;
	PFN_vkCmdEndDebugUtilsLabelEXT m_cmdEndDebugUtilsLabel = nullptr;
	PFN_vkCmdInsertDebugUtilsLabelEXT m_cmdInsertDebugUtilsLabel = nullptr;
	VulkanAdapterInfo m_adapterInfo;
	VulkanFeatureStatus m_featureStatus;
	RhiRayTracingCapabilities m_rayTracingCapabilities;
	PFN_vkGetBufferDeviceAddress m_getBufferDeviceAddress = nullptr;
	PFN_vkCreateAccelerationStructureKHR m_createAccelerationStructure = nullptr;
	PFN_vkDestroyAccelerationStructureKHR m_destroyAccelerationStructure = nullptr;
	PFN_vkGetAccelerationStructureBuildSizesKHR m_getAccelerationStructureBuildSizes = nullptr;
	PFN_vkCmdBuildAccelerationStructuresKHR m_cmdBuildAccelerationStructures = nullptr;
	PFN_vkGetAccelerationStructureDeviceAddressKHR m_getAccelerationStructureDeviceAddress = nullptr;
	PFN_vkGetPartitionedAccelerationStructuresBuildSizesNV m_getPartitionedAccelerationStructureBuildSizes = nullptr;
	PFN_vkCmdBuildPartitionedAccelerationStructuresNV m_cmdBuildPartitionedAccelerationStructures = nullptr;
	std::vector<std::string> m_enabledInstanceExtensions;
	std::vector<std::string> m_enabledDeviceExtensions;
	std::vector<std::string> m_enabledLayers;
	std::vector<RhiDiagnosticMessage> m_diagnosticMessages;
	std::mutex m_diagnosticMessagesMutex;
	bool m_validationEnabled = false;
};

#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Device/VulkanExternalFeatureInteropCapabilities.h"

#include "Vulkan/Device/VulkanRhi.h"

namespace
{
	RhiAdapterIdentity BuildVulkanAdapterIdentity(const VulkanRhi* rhi)
	{
		if (rhi == nullptr)
		{
			return {};
		}

		const VulkanAdapterInfo& adapter = rhi->GetAdapterInfo();
		return RhiAdapterIdentity{
		    .Name = adapter.Name,
		    .DriverDescription = adapter.Driver,
		    .VendorId = adapter.VendorId,
		    .DeviceId = adapter.DeviceId};
	}
}

RhiExternalFeatureInteropCapabilities BuildVulkanExternalFeatureInteropCapabilities(
    const VulkanRhi* rhi,
    bool hasGraphicsCommandContext) noexcept
{
	RhiExternalFeatureInteropCapabilities capabilities{};
	capabilities.BridgeKind = ERhiExternalFeatureBridgeKind::VulkanManualFunctionPointers;
	capabilities.Adapter = BuildVulkanAdapterIdentity(rhi);
	if (rhi != nullptr)
	{
		capabilities.VulkanHasInstanceHandle = rhi->GetInstance() != VK_NULL_HANDLE;
		capabilities.VulkanHasPhysicalDeviceHandle = rhi->GetPhysicalDevice() != VK_NULL_HANDLE;
		capabilities.VulkanHasDeviceHandle = rhi->GetDevice() != VK_NULL_HANDLE;
		capabilities.VulkanHasGraphicsQueueHandle = rhi->GetGraphicsQueue() != VK_NULL_HANDLE;
		capabilities.VulkanHasGraphicsQueueFamilyIndex = rhi->GetGraphicsQueueFamilyIndex() != UINT32_MAX;
	}

	capabilities.ExposesNativeDevice = capabilities.VulkanHasDeviceHandle;
	capabilities.ExposesNativeGraphicsQueue = capabilities.VulkanHasGraphicsQueueHandle;
	capabilities.ExposesNativeGraphicsCommandList = hasGraphicsCommandContext;
	capabilities.ExposesNativeResources = true;
	capabilities.SupportsExplicitResourceStates = true;
	capabilities.VulkanManualFunctionPointerHookingReady =
	    capabilities.VulkanHasInstanceHandle && capabilities.VulkanHasPhysicalDeviceHandle && capabilities.VulkanHasDeviceHandle &&
	    capabilities.VulkanHasGraphicsQueueHandle && capabilities.VulkanHasGraphicsQueueFamilyIndex && hasGraphicsCommandContext;
	capabilities.VulkanInterposerRequired = false;
	capabilities.SupportsExternalProviderEvaluation =
	    capabilities.VulkanManualFunctionPointerHookingReady && capabilities.ExposesNativeResources;
	capabilities.SupportsRuntimeProviderChecks = capabilities.SupportsExternalProviderEvaluation;
	return capabilities;
}

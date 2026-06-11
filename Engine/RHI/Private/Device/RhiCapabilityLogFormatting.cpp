#include "PCH.h"

#include "Device/RhiCapabilityLogFormatting.h"

#include <format>

std::string FormatExternalFeatureInteropCapabilities(const RhiExternalFeatureInteropCapabilities& capabilities)
{
	return std::format(
	    "externalFeatureInterop(bridge={}, adapter='{}', vendorId={:#06x}, deviceId={:#06x}, driver='{}', nativeDevice={}, "
	    "nativeQueue={}, nativeCommandList={}, nativeResources={}, explicitStates={}, providerEval={}, runtimeChecks={}, "
	    "vk(instance={}, physicalDevice={}, device={}, queue={}, queueFamily={}, manualHookReady={}, interposerRequired={}))",
	    RhiExternalFeatureBridgeKindToString(capabilities.BridgeKind),
	    capabilities.Adapter.Name,
	    capabilities.Adapter.VendorId,
	    capabilities.Adapter.DeviceId,
	    capabilities.Adapter.DriverDescription,
	    capabilities.ExposesNativeDevice,
	    capabilities.ExposesNativeGraphicsQueue,
	    capabilities.ExposesNativeGraphicsCommandList,
	    capabilities.ExposesNativeResources,
	    capabilities.SupportsExplicitResourceStates,
	    capabilities.SupportsExternalProviderEvaluation,
	    capabilities.SupportsRuntimeProviderChecks,
	    capabilities.VulkanHasInstanceHandle,
	    capabilities.VulkanHasPhysicalDeviceHandle,
	    capabilities.VulkanHasDeviceHandle,
	    capabilities.VulkanHasGraphicsQueueHandle,
	    capabilities.VulkanHasGraphicsQueueFamilyIndex,
	    capabilities.VulkanManualFunctionPointerHookingReady,
	    capabilities.VulkanInterposerRequired);
}


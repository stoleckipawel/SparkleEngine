#include "../PCH.h"
#include "Streamline/StreamlineRuntimeSupport.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
#include <vulkan/vulkan.h>
#include <sl_helpers.h>
#include <sl_helpers_vk.h>

#include <format>

bool HasStreamlineNativeAdapterLuid(const RhiAdapterIdentity& adapter) noexcept
{
	return adapter.NativeLuidSizeInBytes > 0 && adapter.NativeLuidSizeInBytes <= adapter.NativeLuid.size();
}

StreamlineBackendContract ValidateStreamlineBackend(
    const RhiCapabilities& capabilities,
    RhiNativeDeviceQueueInterop nativeInterop)
{
	const bool isD3D12 = capabilities.BackendApi == ERhiBackendApi::D3D12;
	const bool isVulkan = capabilities.BackendApi == ERhiBackendApi::Vulkan;
	if (!isD3D12 && !isVulkan)
	{
		return StreamlineBackendContract{
		    .Valid = false,
		    .FailureReason = "Streamline is only implemented for D3D12 and Vulkan backends."};
	}
	if (isD3D12 && !nativeInterop)
	{
		return StreamlineBackendContract{
		    .Valid = false,
		    .UsesD3D12 = true,
		    .FailureReason = "D3D12 native device or graphics queue handle is unavailable."};
	}
	if (isVulkan && !nativeInterop.Vulkan)
	{
		return StreamlineBackendContract{
		    .Valid = false,
		    .UsesVulkan = true,
		    .FailureReason = "Vulkan native instance, physical device, device, graphics queue, or graphics queue family is unavailable."};
	}
	if (isD3D12 && !HasStreamlineNativeAdapterLuid(capabilities.ExternalFeatureInterop.Adapter))
	{
		return StreamlineBackendContract{
		    .Valid = false,
		    .UsesD3D12 = true,
		    .FailureReason = "D3D12 adapter native LUID is unavailable for Streamline feature support query."};
	}

	return StreamlineBackendContract{
	    .Valid = true,
	    .UsesD3D12 = isD3D12,
	    .UsesVulkan = isVulkan};
}

sl::Result SetStreamlineNativeDevice(
    const StreamlineBackendContract& backend,
    RhiNativeDeviceQueueInterop nativeInterop) noexcept
{
	if (backend.UsesD3D12)
	{
		return slSetD3DDevice(nativeInterop.Device.Value);
	}

	sl::VulkanInfo vulkanInfo{};
	vulkanInfo.instance = static_cast<VkInstance>(nativeInterop.Vulkan.Instance);
	vulkanInfo.physicalDevice = static_cast<VkPhysicalDevice>(nativeInterop.Vulkan.PhysicalDevice);
	vulkanInfo.device = static_cast<VkDevice>(nativeInterop.Vulkan.Device);
	vulkanInfo.graphicsQueueFamily = nativeInterop.Vulkan.GraphicsQueueFamilyIndex;
	vulkanInfo.computeQueueFamily = nativeInterop.Vulkan.GraphicsQueueFamilyIndex;
	vulkanInfo.graphicsQueueIndex = 0;
	vulkanInfo.computeQueueIndex = 0;
	return slSetVulkanInfo(vulkanInfo);
}

StreamlineAdapterInfo BuildStreamlineAdapterInfo(
    const StreamlineBackendContract& backend,
    const RhiCapabilities& capabilities,
    RhiNativeDeviceQueueInterop nativeInterop) noexcept
{
	StreamlineAdapterInfo adapterInfo{};
	if (backend.UsesVulkan)
	{
		adapterInfo.Info.vkPhysicalDevice = nativeInterop.Vulkan.PhysicalDevice;
		return adapterInfo;
	}

	adapterInfo.LuidStorage = capabilities.ExternalFeatureInterop.Adapter.NativeLuid;
	adapterInfo.Info.deviceLUID = adapterInfo.LuidStorage.data();
	adapterInfo.Info.deviceLUIDSizeInBytes = capabilities.ExternalFeatureInterop.Adapter.NativeLuidSizeInBytes;
	return adapterInfo;
}

std::string FormatStreamlineFailure(std::string_view operation, sl::Result result)
{
	return std::format("{} failed: {}", operation, sl::getResultAsStr(result));
}

bool UpgradePresentationInterfaceWithStreamline(void** nativeInterface, void*) noexcept
{
	return nativeInterface != nullptr && slUpgradeInterface(nativeInterface) == sl::Result::eOk;
}

void FillStreamlinePreferences(
    sl::Preferences& preferences,
    const sl::Feature* features,
    std::uint32_t featureCount,
    sl::RenderAPI renderApi)
{
	preferences = {};
	preferences.showConsole = false;
	preferences.logLevel = sl::LogLevel::eDefault;
	preferences.featuresToLoad = features;
	preferences.numFeaturesToLoad = featureCount;
	preferences.flags = sl::PreferenceFlags::eDisableCLStateTracking | sl::PreferenceFlags::eUseManualHooking |
	                    sl::PreferenceFlags::eUseFrameBasedResourceTagging | sl::PreferenceFlags::eAllowOTA |
	                    sl::PreferenceFlags::eLoadDownloadedPlugins;
	preferences.applicationId = 0;
	preferences.engine = sl::EngineType::eCustom;
	preferences.engineVersion = "SparkleEngine-Development";
	preferences.projectId = "535041524B4C45454E47494E45303031";
	preferences.renderAPI = renderApi;
}
#endif

#include "../PCH.h"
#include "Streamline/StreamlineRuntimeSupport.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
#include <vulkan/vulkan.h>
#include <sl_helpers.h>
#include <sl_helpers_vk.h>

#include <filesystem>
#include <format>
#include <system_error>

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
	if (isD3D12 && !nativeInterop.Device)
	{
		return StreamlineBackendContract{
		    .Valid = false,
		    .UsesD3D12 = true,
		    .FailureReason = "D3D12 native device handle is unavailable."};
	}
	if (isVulkan && (!capabilities.ExternalFeatureInterop.VulkanInstance ||
	                 !capabilities.ExternalFeatureInterop.VulkanPhysicalDevice ||
	                 !capabilities.ExternalFeatureInterop.VulkanDevice ||
	                 capabilities.ExternalFeatureInterop.VulkanGraphicsQueueFamilyIndex == UINT32_MAX))
	{
		return StreamlineBackendContract{
		    .Valid = false,
		    .UsesVulkan = true,
		    .FailureReason = "Vulkan native instance, physical device, device, or graphics queue family is unavailable."};
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
    const RhiCapabilities& capabilities,
    RhiNativeDeviceQueueInterop nativeInterop) noexcept
{
	if (backend.UsesD3D12)
	{
		return slSetD3DDevice(nativeInterop.Device.Value);
	}

	sl::VulkanInfo vulkanInfo{};
	vulkanInfo.instance = static_cast<VkInstance>(capabilities.ExternalFeatureInterop.VulkanInstance);
	vulkanInfo.physicalDevice = static_cast<VkPhysicalDevice>(capabilities.ExternalFeatureInterop.VulkanPhysicalDevice);
	vulkanInfo.device = static_cast<VkDevice>(capabilities.ExternalFeatureInterop.VulkanDevice);
	vulkanInfo.graphicsQueueFamily = capabilities.ExternalFeatureInterop.VulkanGraphicsQueueFamilyIndex;
	vulkanInfo.computeQueueFamily = capabilities.ExternalFeatureInterop.VulkanGraphicsQueueFamilyIndex;
	vulkanInfo.graphicsQueueIndex = 0;
	vulkanInfo.computeQueueIndex = 0;
	return slSetVulkanInfo(vulkanInfo);
}

StreamlineAdapterInfo BuildStreamlineAdapterInfo(
    const StreamlineBackendContract& backend,
    const RhiCapabilities& capabilities) noexcept
{
	StreamlineAdapterInfo adapterInfo{};
	if (backend.UsesVulkan)
	{
		adapterInfo.Info.vkPhysicalDevice = capabilities.ExternalFeatureInterop.VulkanPhysicalDevice;
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
    const StreamlinePreferencesDesc& desc,
    std::wstring& outLogPath)
{
	preferences = {};
	preferences.showConsole = false;
	preferences.logLevel = desc.DiagnosticsEnabled ? sl::LogLevel::eVerbose : sl::LogLevel::eDefault;
	preferences.featuresToLoad = features;
	preferences.numFeaturesToLoad = featureCount;
	preferences.flags = sl::PreferenceFlags::eDisableCLStateTracking | sl::PreferenceFlags::eUseManualHooking |
	                    sl::PreferenceFlags::eUseFrameBasedResourceTagging | sl::PreferenceFlags::eAllowOTA |
	                    sl::PreferenceFlags::eLoadDownloadedPlugins;
	preferences.applicationId = desc.ApplicationId;
	preferences.engine = sl::EngineType::eCustom;
	preferences.engineVersion = "SparkleEngine-Development";
	preferences.projectId = "535041524B4C45454E47494E45303031";
	preferences.renderAPI = desc.RenderApi;

	std::error_code logPathError;
	outLogPath = (std::filesystem::current_path(logPathError) / ".." / ".." / "logs" / "Streamline").lexically_normal().wstring();
	if (!logPathError)
	{
		std::filesystem::create_directories(outLogPath, logPathError);
		if (!logPathError)
		{
			preferences.pathToLogsAndData = outLogPath.c_str();
		}
	}
}
#endif

#include "../PCH.h"
#include "Streamline/StreamlineRuntimeSupport.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
#include <sl_dlss.h>
#include <sl_dlss_d.h>
#include <vulkan/vulkan.h>
#include <sl_helpers.h>
#include <sl_helpers_vk.h>

#include <format>
#include <mutex>

namespace
{
	std::mutex g_sharedStreamlineRuntimeMutex;
	bool g_sharedStreamlineRuntimeInitialized = false;
	std::uint32_t g_sharedStreamlineRuntimeRefCount = 0;
	sl::RenderAPI g_sharedStreamlineRenderApi = sl::RenderAPI::eD3D12;
}

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

#if SPARKLE_WITH_NVIDIA_STREAMLINE
SharedStreamlineRuntimeSession AcquireSharedStreamlineRuntime(
    const StreamlineBackendContract& backend,
    RhiNativeDeviceQueueInterop nativeInterop)
{
	const sl::RenderAPI renderApi = backend.UsesVulkan ? sl::RenderAPI::eVulkan : sl::RenderAPI::eD3D12;

	std::lock_guard lock(g_sharedStreamlineRuntimeMutex);
	if (g_sharedStreamlineRuntimeInitialized)
	{
		if (g_sharedStreamlineRenderApi != renderApi)
		{
			return SharedStreamlineRuntimeSession{
			    .Succeeded = false,
			    .FailureReason = "Streamline is already initialized for a different render API."};
		}

		++g_sharedStreamlineRuntimeRefCount;
		return SharedStreamlineRuntimeSession{.Succeeded = true};
	}

	const sl::Feature features[] = {sl::kFeatureDLSS, sl::kFeatureDLSS_RR};
	sl::Preferences preferences{};
	FillStreamlinePreferences(preferences, features, 2u, renderApi);

	const sl::Result result = slInit(preferences, sl::kSDKVersion);
	if (result != sl::Result::eOk)
	{
		return SharedStreamlineRuntimeSession{
		    .Succeeded = false,
		    .FailureReason = FormatStreamlineFailure("slInit(Streamline shared runtime)", result)};
	}

	const sl::Result nativeDeviceResult = SetStreamlineNativeDevice(backend, nativeInterop);
	if (nativeDeviceResult != sl::Result::eOk)
	{
		(void) slShutdown();
		return SharedStreamlineRuntimeSession{
		    .Succeeded = false,
		    .FailureReason = FormatStreamlineFailure("Streamline native device setup", nativeDeviceResult)};
	}

	g_sharedStreamlineRuntimeInitialized = true;
	g_sharedStreamlineRuntimeRefCount = 1;
	g_sharedStreamlineRenderApi = renderApi;
	return SharedStreamlineRuntimeSession{.Succeeded = true};
}

void ReleaseSharedStreamlineRuntime() noexcept
{
	std::lock_guard lock(g_sharedStreamlineRuntimeMutex);
	if (g_sharedStreamlineRuntimeRefCount > 0)
	{
		--g_sharedStreamlineRuntimeRefCount;
	}
}
#endif

void ShutdownSharedStreamlineRuntime() noexcept
{
#if SPARKLE_WITH_NVIDIA_STREAMLINE
	std::lock_guard lock(g_sharedStreamlineRuntimeMutex);
	if (g_sharedStreamlineRuntimeInitialized)
	{
		(void) slShutdown();
		g_sharedStreamlineRuntimeInitialized = false;
		g_sharedStreamlineRuntimeRefCount = 0;
	}
#endif
}

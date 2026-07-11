#include "../PCH.h"
#include "Streamline/StreamlineRuntimeSupport.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
	#include <sl_dlss.h>
	#include <sl_dlss_d.h>
	#include <sl_helpers.h>

	#include <mutex>

namespace
{
	std::mutex g_streamlineMutex;
	bool g_streamlineInitialized = false;
	bool g_streamlineDeviceBound = false;
	bool g_streamlinePresentationReady = false;

	bool RuntimeReady() noexcept
	{
		return g_streamlineInitialized && g_streamlineDeviceBound && g_streamlinePresentationReady;
	}

	bool SetNativeDevice(
	    ERhiBackendApi backendApi,
	    NativeGraphicsDeviceHandle nativeDevice,
	    void*) noexcept
	{
		std::lock_guard lock(g_streamlineMutex);
		if (!g_streamlineInitialized || backendApi != ERhiBackendApi::D3D12 || !nativeDevice)
		{
			return false;
		}

		g_streamlineDeviceBound = slSetD3DDevice(nativeDevice.Value) == sl::Result::eOk;
		return g_streamlineDeviceBound;
	}

	bool UpgradeInterface(
	    ERhiBackendApi backendApi,
	    ERhiExternalInterfaceKind,
	    void** nativeInterface,
	    void*) noexcept
	{
		std::lock_guard lock(g_streamlineMutex);
		return g_streamlineInitialized && g_streamlineDeviceBound && backendApi == ERhiBackendApi::D3D12 &&
		       nativeInterface != nullptr && slUpgradeInterface(nativeInterface) == sl::Result::eOk;
	}

	bool ResolveNativeInterface(
	    ERhiBackendApi backendApi,
	    ERhiExternalInterfaceKind,
	    void* externalInterface,
	    void** nativeInterface,
	    void*) noexcept
	{
		std::lock_guard lock(g_streamlineMutex);
		return g_streamlineInitialized && backendApi == ERhiBackendApi::D3D12 &&
		       externalInterface != nullptr && nativeInterface != nullptr &&
		       slGetNativeInterface(externalInterface, nativeInterface) == sl::Result::eOk;
	}

	void SetPresentationReady(ERhiBackendApi backendApi, bool ready, void*) noexcept
	{
		std::lock_guard lock(g_streamlineMutex);
		g_streamlinePresentationReady =
		    backendApi == ERhiBackendApi::D3D12 && g_streamlineInitialized && g_streamlineDeviceBound && ready;
	}

	void FillPreferences(sl::Preferences& preferences)
	{
		static constexpr sl::Feature features[] = {sl::kFeatureDLSS, sl::kFeatureDLSS_RR};
		preferences = {};
		preferences.featuresToLoad = features;
		preferences.numFeaturesToLoad = static_cast<std::uint32_t>(std::size(features));
		preferences.flags = sl::PreferenceFlags::eDisableCLStateTracking | sl::PreferenceFlags::eUseManualHooking |
		                    sl::PreferenceFlags::eUseFrameBasedResourceTagging | sl::PreferenceFlags::eAllowOTA |
		                    sl::PreferenceFlags::eLoadDownloadedPlugins;
		preferences.engine = sl::EngineType::eCustom;
		preferences.engineVersion = "SparkleEngine-Development";
		preferences.projectId = "535041524B4C45454E47494E45303031";
		preferences.renderAPI = sl::RenderAPI::eD3D12;
	}

	bool HasAdapterLuid(const RhiAdapterIdentity& adapter) noexcept
	{
		return adapter.NativeLuidSizeInBytes > 0u && adapter.NativeLuidSizeInBytes <= adapter.NativeLuid.size();
	}

	bool ValidateStreamlineBackend(
	    const RhiCapabilities& capabilities,
	    RhiNativeDeviceQueueInterop nativeInterop) noexcept
	{
		const bool isD3D12 = capabilities.BackendApi == ERhiBackendApi::D3D12;
		const RhiExternalFeatureInteropCapabilities& interop = capabilities.ExternalFeatureInterop;
		const bool commonInterop = interop.ExposesNativeDevice && interop.ExposesNativeGraphicsQueue &&
		                           interop.ExposesNativeGraphicsCommandList && interop.ExposesNativeResources &&
		                           interop.SupportsExternalProviderEvaluation;
		return isD3D12 && commonInterop && nativeInterop && HasAdapterLuid(interop.Adapter);
	}
}

bool IsStreamlineFeatureSupported(
    sl::Feature feature,
    const RhiCapabilities& capabilities,
    RhiNativeDeviceQueueInterop nativeInterop) noexcept
{
	if (!ValidateStreamlineBackend(capabilities, nativeInterop))
	{
		return false;
	}

	std::lock_guard lock(g_streamlineMutex);
	if (!RuntimeReady())
	{
		return false;
	}

	const RhiAdapterIdentity& adapter = capabilities.ExternalFeatureInterop.Adapter;
	auto luid = adapter.NativeLuid;
	sl::AdapterInfo adapterInfo{};
	adapterInfo.deviceLUID = luid.data();
	adapterInfo.deviceLUIDSizeInBytes = adapter.NativeLuidSizeInBytes;
	return slIsFeatureSupported(feature, adapterInfo) == sl::Result::eOk;
}
#endif

bool InitializeSharedStreamlineRuntime(ERhiBackendApi backendApi)
{
#if SPARKLE_WITH_NVIDIA_STREAMLINE
	if (backendApi != ERhiBackendApi::D3D12)
	{
		return false;
	}

	std::lock_guard lock(g_streamlineMutex);
	if (g_streamlineInitialized)
	{
		return true;
	}

	sl::Preferences preferences{};
	FillPreferences(preferences);
	g_streamlineInitialized = slInit(preferences, sl::kSDKVersion) == sl::Result::eOk;
	return g_streamlineInitialized;
#else
	(void) backendApi;
	return false;
#endif
}

RhiExternalFeatureHooks GetSharedStreamlineRhiHooks() noexcept
{
#if SPARKLE_WITH_NVIDIA_STREAMLINE
	std::lock_guard lock(g_streamlineMutex);
	if (!g_streamlineInitialized)
	{
		return {};
	}
	return RhiExternalFeatureHooks{
	    .DeviceCreated = &SetNativeDevice,
	    .UpgradeInterface = &UpgradeInterface,
	    .ResolveNativeInterface = &ResolveNativeInterface,
	    .PresentationReady = &SetPresentationReady};
#else
	return {};
#endif
}

void ShutdownSharedStreamlineRuntime() noexcept
{
#if SPARKLE_WITH_NVIDIA_STREAMLINE
	std::lock_guard lock(g_streamlineMutex);
	if (g_streamlineInitialized)
	{
		(void) slShutdown();
	}
	g_streamlineInitialized = false;
	g_streamlineDeviceBound = false;
	g_streamlinePresentationReady = false;
#endif
}

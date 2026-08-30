#include "../PCH.h"
#include "Streamline/StreamlineRuntimeSupport.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
  #include <sl_dlss.h>
  #include <sl_dlss_d.h>
  #include <sl_helpers.h>
  #include <sl_pcl.h>
  #include <sl_reflex.h>

  #include <condition_variable>
  #include <mutex>

static const auto g_streamlineRuntimeLogger = Logging::GetOrCreateLogger("Renderer.Streamline.Runtime");

class StreamlineRuntime final
{
public:
	static bool Initialize(ERhiBackendApi backendApi)
	{
		if (backendApi != ERhiBackendApi::D3D12)
		{
			return false;
		}

		{
			std::lock_guard lock(s_mutex);
			if (s_initialized)
				return true;
			if (s_initializing || s_shuttingDown)
				return false;
			s_initializing = true;
		}

		sl::Preferences preferences{};
		FillPreferences(preferences);
		const bool initialized = slInit(preferences, sl::kSDKVersion) == sl::Result::eOk;
		{
			std::lock_guard lock(s_mutex);
			s_initializing = false;
			s_initialized = initialized;
		}
		s_idle.notify_all();
		return initialized;
	}

	static RhiInterposerHooks GetInterposerHooks() noexcept
	{
		std::lock_guard lock(s_mutex);
		if (!s_initialized || s_shuttingDown)
		{
			return {};
		}

		return RhiInterposerHooks{
		    .DeviceCreated = &SetNativeDevice,
		    .UpgradeInterface = &UpgradeInterface,
		    .ResolveNativeInterface = &ResolveNativeInterface,
		    .PresentationReady = &SetPresentationReady,
		    .FrameMarker = &SetFrameMarker,
		    .RuntimeShutdown = &ShutdownFromInterposer};
	}

	static void Shutdown() noexcept
	{
		bool initialized = false;
		{
			std::unique_lock lock(s_mutex);
			s_shuttingDown = true;
			s_idle.wait(lock, [] { return s_activeCalls == 0 && !s_initializing; });
			initialized = s_initialized;
		}
		if (initialized)
			(void) slShutdown();
		{
			std::lock_guard lock(s_mutex);
			s_initialized = false;
			s_deviceBound = false;
			s_presentationReady = false;
			s_pclSupported = false;
			s_reflexSupported = false;
			s_shuttingDown = false;
		}
	}

	static bool IsFeatureSupported(
	    sl::Feature feature,
	    const RhiCapabilities& capabilities,
	    RhiNativeDeviceQueueInterop nativeInterop) noexcept
	{
		if (!ValidateBackend(capabilities, nativeInterop))
		{
			return false;
		}

		CallLease call(CallRequirement::RuntimeReady);
		if (!call)
			return false;

		const RhiAdapterIdentity& adapter = capabilities.ExternalFeatureInterop.Adapter;
		auto luid = adapter.NativeLuid;
		sl::AdapterInfo adapterInfo{};
		adapterInfo.deviceLUID = luid.data();
		adapterInfo.deviceLUIDSizeInBytes = adapter.NativeLuidSizeInBytes;
		return slIsFeatureSupported(feature, adapterInfo) == sl::Result::eOk;
	}

	static void SetApplicationFrameMarker(ERhiFrameLatencyMarker marker, std::uint64_t frameId) noexcept
	{
		SetFrameMarker(marker, frameId, nullptr);
	}

private:
	enum class CallRequirement : std::uint8_t
	{
		Initialized,
		DeviceBound,
		RuntimeReady,
	};

	class CallLease final
	{
	public:
		explicit CallLease(CallRequirement requirement) noexcept
		{
			std::lock_guard lock(s_mutex);
			if (!s_shuttingDown && MeetsRequirement(requirement))
			{
				++s_activeCalls;
				m_acquired = true;
			}
		}

		~CallLease() noexcept
		{
			if (!m_acquired)
				return;
			{
				std::lock_guard lock(s_mutex);
				--s_activeCalls;
			}
			s_idle.notify_all();
		}

		explicit operator bool() const noexcept { return m_acquired; }

	private:
		bool m_acquired = false;
	};

	static bool MeetsRequirement(CallRequirement requirement) noexcept
	{
		if (!s_initialized)
			return false;
		if (requirement == CallRequirement::Initialized)
			return true;
		if (!s_deviceBound)
			return false;
		return requirement == CallRequirement::DeviceBound || s_presentationReady;
	}

	static bool SetNativeDevice(NativeGraphicsDeviceHandle nativeDevice, const RhiAdapterIdentity& adapter, void*) noexcept
	{
		CallLease call(CallRequirement::Initialized);
		if (!call || !nativeDevice)
		{
			return false;
		}

		const bool deviceBound = slSetD3DDevice(nativeDevice.Value) == sl::Result::eOk;
		bool pclSupported = false;
		bool reflexSupported = false;
		if (deviceBound && HasAdapterLuid(adapter))
		{
			auto adapterLuid = adapter.NativeLuid;
			sl::AdapterInfo adapterInfo{};
			adapterInfo.deviceLUID = adapterLuid.data();
			adapterInfo.deviceLUIDSizeInBytes = adapter.NativeLuidSizeInBytes;
			pclSupported = slIsFeatureSupported(sl::kFeaturePCL, adapterInfo) == sl::Result::eOk;
			reflexSupported = slIsFeatureSupported(sl::kFeatureReflex, adapterInfo) == sl::Result::eOk;
			if (reflexSupported && slReflexSetOptions(sl::ReflexOptions{}) != sl::Result::eOk)
			{
				Diagnostics::Fatal(
				    g_streamlineRuntimeLogger,
				    __FILE__,
				    __LINE__,
				    "Streamline Reflex rejected its initial latency configuration.");
			}
		}
		{
			std::lock_guard lock(s_mutex);
			s_deviceBound = deviceBound;
			s_pclSupported = pclSupported;
			s_reflexSupported = reflexSupported;
		}
		return deviceBound;
	}

	static bool UpgradeInterface(ERhiInterposerInterfaceKind, void** nativeInterface, void*) noexcept
	{
		CallLease call(CallRequirement::DeviceBound);
		return call && nativeInterface != nullptr && slUpgradeInterface(nativeInterface) == sl::Result::eOk;
	}

	static bool ResolveNativeInterface(ERhiInterposerInterfaceKind, void* externalInterface, void** nativeInterface, void*) noexcept
	{
		CallLease call(CallRequirement::Initialized);
		return call && externalInterface != nullptr && nativeInterface != nullptr
		    && slGetNativeInterface(externalInterface, nativeInterface) == sl::Result::eOk;
	}

	static void SetPresentationReady(bool ready, void*) noexcept
	{
		std::lock_guard lock(s_mutex);
		s_presentationReady = !s_shuttingDown && s_initialized && s_deviceBound && ready;
	}

	static sl::PCLMarker ToPclMarker(ERhiFrameLatencyMarker marker) noexcept
	{
		switch (marker)
		{
			case ERhiFrameLatencyMarker::SimulationStart:
				return sl::PCLMarker::eSimulationStart;
			case ERhiFrameLatencyMarker::SimulationEnd:
				return sl::PCLMarker::eSimulationEnd;
			case ERhiFrameLatencyMarker::RenderSubmitStart:
				return sl::PCLMarker::eRenderSubmitStart;
			case ERhiFrameLatencyMarker::RenderSubmitEnd:
				return sl::PCLMarker::eRenderSubmitEnd;
			case ERhiFrameLatencyMarker::PresentStart:
				return sl::PCLMarker::ePresentStart;
			case ERhiFrameLatencyMarker::PresentEnd:
				return sl::PCLMarker::ePresentEnd;
		}
		Diagnostics::Fatal(g_streamlineRuntimeLogger, __FILE__, __LINE__, "Unknown external frame marker.");
	}

	static void SetFrameMarker(ERhiFrameLatencyMarker marker, std::uint64_t frameId, void*) noexcept
	{
		CallLease call(CallRequirement::RuntimeReady);
		if (!call || !s_pclSupported)
		{
			return;
		}

		const std::uint32_t frameIndex = static_cast<std::uint32_t>(frameId);
		sl::FrameToken* frameToken = nullptr;
		if (slGetNewFrameToken(frameToken, &frameIndex) != sl::Result::eOk || frameToken == nullptr)
		{
			Diagnostics::Fatal(g_streamlineRuntimeLogger, __FILE__, __LINE__, "Streamline could not resolve the logical frame token.");
		}
		if (marker == ERhiFrameLatencyMarker::SimulationStart && s_reflexSupported && slReflexSleep(*frameToken) != sl::Result::eOk)
		{
			Diagnostics::Fatal(g_streamlineRuntimeLogger, __FILE__, __LINE__, "Streamline Reflex sleep failed for the active frame.");
		}
		if (slPCLSetMarker(ToPclMarker(marker), *frameToken) != sl::Result::eOk)
		{
			Diagnostics::Fatal(g_streamlineRuntimeLogger, __FILE__, __LINE__, "Streamline PCL rejected a frame marker.");
		}
	}

	static void ShutdownFromInterposer(void*) noexcept { Shutdown(); }

	static void FillPreferences(sl::Preferences& preferences)
	{
		static constexpr sl::Feature features[] = {sl::kFeatureDLSS, sl::kFeatureDLSS_RR, sl::kFeaturePCL, sl::kFeatureReflex};
		preferences = {};
		preferences.featuresToLoad = features;
		preferences.numFeaturesToLoad = static_cast<std::uint32_t>(std::size(features));
		preferences.flags = sl::PreferenceFlags::eDisableCLStateTracking | sl::PreferenceFlags::eUseManualHooking
		    | sl::PreferenceFlags::eUseFrameBasedResourceTagging | sl::PreferenceFlags::eAllowOTA
		    | sl::PreferenceFlags::eLoadDownloadedPlugins;
		preferences.engine = sl::EngineType::eCustom;
		preferences.engineVersion = "SparkleEngine-Development";
		preferences.projectId = "535041524B4C45454E47494E45303031";
		preferences.renderAPI = sl::RenderAPI::eD3D12;
	}

	static bool HasAdapterLuid(const RhiAdapterIdentity& adapter) noexcept
	{
		return adapter.NativeLuidSizeInBytes > 0u && adapter.NativeLuidSizeInBytes <= adapter.NativeLuid.size();
	}

	static bool ValidateBackend(const RhiCapabilities& capabilities, RhiNativeDeviceQueueInterop nativeInterop) noexcept
	{
		const RhiExternalFeatureInteropCapabilities& interop = capabilities.ExternalFeatureInterop;
		const bool commonInterop = interop.ExposesNativeDevice && interop.ExposesNativeGraphicsQueue
		    && interop.ExposesNativeGraphicsCommandList && interop.ExposesNativeResources && interop.SupportsExternalProviderEvaluation;
		return capabilities.BackendApi == ERhiBackendApi::D3D12 && commonInterop && nativeInterop && HasAdapterLuid(interop.Adapter);
	}

	static std::mutex s_mutex;
	static std::condition_variable s_idle;
	static bool s_initialized;
	static bool s_deviceBound;
	static bool s_presentationReady;
	static bool s_pclSupported;
	static bool s_reflexSupported;
	static bool s_initializing;
	static bool s_shuttingDown;
	static std::uint32_t s_activeCalls;
};

std::mutex StreamlineRuntime::s_mutex;
std::condition_variable StreamlineRuntime::s_idle;
bool StreamlineRuntime::s_initialized = false;
bool StreamlineRuntime::s_deviceBound = false;
bool StreamlineRuntime::s_presentationReady = false;
bool StreamlineRuntime::s_pclSupported = false;
bool StreamlineRuntime::s_reflexSupported = false;
bool StreamlineRuntime::s_initializing = false;
bool StreamlineRuntime::s_shuttingDown = false;
std::uint32_t StreamlineRuntime::s_activeCalls = 0;
#endif

RhiInterposerHooks InitializeSharedStreamlineRuntime(ERhiBackendApi backendApi)
{
#if SPARKLE_WITH_NVIDIA_STREAMLINE
	if (!StreamlineRuntime::Initialize(backendApi))
	{
		return {};
	}
	return StreamlineRuntime::GetInterposerHooks();
#else
	(void) backendApi;
	return {};
#endif
}

void ShutdownSharedStreamlineRuntime() noexcept
{
#if SPARKLE_WITH_NVIDIA_STREAMLINE
	StreamlineRuntime::Shutdown();
#endif
}

void SetSharedStreamlineFrameMarker(ERhiFrameLatencyMarker marker, std::uint64_t frameId) noexcept
{
#if SPARKLE_WITH_NVIDIA_STREAMLINE
	StreamlineRuntime::SetApplicationFrameMarker(marker, frameId);
#else
	(void) marker;
	(void) frameId;
#endif
}

#if SPARKLE_WITH_NVIDIA_STREAMLINE
bool IsStreamlineFeatureSupported(
    sl::Feature feature,
    const RhiCapabilities& capabilities,
    RhiNativeDeviceQueueInterop nativeInterop) noexcept
{
	return StreamlineRuntime::IsFeatureSupported(feature, capabilities, nativeInterop);
}
#endif

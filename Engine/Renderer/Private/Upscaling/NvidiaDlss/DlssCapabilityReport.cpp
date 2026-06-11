#include "../../PCH.h"
#include "Upscaling/NvidiaDlss/DlssCapabilityReport.h"

#include "RHI/Public/Core/RhiBackendSelection.h"

#include <format>
#include <string>
#include <vector>

namespace
{
	constexpr const char* BoolToString(bool value) noexcept
	{
		return value ? "true" : "false";
	}

	void AppendMissing(std::vector<std::string>& missing, bool present, const char* label)
	{
		if (!present)
		{
			missing.emplace_back(label);
		}
	}

	std::string JoinMissing(const std::vector<std::string>& missing)
	{
		std::string result;
		for (std::size_t index = 0; index < missing.size(); ++index)
		{
			if (index > 0)
			{
				result += ", ";
			}
			result += missing[index];
		}
		return result;
	}
}

bool DlssCapabilityReport::CanCreateFeature() const noexcept
{
	return RhiBridgeReady && SdkRuntimeIntegrated && SdkRuntimeAvailable && FeatureQuerySucceeded && FeatureSupported;
}

DlssCapabilityReport DlssCapabilityReporter::Build(const RhiCapabilities& capabilities) noexcept
{
	const RhiExternalFeatureInteropCapabilities& interop = capabilities.ExternalFeatureInterop;
	DlssCapabilityReport report{};
	report.BackendApi = capabilities.BackendApi;
	report.BridgeKind = interop.BridgeKind;
	report.Adapter = interop.Adapter;
	report.D3D12BridgeReady = capabilities.BackendApi == ERhiBackendApi::D3D12 &&
	                          interop.BridgeKind == ERhiExternalFeatureBridgeKind::D3D12NativeDevice &&
	                          interop.ExposesNativeDevice &&
	                          interop.ExposesNativeGraphicsQueue &&
	                          interop.ExposesNativeGraphicsCommandList &&
	                          interop.ExposesNativeResources &&
	                          interop.SupportsExplicitResourceStates &&
	                          interop.SupportsExternalProviderEvaluation;
	report.VulkanBridgeReady = capabilities.BackendApi == ERhiBackendApi::Vulkan &&
	                           interop.BridgeKind == ERhiExternalFeatureBridgeKind::VulkanManualFunctionPointers &&
	                           interop.VulkanManualFunctionPointerHookingReady &&
	                           !interop.VulkanInterposerRequired &&
	                           interop.ExposesNativeResources &&
	                           interop.SupportsExplicitResourceStates &&
	                           interop.SupportsExternalProviderEvaluation;
	report.RhiBridgeReady = report.D3D12BridgeReady || report.VulkanBridgeReady;

	const StreamlineDlssRuntimeCapabilities runtimeCapabilities = QueryStreamlineDlssRuntimeCapabilities(capabilities);
	report.SdkRuntimeIntegrated = runtimeCapabilities.RuntimeIntegrated;
	report.SdkRuntimeAvailable = runtimeCapabilities.RuntimeAvailable;
	report.FeatureQuerySucceeded = runtimeCapabilities.FeatureQuerySucceeded;
	report.FeatureSupported = runtimeCapabilities.FeatureSupported;
	report.RuntimeState = report.CanCreateFeature() ? EDlssProviderRuntimeState::AvailableNotCreated : EDlssProviderRuntimeState::Unavailable;
	report.SdkVersion = runtimeCapabilities.SdkVersion;
	report.UnavailableReason = BuildUnavailableReason(capabilities, report);
	return report;
}

void DlssCapabilityReporter::ApplyRuntimeDiagnostics(DlssCapabilityReport& report, const StreamlineDlssRuntimeDiagnostics& diagnostics)
{
	report.RuntimeState = diagnostics.State;
	report.SdkVersion = diagnostics.SdkVersion;
	report.SelectedQualityMode = diagnostics.SelectedQualityMode;
	report.RenderExtent = diagnostics.RenderExtent;
	report.OutputExtent = diagnostics.OutputExtent;
	report.ResetRequested = diagnostics.ResetRequested;
	report.ResetReason = diagnostics.ResetReason;
	if (!diagnostics.FailureReason.empty())
	{
		report.UnavailableReason = diagnostics.FailureReason;
	}
}

void DlssCapabilityReporter::LogOnce(const DlssCapabilityReport& report) noexcept
{
	static bool s_logged = false;
	if (s_logged)
	{
		return;
	}

	s_logged = true;
	const std::shared_ptr<spdlog::logger> logger = Logging::GetOrCreateLogger("Renderer.DLSS");
	SPDLOG_LOGGER_INFO(
	    logger,
	    "DLSS capability summary: backend={} bridge={} adapter='{}' vendorId={:#06x} deviceId={:#06x} driver='{}' "
	    "rhiBridgeReady={} d3d12BridgeReady={} vulkanBridgeReady={} sdkRuntimeIntegrated={} sdkRuntimeAvailable={} "
	    "featureQuerySucceeded={} featureSupported={} canCreateFeature={} runtimeState={} sdkVersion='{}' selectedMode='{}' "
	    "renderExtent={}x{} outputExtent={}x{} resetRequested={} resetReason='{}' reason='{}'",
	    RhiBackendApiToString(report.BackendApi),
	    RhiExternalFeatureBridgeKindToString(report.BridgeKind),
	    report.Adapter.Name,
	    report.Adapter.VendorId,
	    report.Adapter.DeviceId,
	    report.Adapter.DriverDescription,
	    BoolToString(report.RhiBridgeReady),
	    BoolToString(report.D3D12BridgeReady),
	    BoolToString(report.VulkanBridgeReady),
	    BoolToString(report.SdkRuntimeIntegrated),
	    BoolToString(report.SdkRuntimeAvailable),
	    BoolToString(report.FeatureQuerySucceeded),
	    BoolToString(report.FeatureSupported),
	    BoolToString(report.CanCreateFeature()),
	    DlssProviderRuntimeStateToString(report.RuntimeState),
	    report.SdkVersion,
	    report.SelectedQualityMode,
	    report.RenderExtent.Width,
	    report.RenderExtent.Height,
	    report.OutputExtent.Width,
	    report.OutputExtent.Height,
	    BoolToString(report.ResetRequested),
	    report.ResetReason,
	    report.UnavailableReason);
}

std::string DlssCapabilityReporter::BuildUnavailableReason(const RhiCapabilities& capabilities, const DlssCapabilityReport& report)
{
	const RhiExternalFeatureInteropCapabilities& interop = capabilities.ExternalFeatureInterop;
	if (!report.RhiBridgeReady)
	{
		std::vector<std::string> missing;
		AppendMissing(missing, interop.ExposesNativeDevice, "native device handle");
		AppendMissing(missing, interop.ExposesNativeGraphicsQueue, "native graphics queue handle");
		AppendMissing(missing, interop.ExposesNativeGraphicsCommandList, "native command list/command buffer handle");
		AppendMissing(missing, interop.ExposesNativeResources, "native resource handles");
		AppendMissing(missing, interop.SupportsExplicitResourceStates, "explicit resource state control");
		AppendMissing(missing, interop.SupportsExternalProviderEvaluation, "external provider evaluation support");
		if (capabilities.BackendApi == ERhiBackendApi::Vulkan)
		{
			AppendMissing(missing, interop.VulkanHasInstanceHandle, "Vulkan instance handle");
			AppendMissing(missing, interop.VulkanHasPhysicalDeviceHandle, "Vulkan physical device handle");
			AppendMissing(missing, interop.VulkanHasDeviceHandle, "Vulkan device handle");
			AppendMissing(missing, interop.VulkanHasGraphicsQueueHandle, "Vulkan graphics queue handle");
			AppendMissing(missing, interop.VulkanHasGraphicsQueueFamilyIndex, "Vulkan graphics queue family index");
			AppendMissing(missing, interop.VulkanManualFunctionPointerHookingReady, "Vulkan manual function-pointer hook readiness");
			if (interop.VulkanInterposerRequired)
			{
				missing.emplace_back("Vulkan interposer required by current bridge decision");
			}
		}

		return missing.empty()
		           ? std::format("Backend {} has no accepted external-provider bridge kind.", RhiBackendApiToString(capabilities.BackendApi))
		           : std::format("RHI external-provider bridge incomplete for backend {}: missing {}.", RhiBackendApiToString(capabilities.BackendApi), JoinMissing(missing));
	}

	if (!report.SdkRuntimeIntegrated)
	{
		return "DLSS provider runtime is not integrated yet; SDK binary lookup and feature query are deferred to the provider implementation phase.";
	}

	if (!report.SdkRuntimeAvailable)
	{
		return "DLSS SDK runtime is unavailable.";
	}

	if (!report.FeatureQuerySucceeded)
	{
		return "DLSS feature support query failed.";
	}

	if (!report.FeatureSupported)
	{
		return "DLSS feature is not supported on the selected adapter.";
	}

	return "DLSS feature can be created.";
}

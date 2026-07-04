#include "PCH.h"

#include "Device/RhiCapabilityLogFormatting.h"

#include <format>

std::string FormatBackendVersionInfo(const RhiBackendVersionInfo& version)
{
	if (!version.IsKnown())
	{
		return "backendVersion(unknown)";
	}

	return std::format(
	    "backendVersion(kind={} value={}.{}.{} packed={:#x})",
	    RhiBackendVersionSemanticToString(version.Semantic),
	    version.Major,
	    version.Minor,
	    version.Patch,
	    version.PackedValue);
}

std::string FormatBackendDiagnosticsSupport(const RhiBackendDiagnosticsSupport& diagnostics)
{
	return std::format(
	    "diagnostics(validation={}, debugLayer={}, objectNames={}, gpuEvents={}, timestampQueries={}, debugMessages={}, "
	    "liveObjectReports={}, crashDiagnostics={})",
	    diagnostics.ValidationEnabled,
	    diagnostics.SupportsDebugLayer,
	    diagnostics.SupportsObjectNames,
	    diagnostics.SupportsGpuEvents,
	    diagnostics.SupportsTimestampQueries,
	    diagnostics.SupportsDebugMessages,
	    diagnostics.SupportsLiveObjectReports,
	    diagnostics.SupportsCrashDiagnostics);
}

std::string FormatBackendMemorySupport(const RhiBackendMemorySupport& memory)
{
	return std::format(
	    "memorySupport(diagnostics={}, budgetQueries={}, jsonDump={}, residencyPressure={})",
	    memory.SupportsMemoryDiagnostics,
	    memory.SupportsBudgetQueries,
	    memory.SupportsJsonDump,
	    memory.SupportsResidencyPressure);
}

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

#include "../../PCH.h"
#include "Upscaling/NvidiaDlss/StreamlineDlssRuntime.h"
#include "Upscaling/NvidiaDlss/StreamlineDlssFeatureMatrix.h"

StreamlineDlssRuntimeCapabilities QueryStreamlineDlssRuntimeCapabilities(const RhiCapabilities& capabilities) noexcept
{
#if SPARKLE_WITH_NVIDIA_STREAMLINE
	const bool d3d12BridgeReady = capabilities.BackendApi == ERhiBackendApi::D3D12 &&
	                              capabilities.ExternalFeatureInterop.ExposesNativeDevice &&
	                              capabilities.ExternalFeatureInterop.ExposesNativeGraphicsQueue &&
	                              capabilities.ExternalFeatureInterop.ExposesNativeGraphicsCommandList &&
	                              capabilities.ExternalFeatureInterop.ExposesNativeResources;
	const bool vulkanBridgeReady = capabilities.BackendApi == ERhiBackendApi::Vulkan &&
	                               capabilities.ExternalFeatureInterop.VulkanManualFunctionPointerHookingReady &&
	                               capabilities.ExternalFeatureInterop.ExposesNativeResources;
	const bool runtimeReady = d3d12BridgeReady || vulkanBridgeReady;
	std::string reason;
	if (d3d12BridgeReady)
	{
		reason = "NVIDIA Streamline SDK is integrated; D3D12 runtime support will be verified during provider initialization.";
	}
	else if (vulkanBridgeReady)
	{
		reason = "NVIDIA Streamline SDK is integrated; Vulkan runtime support will be verified during provider initialization.";
	}
	else
	{
		reason = "Native device, command-list/command-buffer, or resource interop is unavailable for DLSS.";
	}

	return StreamlineDlssRuntimeCapabilities{
	    .RuntimeIntegrated = true,
	    .RuntimeAvailable = runtimeReady,
	    .FeatureQuerySucceeded = runtimeReady,
	    .FeatureSupported = runtimeReady,
	    .FailureDomain = runtimeReady ? EUpscalerProviderFailureDomain::None : EUpscalerProviderFailureDomain::Backend,
	    .FeatureMatrix = CreateStreamlineDlssFeatureMatrix(runtimeReady, reason),
	    .Reason = reason};
#else
	return StreamlineDlssRuntimeCapabilities{
	    .RuntimeIntegrated = false,
	    .RuntimeAvailable = false,
	    .FeatureQuerySucceeded = false,
	    .FeatureSupported = false,
	    .FailureDomain = EUpscalerProviderFailureDomain::Sdk,
	    .FeatureMatrix = CreateUnavailableStreamlineDlssFeatureMatrix(kStreamlineDlssNotIntegratedReason),
	    .Reason = kStreamlineDlssNotIntegratedReason};
#endif
}

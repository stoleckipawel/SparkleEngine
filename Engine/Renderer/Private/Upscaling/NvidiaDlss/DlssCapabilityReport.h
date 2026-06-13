#pragma once

#include "Upscaling/NvidiaDlss/StreamlineDlssRuntime.h"
#include "RHI/Public/Core/RhiBackendApi.h"
#include "RHI/Public/Core/RhiCapabilities.h"

#include <string>

struct DlssCapabilityReport final
{
	ERhiBackendApi BackendApi = ERhiBackendApi::Unknown;
	ERhiExternalFeatureBridgeKind BridgeKind = ERhiExternalFeatureBridgeKind::None;
	RhiAdapterIdentity Adapter;
	bool RhiBridgeReady = false;
	bool D3D12BridgeReady = false;
	bool VulkanBridgeReady = false;
	bool SdkRuntimeIntegrated = false;
	bool SdkRuntimeAvailable = false;
	bool FeatureQuerySucceeded = false;
	bool FeatureSupported = false;
	EDlssProviderRuntimeState RuntimeState = EDlssProviderRuntimeState::NotSelected;
	EUpscalerProviderFailureDomain FailureDomain = EUpscalerProviderFailureDomain::None;
	std::string SdkVersion;
	std::string SelectedQualityMode;
	DlssFeatureMatrix FeatureMatrix;
	RenderViewportExtent RenderExtent = {};
	RenderViewportExtent OutputExtent = {};
	bool ResetRequested = false;
	std::string ResetReason;
	std::string UnavailableReason;

	bool CanCreateFeature() const noexcept;
};

class DlssCapabilityReporter final
{
  public:
	static DlssCapabilityReport Build(const RhiCapabilities& capabilities) noexcept;
	static void ApplyRuntimeDiagnostics(DlssCapabilityReport& report, const StreamlineDlssRuntimeDiagnostics& diagnostics);
	static void LogOnce(const DlssCapabilityReport& report) noexcept;

  private:
	static std::string BuildUnavailableReason(const RhiCapabilities& capabilities, const DlssCapabilityReport& report);
};

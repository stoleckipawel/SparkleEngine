#pragma once

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
	std::string UnavailableReason;

	bool CanCreateFeature() const noexcept;
};

class DlssCapabilityReporter final
{
  public:
	static DlssCapabilityReport Build(const RhiCapabilities& capabilities) noexcept;
	static void LogOnce(const DlssCapabilityReport& report) noexcept;

  private:
	static std::string BuildUnavailableReason(const RhiCapabilities& capabilities, const DlssCapabilityReport& report);
};

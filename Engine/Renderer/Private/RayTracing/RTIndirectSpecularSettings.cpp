#include "../PCH.h"
#include "RayTracing/RTIndirectSpecularSettings.h"

#include "RayTracing/RTIndirectSpecularCVars.h"
#include "RayTracing/RayTracingCapabilityReport.h"

#include <algorithm>

RTIndirectSpecularSettings BuildRTIndirectSpecularSettingsFromCVars() noexcept
{
	return RTIndirectSpecularSettings{
	    .Enabled = CVarRTIndirectSpecularEnabled.Get(),
	    .SampleMode = CVarRTIndirectSpecularSampleMode.Get(),
	    .DebugMode = CVarRTIndirectSpecularDebugMode.Get(),
	    .NormalBias = std::max(CVarRTIndirectSpecularNormalBias.Get(), 0.0f),
	    .MaxDistance = std::max(CVarRTIndirectSpecularMaxDistance.Get(), 0.001f)};
}

void LogRTIndirectSpecularSettingsOnce(
    const RTIndirectSpecularSettings& settings,
    const RayTracingCapabilityReport& capabilities) noexcept
{
	static bool s_logged = false;
	if (s_logged)
	{
		return;
	}

	s_logged = true;
	const std::shared_ptr<spdlog::logger> logger = Logging::GetOrCreateLogger("Renderer.RTIndirectSpecular");
	SPDLOG_LOGGER_INFO(
	    logger,
	    "RT indirect specular settings: enabled={} sampleMode={} debugMode={} normalBias={} maxDistance={} "
	    "requiresInlineRayQuery=true requiresDescriptorTlas=true",
	    settings.Enabled ? "true" : "false",
	    static_cast<std::uint32_t>(settings.SampleMode),
	    static_cast<std::uint32_t>(settings.DebugMode),
	    settings.NormalBias,
	    settings.MaxDistance);

	if (settings.Enabled && !capabilities.CanUseInlineRayQueryShadows())
	{
		SPDLOG_LOGGER_WARN(
		    logger,
		    "RT indirect specular is enabled but inline ray query support is unavailable: {}.",
		    capabilities.GetInlineRayQueryShadowUnavailableReason());
	}
}

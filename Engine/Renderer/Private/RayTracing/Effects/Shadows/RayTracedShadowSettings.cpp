#include "../../../PCH.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowSettings.h"

#include "RayTracing/Effects/Shadows/RayTracedShadowCVars.h"
#include "RayTracing/RayTracingCapabilityReport.h"

RayTracedShadowSettings BuildRayTracedShadowSettingsFromCVars() noexcept
{
	return RayTracedShadowSettings{
	    .QualityMode = CVarRayTracedShadowQualityMode.Get(),
	    .DenoiserMode = CVarRayTracedShadowDenoiserMode.Get(),
	    .NormalBias = CVarRayTracedShadowNormalBias.Get(),
	    .MaxDistance = CVarRayTracedShadowMaxDistance.Get(),
	    .Enabled = CVarRayTracedShadowsEnabled.Get(),
	    .DiagnosticsEnabled = CVarRayTracedShadowDiagnosticsEnabled.Get()};
}

void LogRayTracedShadowSettingsOnce(
    const RayTracedShadowSettings& settings,
    const RayTracingCapabilityReport& capabilities) noexcept
{
	static bool s_logged = false;
	if (s_logged)
	{
		return;
	}

	s_logged = true;
	const std::shared_ptr<spdlog::logger> logger = Logging::GetOrCreateLogger("Renderer.RayTracing");

	if (!capabilities.CanUseInlineRayQueryShadows())
	{
		SPDLOG_LOGGER_ERROR(
		    logger,
		    "Ray traced shadows are the engine shadow path, but the active backend does not expose the required inline ray query "
		    "capability.");
	}
}

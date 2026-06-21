#include "../PCH.h"
#include "RayTracing/IndirectSpecularSettings.h"

#include "RayTracing/IndirectSpecularCVars.h"
#include "RayTracing/RayTracingCapabilityReport.h"
#include "SceneData/MaterialCVars.h"

#include <algorithm>

IndirectSpecularSettings BuildIndirectSpecularSettingsFromCVars() noexcept
{
	return IndirectSpecularSettings{
	    .Enabled = CVarIndirectSpecularEnabled.Get(),
	    .SampleMode = CVarIndirectSpecularSampleMode.Get(),
	    .DebugMode = CVarIndirectSpecularDebugMode.Get(),
	    .MaterialMode = CVarRendererMaterialBindingMode.Get(),
	    .NormalBias = std::max(CVarIndirectSpecularNormalBias.Get(), 0.0f),
	    .MaxDistance = std::max(CVarIndirectSpecularMaxDistance.Get(), 0.001f)};
}

void LogIndirectSpecularSettingsOnce(
    const IndirectSpecularSettings& settings,
    const RayTracingCapabilityReport& capabilities) noexcept
{
	static bool s_logged = false;
	if (s_logged)
	{
		return;
	}

	s_logged = true;
	const std::shared_ptr<spdlog::logger> logger = Logging::GetOrCreateLogger("Renderer.IndirectSpecular");
	SPDLOG_LOGGER_INFO(
	    logger,
	    "Indirect specular settings: enabled={} sampleMode={} debugMode={} materialBindingMode={} normalBias={} maxDistance={} "
	    "requiresInlineRayQuery=true requiresDescriptorTlas=true",
	    settings.Enabled ? "true" : "false",
	    static_cast<std::uint32_t>(settings.SampleMode),
	    static_cast<std::uint32_t>(settings.DebugMode),
	    MaterialBindingModeToString(settings.MaterialMode),
	    settings.NormalBias,
	    settings.MaxDistance);

	if (settings.Enabled && !capabilities.CanUseInlineRayQueryShadows())
	{
		SPDLOG_LOGGER_WARN(
		    logger,
		    "Indirect specular is enabled but inline ray query support is unavailable: {}.",
		    capabilities.GetInlineRayQueryShadowUnavailableReason());
	}

	if (settings.Enabled && settings.MaterialMode == MaterialBindingMode::Everything &&
	    !capabilities.MaterialTextureTable.SupportsMaterialBindingMode(settings.MaterialMode))
	{
		SPDLOG_LOGGER_WARN(
		    logger,
		    "Indirect specular requested renderer Everything material binding, but this source path is unavailable: {}.",
		    capabilities.MaterialTextureTable.StatusReason);
	}
}

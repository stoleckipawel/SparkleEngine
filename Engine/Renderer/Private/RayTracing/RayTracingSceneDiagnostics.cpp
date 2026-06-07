#include "PCH.h"

#include "RayTracing/RayTracingSceneDiagnostics.h"

#include "RHI/Public/Core/RhiBackendSelection.h"

static const auto g_rayTracingSceneLogger = Logging::GetOrCreateLogger("Renderer.RayTracing");

void RayTracingSceneDiagnostics::LogSceneUpdate(
    const RayTracingCapabilityReport& capabilityReport,
    const RayTracingBlasCache::BuildStats& blasStats,
    const RayTracingTlasBuilder::BuildStats& tlasStats) noexcept
{
	if (!capabilityReport.SupportsRayTracing)
	{
		return;
	}

	if (m_hasLoggedSceneSummary && m_lastReferencedMeshCount == blasStats.referencedMeshCount &&
	    m_lastBuiltBlasCount == blasStats.builtBlasCount && m_lastReusedBlasCount == blasStats.reusedBlasCount &&
	    m_lastTlasInstanceCount == tlasStats.instanceCount && m_lastBuiltTlas == tlasStats.builtTlas)
	{
		return;
	}

	m_hasLoggedSceneSummary = true;
	m_lastReferencedMeshCount = blasStats.referencedMeshCount;
	m_lastBuiltBlasCount = blasStats.builtBlasCount;
	m_lastReusedBlasCount = blasStats.reusedBlasCount;
	m_lastTlasInstanceCount = tlasStats.instanceCount;
	m_lastBuiltTlas = tlasStats.builtTlas;

	SPDLOG_LOGGER_INFO(
	    g_rayTracingSceneLogger,
	    "RenderRayTracingScene: backend={} supportsRT={} inlineRayQuery={} referencedMeshes={} builtBlas={} reusedBlas={} tlasInstances={} builtTlas={}.",
	    RhiBackendApiToString(capabilityReport.BackendApi),
	    capabilityReport.SupportsRayTracing,
	    capabilityReport.SupportsInlineRayQuery,
	    blasStats.referencedMeshCount,
	    blasStats.builtBlasCount,
	    blasStats.reusedBlasCount,
	    tlasStats.instanceCount,
	    tlasStats.builtTlas);
}

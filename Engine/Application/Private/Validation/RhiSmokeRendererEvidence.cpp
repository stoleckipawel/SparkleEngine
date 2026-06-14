#include "PCH.h"

#include "Validation/RhiSmokeRendererEvidence.h"

#include "Renderer.h"
#include "Renderer/Public/Diagnostics/RendererSmokeDiagnostics.h"
#include "RHI/Public/Core/RhiBackendApi.h"
#include "RHI/Public/Core/RhiBackendSelection.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Diagnostics/RhiDiagnosticsService.h"
#include "RuntimeApplication.h"

namespace
{
	std::shared_ptr<spdlog::logger> GetSmokeLogger()
	{
		return Logging::GetOrCreateLogger("Application.SmokeValidation");
	}

	void LogUnavailableDiagnostics(const RhiDiagnosticsCapabilities& capabilities, const std::shared_ptr<spdlog::logger>& logger) noexcept
	{
		if (!capabilities.SupportsGpuEvents)
		{
			SPDLOG_LOGGER_WARN(
			    logger,
			    "RHI smoke validation: command scopes are unavailable because the active backend could not initialize GPU event markers.");
		}
		if (!capabilities.SupportsTimestampQueries)
		{
			SPDLOG_LOGGER_WARN(logger, "RHI smoke validation: timestamp queries are unavailable on the current backend/device path.");
		}
		if (!capabilities.SupportsDebugMessages)
		{
			SPDLOG_LOGGER_WARN(
			    logger,
			    "RHI smoke validation: debug messages are unavailable; inspect the active backend diagnostics log lines for the concrete "
			    "environment or runtime reason.");
		}
		if (!capabilities.SupportsLiveObjectReports)
		{
			SPDLOG_LOGGER_WARN(
			    logger,
			    "RHI smoke validation: live object reporting is unavailable; inspect the active backend diagnostics log lines for the "
			    "concrete "
			    "environment or runtime reason.");
		}
		if (!capabilities.SupportsCrashDiagnostics)
		{
			SPDLOG_LOGGER_WARN(
			    logger,
			    "RHI smoke validation: crash diagnostics are unavailable; inspect the active backend diagnostics log lines for the "
			    "concrete "
			    "environment or runtime reason.");
		}
	}

	void LogRendererSnapshot(
	    const RendererSmokeDiagnosticsSnapshot& snapshot,
	    std::string_view evidenceLabel,
	    const std::shared_ptr<spdlog::logger>& logger) noexcept
	{
		SPDLOG_LOGGER_INFO(
		    logger,
		    "{}: backend={} frameGraphUnresolvedBarrierWarnings={} upscalerProvider='{}' upscalerStatus={} upscalerReason='{}' "
		    "rayTracing={} inlineRayQuery={} topLevelProvider={} partitionedProvider={} partitionedSupported={} tlasValid={} "
		    "tlasInstances={} referencedMeshes={} builtBlas={} reusedBlas={} candidateInstances={} missingGpuMesh={} rejectedBlas={} "
		    "builtTlas={} cpuMs(scenePrepare={:.3f}, sceneBuild={:.3f}, blas={:.3f}, tlas={:.3f}, instancePrep={:.3f}) "
		    "gpuMs(blas={:.3f}, classicTlas={:.3f}, rayTracingPass={:.3f})",
		    evidenceLabel,
		    RhiBackendApiToString(snapshot.BackendApi),
		    snapshot.FrameGraphUnresolvedBarrierWarnings,
		    snapshot.UpscalerProvider,
		    snapshot.UpscalerStatus,
		    snapshot.UpscalerReason,
		    snapshot.RayTracingSupported,
		    snapshot.InlineRayQuerySupported,
		    RhiRayTracingTopLevelProviderToString(snapshot.RayTracingTopLevelProvider),
		    RhiPartitionedTlasProviderToString(snapshot.RayTracingPartitionedTlasProvider),
		    snapshot.RayTracingPartitionedTlasSupported,
		    snapshot.RayTracingTlasValid,
		    snapshot.RayTracingTlasInstanceCount,
		    snapshot.RayTracingReferencedMeshCount,
		    snapshot.RayTracingBuiltBlasCount,
		    snapshot.RayTracingReusedBlasCount,
		    snapshot.RayTracingCandidateInstanceCount,
		    snapshot.RayTracingMissingGpuMeshCount,
		    snapshot.RayTracingRejectedBlasCount,
		    snapshot.RayTracingBuiltTlas,
		    snapshot.RayTracingScenePrepareCpuMilliseconds,
		    snapshot.RayTracingSceneBuildCpuMilliseconds,
		    snapshot.RayTracingBlasCpuMilliseconds,
		    snapshot.RayTracingTlasCpuMilliseconds,
		    snapshot.RayTracingTlasInstancePreparationCpuMilliseconds,
		    snapshot.RayTracingBlasGpuMilliseconds,
		    snapshot.RayTracingClassicTlasGpuMilliseconds,
		    snapshot.RayTracingPassGpuMilliseconds);
	}
}

namespace RhiSmokeRendererEvidence
{
	void LogRhiDiagnosticsCapabilities(RuntimeApplication& app, bool& logged) noexcept
	{
		if (logged)
		{
			return;
		}

		const std::shared_ptr<spdlog::logger> logger = GetSmokeLogger();
		if (logger == nullptr)
		{
			return;
		}

		const RenderHardwareInterface& renderHardware = app.GetRenderer().GetRenderHardwareInterface();
		const RhiDiagnosticsCapabilities capabilities = renderHardware.GetDiagnosticsService().GetDiagnostics().GetCapabilities();
		SPDLOG_LOGGER_INFO(
		    logger,
		    "RHI smoke diagnostics capabilities: objectNames={} commandScopes={} timestampQueries={} debugMessages={} liveObjectReports={} "
		    "crashDiagnostics={}",
		    capabilities.SupportsObjectNames,
		    capabilities.SupportsGpuEvents,
		    capabilities.SupportsTimestampQueries,
		    capabilities.SupportsDebugMessages,
		    capabilities.SupportsLiveObjectReports,
		    capabilities.SupportsCrashDiagnostics);

		LogUnavailableDiagnostics(capabilities, logger);
		logged = true;
	}

	bool LogRendererEvidence(
	    RuntimeApplication& app,
	    bool& logged,
	    std::string_view evidenceLabel,
	    std::string_view validationLabel) noexcept
	{
		if (logged)
		{
			return true;
		}

		const std::shared_ptr<spdlog::logger> logger = GetSmokeLogger();
		if (logger == nullptr)
		{
			return true;
		}

		const RendererSmokeDiagnosticsSnapshot snapshot = app.GetRenderer().CaptureSmokeDiagnostics();
		LogRendererSnapshot(snapshot, evidenceLabel, logger);

		bool passed = true;
		if (snapshot.FrameGraphUnresolvedBarrierWarnings > 0)
		{
			passed = false;
			SPDLOG_LOGGER_ERROR(
			    logger,
			    "{}: frame graph reported {} unresolved barrier warning(s).",
			    validationLabel,
			    snapshot.FrameGraphUnresolvedBarrierWarnings);
		}

		logged = true;
		return passed;
	}
}

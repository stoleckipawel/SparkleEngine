#include "PCH.h"

#include "Validation/RhiSmokeRendererEvidence.h"

#include "Renderer.h"
#include "Renderer/Public/Diagnostics/RendererSmokeDiagnostics.h"
#include "RHI/Public/Core/RhiBackendApi.h"
#include "RHI/Public/Core/RhiBackendSelection.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Diagnostics/RhiDiagnostics.h"
#include "RuntimeApplication.h"
#include "Validation/RhiSmokeRayTracingEvidence.h"

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
		const RhiDiagnosticsCapabilities capabilities = renderHardware.GetDiagnostics().GetCapabilities();

		LogUnavailableDiagnostics(capabilities, logger);
		logged = true;
	}

	bool LogRendererEvidence(
	    RuntimeApplication& app,
	    bool& logged,
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

		bool passed = true;
		if (snapshot.FrameGraph.UnresolvedBarrierWarnings > 0)
		{
			passed = false;
			SPDLOG_LOGGER_ERROR(
			    logger,
			    "{}: frame graph reported {} unresolved barrier warning(s).",
			    validationLabel,
			    snapshot.FrameGraph.UnresolvedBarrierWarnings);
		}
		if (snapshot.FrameGraph.MissingExecutionBindings > 0)
		{
			passed = false;
			SPDLOG_LOGGER_ERROR(
			    logger,
			    "{}: frame graph reported {} missing execution binding(s).",
			    validationLabel,
			    snapshot.FrameGraph.MissingExecutionBindings);
		}
		if (!RhiSmokeRayTracingEvidence::Validate(snapshot.RayTracing, validationLabel, logger))
		{
			passed = false;
		}

		logged = true;
		return passed;
	}
}

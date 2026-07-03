#include "PCH.h"

#include "Validation/RhiSmokeRayTracingEvidence.h"

#include "Renderer/Public/Diagnostics/RendererSmokeRayTracingDiagnostics.h"

namespace RhiSmokeRayTracingEvidence
{
	bool Validate(
	    const RendererSmokeRayTracingDiagnostics& diagnostics,
	    std::string_view validationLabel,
	    const std::shared_ptr<spdlog::logger>& logger) noexcept
	{
		if (!diagnostics.PtlasPlanner.Overflow && diagnostics.PtlasPlanner.DuplicateStableIndexCount == 0)
		{
			return true;
		}

		SPDLOG_LOGGER_ERROR(
		    logger,
		    "{}: ray tracing PTLAS planner reported overflow={} duplicateStableIndices={}.",
		    validationLabel,
		    diagnostics.PtlasPlanner.Overflow,
		    diagnostics.PtlasPlanner.DuplicateStableIndexCount);
		return false;
	}
}

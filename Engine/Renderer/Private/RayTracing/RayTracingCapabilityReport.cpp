#include "../PCH.h"
#include "RayTracing/RayTracingCapabilityReport.h"

#include "RHI/Public/Core/RhiCapabilities.h"
#include "RHI/Public/Core/RhiBackendSelection.h"

namespace
{
	constexpr const char* BoolToString(bool value) noexcept
	{
		return value ? "true" : "false";
	}
}  // namespace

bool RayTracingCapabilityReport::CanUseInlineRayQueryShadows() const noexcept
{
	return SupportsRayTracing && SupportsInlineRayQuery && HasAccelerationStructureAlignment && HasScratchBufferAlignment &&
	       HasInstanceDescSize;
}

const char* RayTracingCapabilityReport::GetInlineRayQueryShadowUnavailableReason() const noexcept
{
	if (!SupportsRayTracing)
	{
		return "ray-tracing-unsupported";
	}
	if (!SupportsInlineRayQuery)
	{
		return "inline-ray-query-unsupported";
	}
	if (!HasAccelerationStructureAlignment)
	{
		return "missing-acceleration-structure-alignment";
	}
	if (!HasScratchBufferAlignment)
	{
		return "missing-scratch-buffer-alignment";
	}
	if (!HasInstanceDescSize)
	{
		return "missing-instance-desc-size";
	}
	return "available";
}

RayTracingCapabilityReport RayTracingCapabilityReporter::Build(const RhiCapabilities& capabilities) noexcept
{
	return Build(capabilities.BackendApi, capabilities.RayTracing);
}

void RayTracingCapabilityReporter::LogOnce(const RayTracingCapabilityReport& report) noexcept
{
	static bool s_logged = false;
	if (s_logged)
	{
		return;
	}

	s_logged = true;
	const std::shared_ptr<spdlog::logger> logger = Logging::GetOrCreateLogger("Renderer.RayTracing");
	SPDLOG_LOGGER_INFO(
	    logger,
	    "Ray tracing capability summary: backend={} rayTracing={} inlineRayQuery={} asAlignment={} scratchAlignment={} "
	    "instanceDescSize={} maxRecursionDepth={} maxPayloadBytes={} maxAttributeBytes={} inlineShadowReady={} inlineShadowReason={}",
	    RhiBackendApiToString(report.BackendApi),
	    BoolToString(report.SupportsRayTracing),
	    BoolToString(report.SupportsInlineRayQuery),
	    report.AccelerationStructureByteAlignment,
	    report.ScratchBufferByteAlignment,
	    report.InstanceDescSizeInBytes,
	    report.MaxTraceRecursionDepth,
	    report.MaxRayPayloadSizeInBytes,
	    report.MaxRayAttributeSizeInBytes,
	    BoolToString(report.CanUseInlineRayQueryShadows()),
	    report.GetInlineRayQueryShadowUnavailableReason());
}

RayTracingCapabilityReport RayTracingCapabilityReporter::Build(
    ERhiBackendApi backendApi,
    const RhiRayTracingCapabilities& rayTracing) noexcept
{
	return RayTracingCapabilityReport{
	    .BackendApi = backendApi,
	    .SupportsRayTracing = rayTracing.SupportsRayTracing,
	    .SupportsInlineRayQuery = rayTracing.SupportsInlineRayQuery,
	    .HasAccelerationStructureAlignment = rayTracing.AccelerationStructureByteAlignment != 0,
	    .HasScratchBufferAlignment = rayTracing.ScratchBufferByteAlignment != 0,
	    .HasInstanceDescSize = rayTracing.InstanceDescSizeInBytes != 0,
	    .MaxTraceRecursionDepth = rayTracing.MaxTraceRecursionDepth,
	    .MaxRayPayloadSizeInBytes = rayTracing.MaxRayPayloadSizeInBytes,
	    .MaxRayAttributeSizeInBytes = rayTracing.MaxRayAttributeSizeInBytes,
	    .AccelerationStructureByteAlignment = rayTracing.AccelerationStructureByteAlignment,
	    .ScratchBufferByteAlignment = rayTracing.ScratchBufferByteAlignment,
	    .InstanceDescSizeInBytes = rayTracing.InstanceDescSizeInBytes};
}

#pragma once

#include "Renderer/Public/Settings/EngineRenderingRayTracingTypes.h"

#include <cstdint>

struct RayTracingCapabilityReport;

enum class RayTracingExecutionFrontend : std::uint8_t
{
	None,
	Inline,
	Pipeline,
};

enum class RayTracingExecutionReason : std::uint8_t
{
	RasterizedAlgorithm,
	StrictInline,
	StrictPipeline,
	AutomaticPipelinePreferred,
	AutomaticInlineBecausePipelineUnavailable,
	AutomaticPipelineBecauseInlineUnavailable,
	AutomaticInlineBecauseMaskedGeometry,
	InlineUnavailable,
	PipelineUnavailable,
	MaskedGeometryRequiresAnyHit,
	NoFrontendAvailable,
	InvalidAlgorithm,
	InvalidExecutionMode,
};

struct RayTracingGBufferExecutionPlan final
{
	RayTracingExecutionFrontend Active = RayTracingExecutionFrontend::None;
	RayTracingExecutionReason Reason = RayTracingExecutionReason::RasterizedAlgorithm;

	bool operator==(const RayTracingGBufferExecutionPlan&) const noexcept = default;
};

RayTracingGBufferExecutionPlan ResolveRayTracingGBufferExecutionPlan(
	bool hasMaskedGeometry,
	const RayTracingCapabilityReport& capabilities) noexcept;

const char* GetRayTracingExecutionReasonLabel(RayTracingExecutionReason reason) noexcept;

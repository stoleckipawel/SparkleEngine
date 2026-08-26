#pragma once

#include "RayTracing/Effects/RayTracingExecutionFrontend.h"
#include "Renderer/Public/Settings/EngineRenderingRayTracingTypes.h"

#include <cstdint>

struct RayTracingCapabilityReport;

enum class RayTracingExecutionReason : std::uint8_t
{
	RasterizedAlgorithm,
	StrictInline,
	StrictPipeline,
	AutomaticPipelinePreferred,
	AutomaticInlineBecausePipelineUnavailable,
	AutomaticPipelineBecauseInlineUnavailable,
	InlineUnavailable,
	PipelineUnavailable,
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

RayTracingGBufferExecutionPlan ResolveRayTracingGBufferExecutionPlan(const RayTracingCapabilityReport& capabilities) noexcept;

const char* GetRayTracingExecutionReasonLabel(RayTracingExecutionReason reason) noexcept;

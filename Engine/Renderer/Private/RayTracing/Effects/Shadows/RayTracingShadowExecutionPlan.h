#pragma once

#include "RayTracing/Effects/RayTracingExecutionFrontend.h"

#include <cstdint>

struct RayTracingCapabilityReport;

enum class RayTracingShadowExecutionReason : std::uint8_t
{
	StrictInline,
	StrictPipeline,
	AutomaticPipelinePreferred,
	AutomaticInlineBecausePipelineUnavailable,
	AutomaticPipelineBecauseInlineUnavailable,
	InlineUnavailable,
	PipelineUnavailable,
	NoFrontendAvailable,
	InvalidExecutionMode,
};

struct RayTracingShadowExecutionPlan final
{
	RayTracingExecutionFrontend Active = RayTracingExecutionFrontend::None;
	RayTracingShadowExecutionReason Reason = RayTracingShadowExecutionReason::NoFrontendAvailable;

	bool operator==(const RayTracingShadowExecutionPlan&) const noexcept = default;
};

RayTracingShadowExecutionPlan ResolveRayTracingShadowExecutionPlan(const RayTracingCapabilityReport& capabilities) noexcept;
const char* GetRayTracingShadowExecutionReasonLabel(RayTracingShadowExecutionReason reason) noexcept;

#include "PCH.h"

#include "RayTracing/Effects/Shadows/RayTracingShadowExecutionPlan.h"

#include "Debug/RendererCVars.h"
#include "RayTracing/RayTracingCapabilityReport.h"

RayTracingShadowExecutionPlan ResolveRayTracingShadowExecutionPlan(const RayTracingCapabilityReport& capabilities) noexcept
{
	const bool inlineReady =
	    capabilities.SupportsAccelerationStructure && capabilities.SupportsInlineRayQuery && capabilities.SupportsDescriptorIndexing;
	const bool pipelineReady =
	    capabilities.SupportsAccelerationStructure && capabilities.SupportsRayTracingPipeline && capabilities.SupportsDescriptorIndexing;
	switch (CVarShadowRayTracingExecutionMode.Get())
	{
		case RayTracingExecutionMode::Inline:
			return RayTracingShadowExecutionPlan{
			    .Active = inlineReady ? RayTracingExecutionFrontend::Inline : RayTracingExecutionFrontend::None,
			    .Reason = inlineReady ? RayTracingShadowExecutionReason::StrictInline
			                          : RayTracingShadowExecutionReason::InlineUnavailable};
		case RayTracingExecutionMode::Pipeline:
			return RayTracingShadowExecutionPlan{
			    .Active = pipelineReady ? RayTracingExecutionFrontend::Pipeline : RayTracingExecutionFrontend::None,
			    .Reason = pipelineReady ? RayTracingShadowExecutionReason::StrictPipeline
			                            : RayTracingShadowExecutionReason::PipelineUnavailable};
		case RayTracingExecutionMode::Automatic:
			if (pipelineReady)
			{
				return RayTracingShadowExecutionPlan{
				    .Active = RayTracingExecutionFrontend::Pipeline,
				    .Reason = inlineReady ? RayTracingShadowExecutionReason::AutomaticPipelinePreferred
				                          : RayTracingShadowExecutionReason::AutomaticPipelineBecauseInlineUnavailable};
			}
			if (inlineReady)
			{
				return RayTracingShadowExecutionPlan{
				    .Active = RayTracingExecutionFrontend::Inline,
				    .Reason = RayTracingShadowExecutionReason::AutomaticInlineBecausePipelineUnavailable};
			}
			return RayTracingShadowExecutionPlan{.Reason = RayTracingShadowExecutionReason::NoFrontendAvailable};
		default:
			return RayTracingShadowExecutionPlan{.Reason = RayTracingShadowExecutionReason::InvalidExecutionMode};
	}
}

const char* GetRayTracingShadowExecutionReasonLabel(RayTracingShadowExecutionReason reason) noexcept
{
	switch (reason)
	{
		case RayTracingShadowExecutionReason::StrictInline:
			return "StrictInline";
		case RayTracingShadowExecutionReason::StrictPipeline:
			return "StrictPipeline";
		case RayTracingShadowExecutionReason::AutomaticPipelinePreferred:
			return "AutomaticPipelinePreferred";
		case RayTracingShadowExecutionReason::AutomaticInlineBecausePipelineUnavailable:
			return "AutomaticInlinePipelineUnavailable";
		case RayTracingShadowExecutionReason::AutomaticPipelineBecauseInlineUnavailable:
			return "AutomaticPipelineInlineUnavailable";
		case RayTracingShadowExecutionReason::InlineUnavailable:
			return "InlineUnavailable";
		case RayTracingShadowExecutionReason::PipelineUnavailable:
			return "PipelineUnavailable";
		case RayTracingShadowExecutionReason::NoFrontendAvailable:
			return "NoFrontendAvailable";
		case RayTracingShadowExecutionReason::InvalidExecutionMode:
		default:
			return "InvalidExecutionMode";
	}
}

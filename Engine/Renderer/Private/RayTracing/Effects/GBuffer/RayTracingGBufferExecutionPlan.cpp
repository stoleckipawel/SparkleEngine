#include "PCH.h"

#include "RayTracing/Effects/GBuffer/RayTracingGBufferExecutionPlan.h"

#include "Debug/RendererCVars.h"
#include "RayTracing/RayTracingCapabilityReport.h"

RayTracingGBufferExecutionPlan ResolveRayTracingGBufferExecutionPlan(const RayTracingCapabilityReport& capabilities) noexcept
{
	const GBufferAlgorithm algorithm = CVarGBufferAlgorithm.Get();
	if (algorithm == GBufferAlgorithm::Rasterized)
	{
		return RayTracingGBufferExecutionPlan{
		    .Reason = RayTracingExecutionReason::RasterizedAlgorithm};
	}
	if (algorithm != GBufferAlgorithm::RayTracing)
	{
		return RayTracingGBufferExecutionPlan{
		    .Reason = RayTracingExecutionReason::InvalidAlgorithm};
	}

	const bool inlineReady =
	    capabilities.SupportsAccelerationStructure && capabilities.SupportsInlineRayQuery && capabilities.SupportsDescriptorIndexing;
	const bool pipelineReady =
	    capabilities.SupportsAccelerationStructure && capabilities.SupportsRayTracingPipeline && capabilities.SupportsDescriptorIndexing;
	switch (CVarGBufferRayTracingExecutionMode.Get())
	{
		case RayTracingExecutionMode::Inline:
			return RayTracingGBufferExecutionPlan{
			    .Active = inlineReady ? RayTracingExecutionFrontend::Inline : RayTracingExecutionFrontend::None,
			    .Reason = inlineReady ? RayTracingExecutionReason::StrictInline : RayTracingExecutionReason::InlineUnavailable};
		case RayTracingExecutionMode::Pipeline:
			return RayTracingGBufferExecutionPlan{
			    .Active = pipelineReady ? RayTracingExecutionFrontend::Pipeline : RayTracingExecutionFrontend::None,
			    .Reason = pipelineReady ? RayTracingExecutionReason::StrictPipeline : RayTracingExecutionReason::PipelineUnavailable};
		case RayTracingExecutionMode::Automatic:
			if (pipelineReady)
			{
				return RayTracingGBufferExecutionPlan{
				    .Active = RayTracingExecutionFrontend::Pipeline,
				    .Reason = inlineReady ? RayTracingExecutionReason::AutomaticPipelinePreferred
				                          : RayTracingExecutionReason::AutomaticPipelineBecauseInlineUnavailable};
			}
			if (inlineReady)
			{
				return RayTracingGBufferExecutionPlan{
				    .Active = RayTracingExecutionFrontend::Inline,
				    .Reason = RayTracingExecutionReason::AutomaticInlineBecausePipelineUnavailable};
			}
			return RayTracingGBufferExecutionPlan{
			    .Reason = RayTracingExecutionReason::NoFrontendAvailable};
		default:
			return RayTracingGBufferExecutionPlan{
			    .Reason = RayTracingExecutionReason::InvalidExecutionMode};
	}
}

const char* GetRayTracingExecutionReasonLabel(RayTracingExecutionReason reason) noexcept
{
	switch (reason)
	{
		case RayTracingExecutionReason::RasterizedAlgorithm:
			return "RasterizedAlgorithm";
		case RayTracingExecutionReason::StrictInline:
			return "StrictInline";
		case RayTracingExecutionReason::StrictPipeline:
			return "StrictPipeline";
		case RayTracingExecutionReason::AutomaticPipelinePreferred:
			return "AutomaticPipelinePreferred";
		case RayTracingExecutionReason::AutomaticInlineBecausePipelineUnavailable:
			return "AutomaticInlinePipelineUnavailable";
		case RayTracingExecutionReason::AutomaticPipelineBecauseInlineUnavailable:
			return "AutomaticPipelineInlineUnavailable";
		case RayTracingExecutionReason::InlineUnavailable:
			return "InlineUnavailable";
		case RayTracingExecutionReason::PipelineUnavailable:
			return "PipelineUnavailable";
		case RayTracingExecutionReason::NoFrontendAvailable:
			return "NoFrontendAvailable";
		case RayTracingExecutionReason::InvalidAlgorithm:
			return "InvalidAlgorithm";
		case RayTracingExecutionReason::InvalidExecutionMode:
		default:
			return "InvalidExecutionMode";
	}
}

#include "../../PCH.h"
#include "Passes/RayTracing/ReferenceLightingAccumulationPass.h"

#include "Frame/Core/FrameContext.h"
#include "View/RenderView.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeContext.h"
#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "RayTracing/Effects/PathTracedLighting/PathTracedLightingSettings.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

void ReferenceLightingAccumulationPassParameters::Describe(
    ShaderParameterStructBuilder<ReferenceLightingAccumulationPassParameters>& builder)
{
	builder.ReadTexture(
	    "ReferenceLightingSample",
	    &ReferenceLightingAccumulationPassParameters::ReferenceLightingSample,
	    ShaderStageVisibility::Compute);
	builder.RWTexture("SceneColorTexture", &ReferenceLightingAccumulationPassParameters::SceneColorTexture, ShaderStageVisibility::Compute);
	builder.ReadTexture(
	    "PreviousReferenceLighting",
	    &ReferenceLightingAccumulationPassParameters::PreviousReferenceLighting,
	    ShaderStageVisibility::Compute);
	builder.RWTexture(
	    "CurrentReferenceLighting",
	    &ReferenceLightingAccumulationPassParameters::CurrentReferenceLighting,
	    ShaderStageVisibility::Compute);
	builder.ReadTexture(
	    "ReferenceSampleValidity",
	    &ReferenceLightingAccumulationPassParameters::ReferenceSampleValidity,
	    ShaderStageVisibility::Compute);
	builder.ReadTexture(
	    "GBufferMotionVector",
	    &ReferenceLightingAccumulationPassParameters::GBufferMotionVector,
	    ShaderStageVisibility::Compute);
	builder.Uniform(
	    "ReferenceLightingAccumulationConstants",
	    &ReferenceLightingAccumulationPassParameters::ReferenceLightingAccumulationConstants,
	    ShaderStageVisibility::Compute);
}

ReferenceLightingAccumulationPass::ReferenceLightingAccumulationPass(const ComputePassPipelineRuntime& runtime) noexcept :
    m_runtime(runtime)
{
}

const ReferenceLightingAccumulationPass::ParameterMetadata& ReferenceLightingAccumulationPass::GetParameterMetadata() noexcept
{
	return ComputePassOperations::BuildParameterMetadata<ReferenceLightingAccumulationPass>();
}

const RenderPassDefinition& ReferenceLightingAccumulationPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassOperations::BuildDefinition(
	    PassName,
	    RendererShaderPackages::ReferenceLightingAccumulation,
	    L"ReferenceLightingAccumulation_BindingLayout",
	    L"ReferenceLightingAccumulation_Pipeline");
	return definition;
}

void ReferenceLightingAccumulationPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	parameters->ReferenceLightingAccumulationConstants = ReferenceLightingAccumulationUniformData{
	    .SamplesFrame = BuildPathTracedLightingSettings().SamplesPerPixel,
	    .HistoryValid = context.Runtime.History.ReferenceLighting ? 1u : 0u};
	ComputePassOperations::DispatchSized<ReferenceLightingAccumulationPass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.view.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.view.viewport.Height));
}

#include "../../PCH.h"
#include "Passes/RayTracing/ReferenceLightingAccumulationPass.h"

#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Passes/Core/ComputePassUtilities.h"
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
	return ComputePassUtilities::BuildParameterMetadata<ReferenceLightingAccumulationPass>();
}

const RenderPassDefinition& ReferenceLightingAccumulationPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    RendererShaderPackages::ReferenceLightingAccumulation,
	    L"ReferenceLightingAccumulation_BindingLayout",
	    L"ReferenceLightingAccumulation_PipelineState");
	return definition;
}

void ReferenceLightingAccumulationPass::DeclareResources(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle referenceLightingSample,
    FrameGraphTextureHandle sceneColor,
    FrameGraphTextureHandle previousReferenceLighting,
    FrameGraphTextureHandle currentReferenceLighting,
    FrameGraphTextureHandle referenceSampleValidity,
    FrameGraphTextureHandle gBufferMotionVector,
    ParameterInstance& parameters)
{
	parameters->ReferenceLightingSample = builder.CreateSRV(referenceLightingSample);
	parameters->SceneColorTexture = builder.CreateUAV(sceneColor);
	parameters->PreviousReferenceLighting = builder.CreateSRV(previousReferenceLighting);
	parameters->CurrentReferenceLighting = builder.CreateUAV(currentReferenceLighting);
	parameters->ReferenceSampleValidity = builder.CreateSRV(referenceSampleValidity);
	parameters->GBufferMotionVector = builder.CreateSRV(gBufferMotionVector);
}

void ReferenceLightingAccumulationPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	parameters->ReferenceLightingAccumulationConstants = ReferenceLightingAccumulationUniformData{
	    .SamplesPerFrame = BuildPathTracedLightingSettings().SamplesPerPixel,
	    .HistoryValid = context.RuntimeServices.History.ReferenceLighting ? 1u : 0u};
	ComputePassUtilities::DispatchSized<ReferenceLightingAccumulationPass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
}

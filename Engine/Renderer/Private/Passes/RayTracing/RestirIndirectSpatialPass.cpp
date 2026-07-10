#include "../../PCH.h"
#include "Passes/RayTracing/RestirIndirectSpatialPass.h"

#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/Core/ComputePassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "RayTracing/RayTracingShaderFeatureFlags.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

void RestirIndirectSpatialPassParameters::Describe(ShaderParameterStructBuilder<RestirIndirectSpatialPassParameters>& builder)
{
	builder.ReadTexture(
	    "TemporalReservoirSampleTexture",
	    &RestirIndirectSpatialPassParameters::TemporalReservoirSampleTexture,
	    ShaderStageVisibility::Compute);
	builder.ReadTexture(
	    "TemporalReservoirWeightTexture",
	    &RestirIndirectSpatialPassParameters::TemporalReservoirWeightTexture,
	    ShaderStageVisibility::Compute);
	builder.RWTexture(
	    "CurrentReservoirSampleTexture",
	    &RestirIndirectSpatialPassParameters::CurrentReservoirSampleTexture,
	    ShaderStageVisibility::Compute);
	builder.RWTexture(
	    "CurrentReservoirWeightTexture",
	    &RestirIndirectSpatialPassParameters::CurrentReservoirWeightTexture,
	    ShaderStageVisibility::Compute);
	builder.RWTexture(
	    "CurrentReservoirSurfaceTexture",
	    &RestirIndirectSpatialPassParameters::CurrentReservoirSurfaceTexture,
	    ShaderStageVisibility::Compute);
	RestirIndirectPassCommon::DescribeSceneParameters(builder);
}

const RestirIndirectSpatialPass::ParameterMetadata& RestirIndirectSpatialPass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<RestirIndirectSpatialPass>();
}

const RenderPassDefinition& RestirIndirectSpatialPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    RendererShaderPackages::RestirIndirectSpatial,
	    L"RestirIndirectSpatial_BindingLayout",
	    L"RestirIndirectSpatial_PipelineState",
	    RayTracingShaderFeatureFlags::DescriptorRayQuery);
	return definition;
}

void RestirIndirectSpatialPass::DeclareResources(
    FrameGraphBuilder& builder,
    const SceneRenderTargets& scene,
    const GBufferRenderTargets& gbuffer,
    FrameGraphTextureHandle temporalSample,
    FrameGraphTextureHandle temporalWeight,
    FrameGraphTextureHandle currentSample,
    FrameGraphTextureHandle currentWeight,
    FrameGraphTextureHandle currentSurface,
    FrameGraphAccelerationStructureHandle sceneTlas,
    ParameterInstance& parameters)
{
	parameters->TemporalReservoirSampleTexture = builder.CreateSRV(temporalSample);
	parameters->TemporalReservoirWeightTexture = builder.CreateSRV(temporalWeight);
	parameters->CurrentReservoirSampleTexture = builder.CreateUAV(currentSample);
	parameters->CurrentReservoirWeightTexture = builder.CreateUAV(currentWeight);
	parameters->CurrentReservoirSurfaceTexture = builder.CreateUAV(currentSurface);
	RestirIndirectPassCommon::BindSceneResources(builder, scene, gbuffer, sceneTlas, parameters);
}

void RestirIndirectSpatialPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	if (!PrepareExecution(context, parameters))
	{
		return;
	}
	ComputePassUtilities::DispatchSized<RestirIndirectSpatialPass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
}

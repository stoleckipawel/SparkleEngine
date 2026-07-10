#include "../../PCH.h"
#include "Passes/RayTracing/RestirIndirectTemporalPass.h"

#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Passes/Core/ComputePassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "RayTracing/RayTracingShaderFeatureFlags.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

void RestirIndirectTemporalPassParameters::Describe(ShaderParameterStructBuilder<RestirIndirectTemporalPassParameters>& builder)
{
	builder.RWTexture(
	    "TemporalReservoirSampleTexture",
	    &RestirIndirectTemporalPassParameters::TemporalReservoirSampleTexture,
	    ShaderStageVisibility::Compute);
	builder.RWTexture(
	    "TemporalReservoirWeightTexture",
	    &RestirIndirectTemporalPassParameters::TemporalReservoirWeightTexture,
	    ShaderStageVisibility::Compute);
	builder.ReadTexture(
	    "PreviousReservoirSampleTexture",
	    &RestirIndirectTemporalPassParameters::PreviousReservoirSampleTexture,
	    ShaderStageVisibility::Compute);
	builder.ReadTexture(
	    "PreviousReservoirWeightTexture",
	    &RestirIndirectTemporalPassParameters::PreviousReservoirWeightTexture,
	    ShaderStageVisibility::Compute);
	builder.ReadTexture(
	    "PreviousReservoirSurfaceTexture",
	    &RestirIndirectTemporalPassParameters::PreviousReservoirSurfaceTexture,
	    ShaderStageVisibility::Compute);
	builder.ReadTexture("GBufferMotionVector", &RestirIndirectTemporalPassParameters::GBufferMotionVector, ShaderStageVisibility::Compute);
	builder.Uniform("PerTemporal", &RestirIndirectTemporalPassParameters::PerTemporal, ShaderStageVisibility::Compute);
	RestirIndirectPassCommon::DescribeSceneParameters(builder);
}

const RestirIndirectTemporalPass::ParameterMetadata& RestirIndirectTemporalPass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<RestirIndirectTemporalPass>();
}

const RenderPassDefinition& RestirIndirectTemporalPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    RendererShaderPackages::RestirIndirectTemporal,
	    L"RestirIndirectTemporal_BindingLayout",
	    L"RestirIndirectTemporal_PipelineState",
	    RayTracingShaderFeatureFlags::DescriptorRayQuery);
	return definition;
}

void RestirIndirectTemporalPass::DeclareResources(
    FrameGraphBuilder& builder,
    const SceneRenderTargets& scene,
    const GBufferRenderTargets& gbuffer,
    FrameGraphTextureHandle temporalSample,
    FrameGraphTextureHandle temporalWeight,
    FrameGraphTextureHandle previousSample,
    FrameGraphTextureHandle previousWeight,
    FrameGraphTextureHandle previousSurface,
    FrameGraphAccelerationStructureHandle sceneTlas,
    ParameterInstance& parameters)
{
	parameters->TemporalReservoirSampleTexture = builder.CreateUAV(temporalSample);
	parameters->TemporalReservoirWeightTexture = builder.CreateUAV(temporalWeight);
	parameters->PreviousReservoirSampleTexture = builder.CreateSRV(previousSample);
	parameters->PreviousReservoirWeightTexture = builder.CreateSRV(previousWeight);
	parameters->PreviousReservoirSurfaceTexture = builder.CreateSRV(previousSurface);
	parameters->GBufferMotionVector = builder.CreateSRV(gbuffer.MotionVector);
	RestirIndirectPassCommon::BindSceneResources(builder, scene, gbuffer, sceneTlas, parameters);
}

void RestirIndirectTemporalPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	if (!PrepareExecution(context, parameters))
	{
		return;
	}

	PerTemporalConstantBufferData temporalData = context.Frame.mainView.perTemporalData;
	if (!context.RuntimeServices.RestirIndirectReservoirHistoryValid)
	{
		temporalData.HistoryValid = 0u;
	}
	parameters->PerTemporal = temporalData;
	ComputePassUtilities::DispatchSized<RestirIndirectTemporalPass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
}

#include "../../PCH.h"
#include "Passes/RayTracing/RestirIndirectResolvePass.h"

#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/Bindings/IndirectLightingOutputPassBinding.h"
#include "Passes/Bindings/RayReconstructionGuidePassBinding.h"
#include "Passes/Core/ComputePassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "RayTracing/RayTracingShaderFeatureFlags.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

void RestirIndirectResolvePassParameters::Describe(ShaderParameterStructBuilder<RestirIndirectResolvePassParameters>& builder)
{
	builder.ReadTexture(
	    "CurrentReservoirSampleTexture",
	    &RestirIndirectResolvePassParameters::CurrentReservoirSampleTexture,
	    ShaderStageVisibility::Compute);
	builder.ReadTexture(
	    "CurrentReservoirWeightTexture",
	    &RestirIndirectResolvePassParameters::CurrentReservoirWeightTexture,
	    ShaderStageVisibility::Compute);
	IndirectLightingOutputPassBinding::Describe(builder);
	RayReconstructionGuidePassBinding::Describe(builder);
	RestirIndirectPassCommon::DescribeSceneParameters(builder);
}

const RestirIndirectResolvePass::ParameterMetadata& RestirIndirectResolvePass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<RestirIndirectResolvePass>();
}

const RenderPassDefinition& RestirIndirectResolvePass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    RendererShaderPackages::RestirIndirectResolve,
	    L"RestirIndirectResolve_BindingLayout",
	    L"RestirIndirectResolve_PipelineState",
	    RayTracingShaderFeatureFlags::DescriptorRayQuery);
	return definition;
}

void RestirIndirectResolvePass::DeclareResources(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const SceneRenderTargets& scene,
    const GBufferRenderTargets& gbuffer,
    FrameGraphTextureHandle currentSample,
    FrameGraphTextureHandle currentWeight,
    FrameGraphAccelerationStructureHandle sceneTlas,
    ParameterInstance& parameters)
{
	parameters->CurrentReservoirSampleTexture = builder.CreateSRV(currentSample);
	parameters->CurrentReservoirWeightTexture = builder.CreateSRV(currentWeight);
	IndirectLightingOutputPassBinding::Bind(builder, lighting, parameters);
	RayReconstructionGuidePassBinding::Bind(builder, lighting, parameters);
	RestirIndirectPassCommon::BindSceneResources(builder, scene, gbuffer, sceneTlas, parameters);
}

void RestirIndirectResolvePass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	if (!PrepareExecution(context, parameters))
	{
		return;
	}
	ComputePassUtilities::DispatchSized<RestirIndirectResolvePass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
}

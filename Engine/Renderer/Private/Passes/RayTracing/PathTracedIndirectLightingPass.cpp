#include "../../PCH.h"
#include "Passes/RayTracing/PathTracedIndirectLightingPass.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/Bindings/IndirectLightingOutputPassBinding.h"
#include "Passes/Core/ComputePassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "RayTracing/RayTracingShaderFeatureFlags.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "RayTracing/Effects/PathTracedLighting/PathTracedLightingSettings.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

void PathTracedIndirectLightingPassParameters::Describe(ShaderParameterStructBuilder<PathTracedIndirectLightingPassParameters>& builder)
{
	IndirectLightingOutputPassBinding::Describe(builder);
	RayTracedSurfaceLightingPassBinding::Describe(builder);
	builder.Uniform(
	    "PathTracedLightingConstants",
	    &PathTracedIndirectLightingPassParameters::PathTracedLightingConstants,
	    ShaderStageVisibility::Compute);
}

PathTracedIndirectLightingPass::PathTracedIndirectLightingPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const PathTracedIndirectLightingPass::ParameterMetadata& PathTracedIndirectLightingPass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<PathTracedIndirectLightingPass>();
}

const RenderPassDefinition& PathTracedIndirectLightingPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    RendererShaderPackages::PathTracedIndirectLighting,
	    L"PathTracedIndirectLighting_BindingLayout",
	    L"PathTracedIndirectLighting_PipelineState",
	    RayTracingShaderFeatureFlags::DescriptorRayQuery);
	return definition;
}

void PathTracedIndirectLightingPass::DeclareResources(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer,
    FrameGraphAccelerationStructureHandle sceneTlas,
    ParameterInstance& parameters)
{
	IndirectLightingOutputPassBinding::Bind(builder, lighting, parameters);
	RayTracedSurfaceLightingPassBinding::DeclareResources(builder, sceneTargets, gbuffer, sceneTlas, parameters);
}

void PathTracedIndirectLightingPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	if (!m_sceneBinding.Prepare(context, parameters))
	{
		return;
	}
	const PathTracedLightingSettings settings = BuildPathTracedLightingSettings();
	parameters->PathTracedLightingConstants = PathTracedLightingUniformData{
	    .SamplesPerPixel = settings.SamplesPerPixel,
	    .BounceCount = settings.BounceCount,
	    .NormalBias = settings.NormalBias,
	    .MaxDistance = settings.MaxDistance};
	ComputePassUtilities::DispatchSized<PathTracedIndirectLightingPass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
}

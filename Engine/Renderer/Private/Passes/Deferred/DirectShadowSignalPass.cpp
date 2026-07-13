#include "../../PCH.h"
#include "Passes/Deferred/DirectShadowSignalPass.h"

#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/Core/ComputePassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "RayTracing/RayTracingShaderFeatureFlags.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

DirectShadowSignalPass::DirectShadowSignalPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const DirectShadowSignalPass::ParameterMetadata& DirectShadowSignalPass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<DirectShadowSignalPass>();
}

const RenderPassDefinition& DirectShadowSignalPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    RendererShaderPackages::DirectShadowSignal,
	    L"DirectShadowSignal_BindingLayout",
	    L"DirectShadowSignal_PipelineState",
	    RayTracingShaderFeatureFlags::DescriptorRayQuery);
	return definition;
}

void DirectShadowSignalPass::DeclareResources(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle sceneDepth,
    const GBufferRenderTargets& gbuffer,
    FrameGraphAccelerationStructureHandle sceneTlas,
    const DirectShadowSignalResources& shadowSignals,
    FrameGraphBufferHandle directionalLights,
    FrameGraphBufferHandle pointLights,
    FrameGraphBufferHandle spotLights,
    FrameGraphBufferHandle rectLights,
    FrameGraphBufferHandle hitVertices,
    FrameGraphBufferHandle hitIndices,
    FrameGraphBufferHandle hitInstances,
    FrameGraphBufferHandle hitMaterials,
    ParameterInstance& parameters)
{
	DirectShadowSignalPassCommon::DeclareRayQueryResources(
	    builder,
	    sceneDepth,
	    gbuffer,
	    shadowSignals,
	    directionalLights,
	    pointLights,
	    spotLights,
	    rectLights,
	    hitVertices,
	    hitIndices,
	    hitInstances,
	    hitMaterials,
	    *parameters);
	parameters->SceneTlas = builder.Read(sceneTlas);
}

void DirectShadowSignalPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	DirectShadowSignalPassCommon::SetRayQueryParameters(
	    *parameters,
	    context.Frame,
	    context.Frame.mainView,
	    context.RuntimeServices,
	    context.Frame.rayTracingScene.HasTraceableInstances());
	ComputePassUtilities::DispatchSized<DirectShadowSignalPass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
}

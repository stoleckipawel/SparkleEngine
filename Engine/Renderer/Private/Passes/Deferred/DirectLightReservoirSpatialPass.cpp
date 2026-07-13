#include "../../PCH.h"
#include "Passes/Deferred/DirectLightReservoirSpatialPass.h"

#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
#include "Frame/Lighting/ShadowVisibility.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Passes/Core/ComputePassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

DirectLightReservoirSpatialPass::DirectLightReservoirSpatialPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const DirectLightReservoirSpatialPass::ParameterMetadata& DirectLightReservoirSpatialPass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<DirectLightReservoirSpatialPass>();
}

const RenderPassDefinition& DirectLightReservoirSpatialPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    RendererShaderPackages::DirectLightReservoirSpatial,
	    L"DirectLightReservoirSpatial_BindingLayout",
	    L"DirectLightReservoirSpatial_PipelineState");
	return definition;
}

void DirectLightReservoirSpatialPass::DeclareResources(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle sceneDepth,
    const GBufferRenderTargets& gbuffer,
    const DirectShadowSignalResources& shadowSignals,
    FrameGraphBufferHandle directionalLights,
    FrameGraphBufferHandle pointLights,
    FrameGraphBufferHandle spotLights,
    FrameGraphBufferHandle rectLights,
    ParameterInstance& parameters)
{
	parameters->TemporalReservoirSample = builder.CreateSRV(shadowSignals.TemporalReservoirSample);
	parameters->TemporalReservoirWeight = builder.CreateSRV(shadowSignals.TemporalReservoirWeight);
	parameters->CurrentReservoirSample = builder.CreateUAV(shadowSignals.CurrentReservoirSample);
	parameters->CurrentReservoirWeight = builder.CreateUAV(shadowSignals.CurrentReservoirWeight);
	parameters->CurrentReservoirSurface = builder.CreateUAV(shadowSignals.CurrentReservoirSurface);
	DirectLightReservoirPassCommon::DeclareResources(
	    builder,
	    sceneDepth,
	    gbuffer,
	    directionalLights,
	    pointLights,
	    spotLights,
	    rectLights,
	    *parameters);
}

void DirectLightReservoirSpatialPass::SetParameters(
    ParameterInstance& parameters,
    const FrameContext& frame,
    const RenderViewData& viewData,
    const PassRuntimeServices& passRuntimeServices) const
{
	DirectLightReservoirPassCommon::SetParameters(*parameters, frame, viewData, passRuntimeServices);
	parameters->PerTemporal = viewData.perTemporalData;
}

void DirectLightReservoirSpatialPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	SetParameters(parameters, context.Frame, context.Frame.mainView, context.RuntimeServices);
	ComputePassUtilities::DispatchSized<DirectLightReservoirSpatialPass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
}

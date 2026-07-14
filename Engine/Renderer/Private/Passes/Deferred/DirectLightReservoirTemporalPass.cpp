#include "../../PCH.h"
#include "Passes/Deferred/DirectLightReservoirTemporalPass.h"

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

DirectLightReservoirTemporalPass::DirectLightReservoirTemporalPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime)
{
}

const DirectLightReservoirTemporalPass::ParameterMetadata& DirectLightReservoirTemporalPass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<DirectLightReservoirTemporalPass>();
}

const RenderPassDefinition& DirectLightReservoirTemporalPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    RendererShaderPackages::DirectLightReservoirTemporal,
	    L"DirectLightReservoirTemporal_BindingLayout",
	    L"DirectLightReservoirTemporal_PipelineState");
	return definition;
}

void DirectLightReservoirTemporalPass::DeclareResources(
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
	parameters->TemporalReservoirSample = builder.CreateUAV(shadowSignals.TemporalReservoirSample);
	parameters->TemporalReservoirWeight = builder.CreateUAV(shadowSignals.TemporalReservoirWeight);
	parameters->PreviousReservoirSample = builder.CreateSRV(shadowSignals.ReservoirHistory.Sample.Previous);
	parameters->PreviousReservoirWeight = builder.CreateSRV(shadowSignals.ReservoirHistory.Weight.Previous);
	parameters->PreviousReservoirSurface = builder.CreateSRV(shadowSignals.ReservoirHistory.Surface.Previous);
	DirectLightReservoirPassCommon::DeclareResources(
	    builder,
	    sceneDepth,
	    gbuffer,
	    directionalLights,
	    pointLights,
	    spotLights,
	    rectLights,
	    *parameters);
	parameters->GBufferMotionVector = builder.CreateSRV(gbuffer.MotionVector);
}

void DirectLightReservoirTemporalPass::SetParameters(
    ParameterInstance& parameters,
    const FrameContext& frame,
    const RenderViewData& viewData,
    const PassRuntimeServices& passRuntimeServices) const
{
	DirectLightReservoirPassCommon::SetParameters(*parameters, frame, viewData, passRuntimeServices);
	PerTemporalConstantBufferData reservoirTemporalData = viewData.perTemporalData;
	if (!passRuntimeServices.History.DirectLightReservoir)
	{
		reservoirTemporalData.HistoryValid = 0u;
	}
	parameters->PerTemporal = reservoirTemporalData;
}

void DirectLightReservoirTemporalPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	SetParameters(parameters, context.Frame, context.Frame.mainView, context.RuntimeServices);
	ComputePassUtilities::DispatchSized<DirectLightReservoirTemporalPass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
}

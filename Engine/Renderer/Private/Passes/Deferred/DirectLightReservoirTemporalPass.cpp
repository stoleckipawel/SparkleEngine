#include "../../PCH.h"
#include "Passes/Deferred/DirectLightReservoirTemporalPass.h"

#include "Frame/Core/FrameContext.h"
#include "View/RenderView.h"
#include "Frame/Lighting/ShadowVisibility.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeContext.h"
#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

DirectLightReservoirTemporalPass::DirectLightReservoirTemporalPass(const ComputePassPipelineRuntime& runtime) noexcept :
    m_runtime(runtime)
{
}

const DirectLightReservoirTemporalPass::ParameterMetadata& DirectLightReservoirTemporalPass::GetParameterMetadata() noexcept
{
	return ComputePassOperations::BuildParameterMetadata<DirectLightReservoirTemporalPass>();
}

const RenderPassDefinition& DirectLightReservoirTemporalPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassOperations::BuildDefinition(
	    PassName,
	    RendererShaderPackages::DirectLightReservoirTemporal,
	    L"DirectLightReservoirTemporal_BindingLayout",
	    L"DirectLightReservoirTemporal_Pipeline");
	return definition;
}

void DirectLightReservoirTemporalPass::SetParameters(
    ParameterInstance& parameters,
    const FrameContext& frame,
    const RenderView& view,
    const PassRuntimeContext& passRuntimeContext) const
{
	DirectLightReservoirPassCommon::SetParameters(*parameters, frame, view, passRuntimeContext);
	ViewTemporalUniformData reservoirTemporalData = view.temporalUniform;
	if (!passRuntimeContext.History.DirectLightReservoir)
	{
		reservoirTemporalData.HistoryValid = 0u;
	}
	parameters->ViewTemporal = reservoirTemporalData;
}

void DirectLightReservoirTemporalPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	SetParameters(parameters, context.Frame, context.Frame.view, context.Runtime);
	ComputePassOperations::DispatchSized<DirectLightReservoirTemporalPass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.view.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.view.viewport.Height));
}

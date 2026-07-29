#include "../../PCH.h"
#include "Passes/Deferred/DirectLightReservoirSpatialPass.h"

#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
#include "Frame/Lighting/ShadowVisibility.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeContext.h"
#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

DirectLightReservoirSpatialPass::DirectLightReservoirSpatialPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const DirectLightReservoirSpatialPass::ParameterMetadata& DirectLightReservoirSpatialPass::GetParameterMetadata() noexcept
{
	return ComputePassOperations::BuildParameterMetadata<DirectLightReservoirSpatialPass>();
}

const RenderPassDefinition& DirectLightReservoirSpatialPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassOperations::BuildDefinition(
	    PassName,
	    RendererShaderPackages::DirectLightReservoirSpatial,
	    L"DirectLightReservoirSpatial_BindingLayout",
	    L"DirectLightReservoirSpatial_Pipeline");
	return definition;
}

void DirectLightReservoirSpatialPass::SetParameters(
    ParameterInstance& parameters,
    const FrameContext& frame,
    const RenderViewData& viewData,
    const PassRuntimeContext& passRuntimeContext) const
{
	DirectLightReservoirPassCommon::SetParameters(*parameters, frame, viewData, passRuntimeContext);
	parameters->PerTemporal = viewData.perTemporalData;
}

void DirectLightReservoirSpatialPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	SetParameters(parameters, context.Frame, context.Frame.mainView, context.Runtime);
	ComputePassOperations::DispatchSized<DirectLightReservoirSpatialPass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
}

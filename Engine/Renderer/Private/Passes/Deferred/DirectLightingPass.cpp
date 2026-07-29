#include "../../PCH.h"
#include "Passes/Deferred/DirectLightingPass.h"

#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
#include "Frame/Lighting/ShadowVisibility.h"
#include "FrameGraph/PassRuntimeContext.h"
#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

DirectLightingPass::DirectLightingPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const DirectLightingPass::ParameterMetadata& DirectLightingPass::GetParameterMetadata() noexcept
{
	return ComputePassOperations::BuildParameterMetadata<DirectLightingPass>();
}

const RenderPassDefinition& DirectLightingPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassOperations::BuildDefinition(
	    PassName,
	    RendererShaderPackages::DirectLighting,
	    L"DirectLighting_BindingLayout",
	    L"DirectLighting_Pipeline");
	return definition;
}

void DirectLightingPass::SetParameters(
    ParameterInstance& parameters,
    const FrameContext& frame,
    const RenderViewData& viewData,
    const PassRuntimeContext& passRuntimeContext) const
{
	parameters->PerFrame = passRuntimeContext.PerFrame;
	parameters->PerView = viewData.perViewData;
	parameters->PerTemporal = viewData.perTemporalData;
	parameters->ViewLighting = frame.sceneGpuData->Lighting.Constants;
}

void DirectLightingPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	SetParameters(parameters, context.Frame, context.Frame.mainView, context.Runtime);
	{
		ComputePassOperations::DispatchSized<DirectLightingPass>(
		    context,
		    m_runtime,
		    parameters,
		    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
		    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
	}
}

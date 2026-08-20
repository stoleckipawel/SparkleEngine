#include "../../PCH.h"
#include "Passes/Deferred/DirectLightingPass.h"

#include "Frame/Core/FrameContext.h"
#include "View/RenderView.h"
#include "Frame/Lighting/ShadowVisibility.h"
#include "FrameGraph/PassRuntimeContext.h"
#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

DirectLightingPass::DirectLightingPass(const ComputePassPipelineRuntime& runtime) noexcept :
    m_runtime(runtime)
{
}

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
    const RenderView& view,
    const PassRuntimeContext& passRuntimeContext) const
{
	parameters->Frame = passRuntimeContext.Frame;
	parameters->View = view.uniform;
	parameters->ViewCamera = view.cameraUniform;
	parameters->ViewTemporal = view.temporalUniform;
	parameters->ViewLighting = frame.sceneGpuData->Lighting.Constants;
}

void DirectLightingPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	SetParameters(parameters, context.Frame, context.Frame.view, context.Runtime);
	{
		ComputePassOperations::DispatchSized<DirectLightingPass>(
		    context,
		    m_runtime,
		    parameters,
		    static_cast<std::uint32_t>(context.Frame.view.viewport.Width),
		    static_cast<std::uint32_t>(context.Frame.view.viewport.Height));
	}
}

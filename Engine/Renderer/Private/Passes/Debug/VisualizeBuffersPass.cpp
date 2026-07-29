#include "../../PCH.h"
#include "Passes/Debug/VisualizeBuffersPass.h"

#include "Frame/Core/RenderViewData.h"
#include "FrameGraph/PassRuntimeContext.h"
#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

VisualizeBuffersPass::VisualizeBuffersPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const VisualizeBuffersPass::ParameterMetadata& VisualizeBuffersPass::GetParameterMetadata() noexcept
{
	return ComputePassOperations::BuildParameterMetadata<VisualizeBuffersPass>();
}

const RenderPassDefinition& VisualizeBuffersPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassOperations::BuildDefinition(
	    PassName,
	    RendererShaderPackages::VisualizeBuffers,
	    L"VisualizeBuffers_BindingLayout",
	    L"VisualizeBuffers_Pipeline");
	return definition;
}

void VisualizeBuffersPass::SetParameters(
    ParameterInstance& parameters,
    const RenderViewData& viewData,
    const PassRuntimeContext& passRuntimeContext) const
{
	(void) viewData;
	parameters->PerFrame = passRuntimeContext.PerFrame;
}

void VisualizeBuffersPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	SetParameters(parameters, context.Frame.mainView, context.Runtime);
	ComputePassOperations::DispatchSized<VisualizeBuffersPass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
}

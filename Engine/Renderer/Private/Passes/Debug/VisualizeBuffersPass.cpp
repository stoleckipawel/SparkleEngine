#include "../../PCH.h"
#include "Passes/Debug/VisualizeBuffersPass.h"

#include "Frame/Core/RenderViewData.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Passes/Core/ComputePassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

VisualizeBuffersPass::VisualizeBuffersPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const VisualizeBuffersPass::ParameterMetadata& VisualizeBuffersPass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<VisualizeBuffersPass>();
}

const RenderPassDefinition& VisualizeBuffersPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    RendererShaderPackages::VisualizeBuffers,
	    L"VisualizeBuffers_BindingLayout",
	    L"VisualizeBuffers_PipelineState");
	return definition;
}

void VisualizeBuffersPass::SetParameters(
    ParameterInstance& parameters,
    const RenderViewData& viewData,
    const PassRuntimeServices& passRuntimeServices) const
{
	(void) viewData;
	parameters->PerFrame = passRuntimeServices.PerFrame;
}

void VisualizeBuffersPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	SetParameters(parameters, context.Frame.mainView, context.RuntimeServices);
	ComputePassUtilities::DispatchSized<VisualizeBuffersPass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
}

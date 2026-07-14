#include "../../PCH.h"
#include "Passes/Deferred/SceneDepthPass.h"

#include "Frame/Core/RenderViewData.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/Core/ComputePassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

SceneDepthPass::SceneDepthPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const SceneDepthPass::ParameterMetadata& SceneDepthPass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<SceneDepthPass>();
}

const RenderPassDefinition& SceneDepthPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    RendererShaderPackages::SceneDepth,
	    L"SceneDepth_BindingLayout",
	    L"SceneDepth_PipelineState");
	return definition;
}

void SceneDepthPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	parameters->PerView = context.Frame.mainView.perViewData;
	ComputePassUtilities::DispatchSized<SceneDepthPass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
}

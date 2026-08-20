#include "../../PCH.h"
#include "Passes/Deferred/SceneDepthPass.h"

#include "View/RenderView.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

SceneDepthPass::SceneDepthPass(const ComputePassPipelineRuntime& runtime) noexcept :
    m_runtime(runtime)
{
}

const SceneDepthPass::ParameterMetadata& SceneDepthPass::GetParameterMetadata() noexcept
{
	return ComputePassOperations::BuildParameterMetadata<SceneDepthPass>();
}

const RenderPassDefinition& SceneDepthPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassOperations::BuildDefinition(
	    PassName,
	    RendererShaderPackages::SceneDepth,
	    L"SceneDepth_BindingLayout",
	    L"SceneDepth_Pipeline");
	return definition;
}

void SceneDepthPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	parameters->ViewCamera = context.Frame.view.cameraUniform;
	ComputePassOperations::DispatchSized<SceneDepthPass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.view.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.view.viewport.Height));
}

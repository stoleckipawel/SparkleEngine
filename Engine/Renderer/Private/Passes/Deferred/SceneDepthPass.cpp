#include "../../PCH.h"
#include "Passes/Deferred/SceneDepthPass.h"

#include "FrameGraph/Execution/PassCommandContext.h"
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

void SceneDepthPass::Execute(
    PassCommandContext& context,
    ParameterInstance& parameters,
    std::uint32_t outputWidth,
    std::uint32_t outputHeight) const
{
	ComputePassOperations::DispatchSized<SceneDepthPass>(context, m_runtime, parameters, outputWidth, outputHeight);
}

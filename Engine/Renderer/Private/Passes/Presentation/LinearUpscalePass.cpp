#include "../../PCH.h"
#include "Passes/Presentation/LinearUpscalePass.h"

#include "FrameGraph/Execution/PassCommandContext.h"
#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

LinearUpscalePass::LinearUpscalePass(const ComputePassPipelineRuntime& runtime) noexcept :
    m_runtime(runtime)
{
}

const LinearUpscalePass::ParameterMetadata& LinearUpscalePass::GetParameterMetadata() noexcept
{
	return ComputePassOperations::BuildParameterMetadata<LinearUpscalePass>();
}

const RenderPassDefinition& LinearUpscalePass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassOperations::BuildDefinition(
	    PassName,
	    RendererShaderPackages::LinearUpscale,
	    L"LinearUpscale_BindingLayout",
	    L"LinearUpscale_Pipeline");
	return definition;
}

void LinearUpscalePass::Execute(
    PassCommandContext& context,
    ParameterInstance& parameters,
    std::uint32_t outputWidth,
    std::uint32_t outputHeight) const
{
	ComputePassOperations::DispatchSized<LinearUpscalePass>(context, m_runtime, parameters, outputWidth, outputHeight);
}

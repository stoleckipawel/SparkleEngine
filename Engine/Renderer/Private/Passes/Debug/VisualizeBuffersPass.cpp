#include "../../PCH.h"
#include "Passes/Debug/VisualizeBuffersPass.h"

#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "FrameGraph/Execution/PassCommandContext.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

VisualizeBuffersPass::VisualizeBuffersPass(const ComputePassPipelineRuntime& runtime) noexcept :
    m_runtime(runtime)
{
}

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

void VisualizeBuffersPass::Execute(
    PassCommandContext& context,
    ParameterInstance& parameters,
    std::uint32_t outputWidth,
    std::uint32_t outputHeight) const
{
	ComputePassOperations::DispatchSized<VisualizeBuffersPass>(context, m_runtime, parameters, outputWidth, outputHeight);
}

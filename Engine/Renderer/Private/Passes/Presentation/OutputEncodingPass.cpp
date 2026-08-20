#include "../../PCH.h"
#include "Passes/Presentation/OutputEncodingPass.h"

#include "FrameGraph/Execution/PassCommandContext.h"
#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

OutputEncodingPass::OutputEncodingPass(const ComputePassPipelineRuntime& runtime) noexcept :
    m_runtime(runtime)
{
}

const OutputEncodingPass::ParameterMetadata& OutputEncodingPass::GetParameterMetadata() noexcept
{
	return ComputePassOperations::BuildParameterMetadata<OutputEncodingPass>();
}

const RenderPassDefinition& OutputEncodingPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassOperations::BuildDefinition(
	    PassName,
	    RendererShaderPackages::OutputEncoding,
	    L"OutputEncoding_BindingLayout",
	    L"OutputEncoding_Pipeline");
	return definition;
}

void OutputEncodingPass::Execute(
    PassCommandContext& context,
    ParameterInstance& parameters,
    std::uint32_t outputWidth,
    std::uint32_t outputHeight) const
{
	ComputePassOperations::DispatchSized<OutputEncodingPass>(context, m_runtime, parameters, outputWidth, outputHeight);
}

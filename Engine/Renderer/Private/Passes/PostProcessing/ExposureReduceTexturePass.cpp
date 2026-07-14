#include "../../PCH.h"
#include "Passes/PostProcessing/ExposureReduceTexturePass.h"

#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/Core/ComputePassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

ExposureReduceTexturePass::ExposureReduceTexturePass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const ExposureReduceTexturePass::ParameterMetadata& ExposureReduceTexturePass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<ExposureReduceTexturePass>();
}

const RenderPassDefinition& ExposureReduceTexturePass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    RendererShaderPackages::ExposureReduceTexture,
	    L"ExposureReduceTexture_BindingLayout",
	    L"ExposureReduceTexture_PipelineState");
	return definition;
}

void ExposureReduceTexturePass::Execute(
    PassExecutionContext& context,
    ParameterInstance& parameters,
    std::uint32_t outputWidth,
    std::uint32_t outputHeight) const
{
	ComputePassUtilities::DispatchSized<ExposureReduceTexturePass>(context, m_runtime, parameters, outputWidth, outputHeight);
}

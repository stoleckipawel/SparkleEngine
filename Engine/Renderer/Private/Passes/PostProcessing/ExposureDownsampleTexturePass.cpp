#include "../../PCH.h"
#include "Passes/PostProcessing/ExposureDownsampleTexturePass.h"

#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

ExposureDownsampleTexturePass::ExposureDownsampleTexturePass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const ExposureDownsampleTexturePass::ParameterMetadata& ExposureDownsampleTexturePass::GetParameterMetadata() noexcept
{
	return ComputePassOperations::BuildParameterMetadata<ExposureDownsampleTexturePass>();
}

const RenderPassDefinition& ExposureDownsampleTexturePass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassOperations::BuildDefinition(
	    PassName,
	    RendererShaderPackages::ExposureDownsampleTexture,
	    L"ExposureDownsampleTexture_BindingLayout",
	    L"ExposureDownsampleTexture_Pipeline");
	return definition;
}

void ExposureDownsampleTexturePass::Execute(
    PassExecutionContext& context,
    ParameterInstance& parameters,
    std::uint32_t outputWidth,
    std::uint32_t outputHeight) const
{
	ComputePassOperations::DispatchSized<ExposureDownsampleTexturePass>(context, m_runtime, parameters, outputWidth, outputHeight);
}

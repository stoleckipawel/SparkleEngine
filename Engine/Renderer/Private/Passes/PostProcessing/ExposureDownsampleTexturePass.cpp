#include "../../PCH.h"
#include "Passes/PostProcessing/ExposureDownsampleTexturePass.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/Core/ComputePassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

ExposureDownsampleTexturePass::ExposureDownsampleTexturePass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const ExposureDownsampleTexturePass::ParameterMetadata& ExposureDownsampleTexturePass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<ExposureDownsampleTexturePass>();
}

const RenderPassDefinition& ExposureDownsampleTexturePass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    RendererShaderPackages::ExposureDownsampleTexture,
	    L"ExposureDownsampleTexture_BindingLayout",
	    L"ExposureDownsampleTexture_PipelineState");
	return definition;
}

void ExposureDownsampleTexturePass::DeclareResources(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle inputMoments,
    FrameGraphTextureHandle outputMoments,
    ParameterInstance& parameters)
{
	parameters->LuminanceMomentsInput = builder.CreateSRV(inputMoments);
	parameters->LuminanceMomentsOutput = builder.CreateUAV(outputMoments);
}

void ExposureDownsampleTexturePass::Execute(
    PassExecutionContext& context,
    ParameterInstance& parameters,
    std::uint32_t outputWidth,
    std::uint32_t outputHeight) const
{
	ComputePassUtilities::DispatchSized<ExposureDownsampleTexturePass>(context, m_runtime, parameters, outputWidth, outputHeight);
}

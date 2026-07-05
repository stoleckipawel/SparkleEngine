#include "../../PCH.h"
#include "Passes/Presentation/OutputEncodingPass.h"

#include "Frame/Presentation/OutputEncodingSettings.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/Core/ComputePassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

OutputEncodingPass::OutputEncodingPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const OutputEncodingPass::ParameterMetadata& OutputEncodingPass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<OutputEncodingPass>();
}

const RenderPassDefinition& OutputEncodingPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    RendererShaderPackages::OutputEncoding,
	    L"OutputEncoding_BindingLayout",
	    L"OutputEncoding_PipelineState");
	return definition;
}

void OutputEncodingPass::DeclareResources(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle displayLinearColor,
    FrameGraphTextureHandle encodedColor,
    ParameterInstance& parameters)
{
	parameters->EncodedColor = builder.CreateUAV(encodedColor);
	parameters->DisplayLinearColor = builder.CreateSRV(displayLinearColor);
}

void OutputEncodingPass::Execute(
    PassExecutionContext& context,
    ParameterInstance& parameters,
    std::uint32_t outputWidth,
    std::uint32_t outputHeight) const
{
	parameters->OutputEncodingConstants = BuildOutputEncodingUniformDataFromCVars();
	ComputePassUtilities::DispatchSized<OutputEncodingPass>(context, m_runtime, parameters, outputWidth, outputHeight);
}

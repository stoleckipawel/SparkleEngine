#include "../../PCH.h"
#include "Passes/PostProcessing/ExposureDownsampleScenePass.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/Core/ComputePassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

ExposureDownsampleScenePass::ExposureDownsampleScenePass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const ExposureDownsampleScenePass::ParameterMetadata& ExposureDownsampleScenePass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<ExposureDownsampleScenePass>();
}

const RenderPassDefinition& ExposureDownsampleScenePass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    RendererShaderPackages::ExposureDownsampleScene,
	    L"ExposureDownsampleScene_BindingLayout",
	    L"ExposureDownsampleScene_PipelineState");
	return definition;
}

void ExposureDownsampleScenePass::DeclareResources(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle finalSceneColor,
    FrameGraphTextureHandle luminanceMoments,
    ParameterInstance& parameters)
{
	parameters->SceneColor = builder.CreateSRV(finalSceneColor);
	parameters->LuminanceMomentsOutput = builder.CreateUAV(luminanceMoments);
}

void ExposureDownsampleScenePass::Execute(
    PassExecutionContext& context,
    ParameterInstance& parameters,
    std::uint32_t outputWidth,
    std::uint32_t outputHeight) const
{
	ComputePassUtilities::DispatchSized<ExposureDownsampleScenePass>(context, m_runtime, parameters, outputWidth, outputHeight);
}

#include "../../PCH.h"
#include "Passes/PostProcessing/ExposureReduceScenePass.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/Core/ComputePassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

ExposureReduceScenePass::ExposureReduceScenePass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const ExposureReduceScenePass::ParameterMetadata& ExposureReduceScenePass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<ExposureReduceScenePass>();
}

const RenderPassDefinition& ExposureReduceScenePass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    RendererShaderPackages::ExposureReduceScene,
	    L"ExposureReduceScene_BindingLayout",
	    L"ExposureReduceScene_PipelineState");
	return definition;
}

void ExposureReduceScenePass::DeclareResources(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle finalSceneColor,
    FrameGraphTextureHandle luminanceMoments,
    ParameterInstance& parameters)
{
	parameters->SceneColor = builder.CreateSRV(finalSceneColor);
	parameters->LuminanceMomentsOutput = builder.CreateUAV(luminanceMoments);
}

void ExposureReduceScenePass::Execute(
    PassExecutionContext& context,
    ParameterInstance& parameters,
    std::uint32_t outputWidth,
    std::uint32_t outputHeight) const
{
	ComputePassUtilities::DispatchSized<ExposureReduceScenePass>(context, m_runtime, parameters, outputWidth, outputHeight);
}

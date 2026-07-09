#include "../../PCH.h"
#include "Passes/PostProcessing/ExposurePass.h"

#include "Frame/Presentation/ToneMappingSettings.h"
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

void ExposureReduceTexturePass::DeclareResources(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle inputMoments,
    FrameGraphTextureHandle outputMoments,
    ParameterInstance& parameters)
{
	parameters->LuminanceMomentsInput = builder.CreateSRV(inputMoments);
	parameters->LuminanceMomentsOutput = builder.CreateUAV(outputMoments);
}

void ExposureReduceTexturePass::Execute(
    PassExecutionContext& context,
    ParameterInstance& parameters,
    std::uint32_t outputWidth,
    std::uint32_t outputHeight) const
{
	ComputePassUtilities::DispatchSized<ExposureReduceTexturePass>(context, m_runtime, parameters, outputWidth, outputHeight);
}

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

ExposurePass::ExposurePass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const ExposurePass::ParameterMetadata& ExposurePass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<ExposurePass>();
}

const RenderPassDefinition& ExposurePass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    RendererShaderPackages::Exposure,
	    L"Exposure_BindingLayout",
	    L"Exposure_PipelineState");
	return definition;
}

void ExposurePass::DeclareResources(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle luminanceMoments,
    FrameGraphTextureHandle previousExposure,
    FrameGraphTextureHandle currentExposure,
    FrameGraphTextureHandle exposure,
    ParameterInstance& parameters)
{
	parameters->ExposureTexture = builder.CreateUAV(exposure);
	parameters->ExposureHistoryTexture = builder.CreateUAV(currentExposure);
	parameters->PreviousExposureTexture = builder.CreateSRV(previousExposure);
	parameters->LuminanceMoments = builder.CreateSRV(luminanceMoments);
}

void ExposurePass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	parameters->ExposureConstants = BuildExposureUniformData(
	    context.RuntimeServices.PerFrame.DeltaTime,
	    context.RuntimeServices.ExposureHistoryValid);
	ComputePassUtilities::Dispatch<ExposurePass>(context, m_runtime, parameters, ComputeDispatchDesc{1u, 1u, 1u});
}

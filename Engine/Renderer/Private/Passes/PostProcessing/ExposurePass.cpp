#include "../../PCH.h"
#include "Passes/PostProcessing/ExposurePass.h"

#include "Frame/Presentation/ToneMappingSettings.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/Core/ComputePassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

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
	parameters->ExposureConstants =
	    BuildExposureUniformData(context.RuntimeServices.PerFrame.DeltaTime, context.RuntimeServices.ExposureHistoryValid);
	ComputePassUtilities::Dispatch<ExposurePass>(context, m_runtime, parameters, ComputeDispatchDesc{1u, 1u, 1u});
}

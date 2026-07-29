#include "../../PCH.h"
#include "Passes/PostProcessing/ExposurePass.h"

#include "Frame/Presentation/ToneMappingSettings.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

ExposurePass::ExposurePass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const ExposurePass::ParameterMetadata& ExposurePass::GetParameterMetadata() noexcept
{
	return ComputePassOperations::BuildParameterMetadata<ExposurePass>();
}

const RenderPassDefinition& ExposurePass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassOperations::BuildDefinition(
	    PassName,
	    RendererShaderPackages::Exposure,
	    L"Exposure_BindingLayout",
	    L"Exposure_Pipeline");
	return definition;
}

void ExposurePass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	parameters->ExposureConstants =
	    BuildExposureUniformData(context.Runtime.PerFrame.DeltaTimeSeconds, context.Runtime.History.Exposure);
	ComputePassOperations::Dispatch<ExposurePass>(context, m_runtime, parameters, ComputeDispatchDesc{1u, 1u, 1u});
}

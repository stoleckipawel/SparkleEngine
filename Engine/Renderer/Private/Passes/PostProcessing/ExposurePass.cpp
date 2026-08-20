#include "../../PCH.h"
#include "Passes/PostProcessing/ExposurePass.h"

#include "FrameGraph/Execution/PassCommandContext.h"
#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

ExposurePass::ExposurePass(const ComputePassPipelineRuntime& runtime) noexcept :
    m_runtime(runtime)
{
}

const ExposurePass::ParameterMetadata& ExposurePass::GetParameterMetadata() noexcept
{
	return ComputePassOperations::BuildParameterMetadata<ExposurePass>();
}

const RenderPassDefinition& ExposurePass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition =
	    ComputePassOperations::BuildDefinition(PassName, RendererShaderPackages::Exposure, L"Exposure_BindingLayout", L"Exposure_Pipeline");
	return definition;
}

void ExposurePass::Execute(PassCommandContext& context, ParameterInstance& parameters) const
{
	ComputePassOperations::Dispatch<ExposurePass>(context, m_runtime, parameters, ComputeDispatchDesc{1u, 1u, 1u});
}

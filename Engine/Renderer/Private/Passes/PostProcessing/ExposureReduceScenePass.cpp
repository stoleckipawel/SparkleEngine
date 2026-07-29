#include "../../PCH.h"
#include "Passes/PostProcessing/ExposureReduceScenePass.h"

#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

ExposureReduceScenePass::ExposureReduceScenePass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const ExposureReduceScenePass::ParameterMetadata& ExposureReduceScenePass::GetParameterMetadata() noexcept
{
	return ComputePassOperations::BuildParameterMetadata<ExposureReduceScenePass>();
}

const RenderPassDefinition& ExposureReduceScenePass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassOperations::BuildDefinition(
	    PassName,
	    RendererShaderPackages::ExposureReduceScene,
	    L"ExposureReduceScene_BindingLayout",
	    L"ExposureReduceScene_Pipeline");
	return definition;
}

void ExposureReduceScenePass::Execute(
    PassExecutionContext& context,
    ParameterInstance& parameters,
    std::uint32_t outputWidth,
    std::uint32_t outputHeight) const
{
	ComputePassOperations::DispatchSized<ExposureReduceScenePass>(context, m_runtime, parameters, outputWidth, outputHeight);
}

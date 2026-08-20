#include "../../PCH.h"
#include "Passes/PostProcessing/ExposureDownsampleScenePass.h"

#include "FrameGraph/Execution/PassCommandContext.h"
#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

ExposureDownsampleScenePass::ExposureDownsampleScenePass(const ComputePassPipelineRuntime& runtime) noexcept :
    m_runtime(runtime)
{
}

const ExposureDownsampleScenePass::ParameterMetadata& ExposureDownsampleScenePass::GetParameterMetadata() noexcept
{
	return ComputePassOperations::BuildParameterMetadata<ExposureDownsampleScenePass>();
}

const RenderPassDefinition& ExposureDownsampleScenePass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassOperations::BuildDefinition(
	    PassName,
	    RendererShaderPackages::ExposureDownsampleScene,
	    L"ExposureDownsampleScene_BindingLayout",
	    L"ExposureDownsampleScene_Pipeline");
	return definition;
}

void ExposureDownsampleScenePass::Execute(
    PassCommandContext& context,
    ParameterInstance& parameters,
    std::uint32_t outputWidth,
    std::uint32_t outputHeight) const
{
	ComputePassOperations::DispatchSized<ExposureDownsampleScenePass>(context, m_runtime, parameters, outputWidth, outputHeight);
}

#include "../../PCH.h"
#include "Passes/GBuffer/SkyMotionVectorPass.h"

#include "FrameGraph/Execution/PassCommandContext.h"
#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

SkyMotionVectorPass::SkyMotionVectorPass(const ComputePassPipelineRuntime& runtime) noexcept :
    m_runtime(runtime)
{
}

const SkyMotionVectorPass::ParameterMetadata& SkyMotionVectorPass::GetParameterMetadata() noexcept
{
	return ComputePassOperations::BuildParameterMetadata<SkyMotionVectorPass>();
}

const RenderPassDefinition& SkyMotionVectorPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassOperations::BuildDefinition(
	    PassName,
	    RendererShaderPackages::SkyMotionVector,
	    L"SkyMotionVector_BindingLayout",
	    L"SkyMotionVector_Pipeline");
	return definition;
}

void SkyMotionVectorPass::Execute(
    PassCommandContext& context,
    ParameterInstance& parameters,
    std::uint32_t outputWidth,
    std::uint32_t outputHeight) const
{
	ComputePassOperations::DispatchSized<SkyMotionVectorPass>(context, m_runtime, parameters, outputWidth, outputHeight);
}

#include "../../PCH.h"
#include "Passes/Deferred/SkyPass.h"

#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "FrameGraph/Execution/PassCommandContext.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

SkyPass::SkyPass(const ComputePassPipelineRuntime& runtime) noexcept :
    m_runtime(runtime)
{
}

const SkyPass::ParameterMetadata& SkyPass::GetParameterMetadata() noexcept
{
	return ComputePassOperations::BuildParameterMetadata<SkyPass>();
}

const RenderPassDefinition& SkyPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition =
	    ComputePassOperations::BuildDefinition(PassName, RendererShaderPackages::Sky, L"Sky_BindingLayout", L"Sky_Pipeline");
	return definition;
}

void SkyPass::Execute(PassCommandContext& context, ParameterInstance& parameters, std::uint32_t outputWidth, std::uint32_t outputHeight)
    const
{
	ComputePassOperations::DispatchSized<SkyPass>(context, m_runtime, parameters, outputWidth, outputHeight);
}

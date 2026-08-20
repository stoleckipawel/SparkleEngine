#include "../../PCH.h"
#include "Passes/Deferred/LightingCompositePass.h"

#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "FrameGraph/Execution/PassCommandContext.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

LightingCompositePass::LightingCompositePass(const ComputePassPipelineRuntime& runtime) noexcept :
    m_runtime(runtime)
{
}

const LightingCompositePass::ParameterMetadata& LightingCompositePass::GetParameterMetadata() noexcept
{
	return ComputePassOperations::BuildParameterMetadata<LightingCompositePass>();
}

const RenderPassDefinition& LightingCompositePass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassOperations::BuildDefinition(
	    PassName,
	    RendererShaderPackages::LightingComposite,
	    L"LightingComposite_BindingLayout",
	    L"LightingComposite_Pipeline");
	return definition;
}

void LightingCompositePass::Execute(
    PassCommandContext& context,
    ParameterInstance& parameters,
    std::uint32_t outputWidth,
    std::uint32_t outputHeight) const
{
	ComputePassOperations::DispatchSized<LightingCompositePass>(context, m_runtime, parameters, outputWidth, outputHeight);
}

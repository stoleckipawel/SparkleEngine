#include "../../PCH.h"
#include "Passes/Deferred/LightingCompositePass.h"

#include "Passes/Core/ComputePassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

LightingCompositePass::LightingCompositePass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const LightingCompositePass::ParameterMetadata& LightingCompositePass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<LightingCompositePass>();
}

const RenderPassDefinition& LightingCompositePass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    RendererShaderPackages::LightingComposite,
	    L"LightingComposite_BindingLayout",
	    L"LightingComposite_PipelineState");
	return definition;
}

void LightingCompositePass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	ComputePassUtilities::DispatchSized<LightingCompositePass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
}

#include "../../PCH.h"
#include "Passes/Deferred/DirectShadowSignalNoRayQueryPass.h"

#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

DirectShadowSignalNoRayQueryPass::DirectShadowSignalNoRayQueryPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime)
{
}

const DirectShadowSignalNoRayQueryPass::ParameterMetadata& DirectShadowSignalNoRayQueryPass::GetParameterMetadata() noexcept
{
	return ComputePassOperations::BuildParameterMetadata<DirectShadowSignalNoRayQueryPass>();
}

const RenderPassDefinition& DirectShadowSignalNoRayQueryPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassOperations::BuildDefinition(
	    PassName,
	    RendererShaderPackages::DirectShadowSignalNoRayQuery,
	    L"DirectShadowSignalNoRayQuery_BindingLayout",
	    L"DirectShadowSignalNoRayQuery_Pipeline");
	return definition;
}

void DirectShadowSignalNoRayQueryPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	DirectShadowSignalPassCommon::SetParameters(*parameters, context.Frame, context.Frame.mainView, context.Runtime);
	ComputePassOperations::DispatchSized<DirectShadowSignalNoRayQueryPass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
}

#include "../../../PCH.h"
#include "Passes/Lighting/Shadows/DirectShadowSignalNoRayQueryPass.h"

#include "FrameGraph/Execution/PassCommandContext.h"
#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

DirectShadowSignalNoRayQueryPass::DirectShadowSignalNoRayQueryPass(const ComputePassPipelineRuntime& runtime) noexcept :
    m_runtime(runtime)
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

void DirectShadowSignalNoRayQueryPass::Execute(
    PassCommandContext& context,
    ParameterInstance& parameters,
    std::uint32_t outputWidth,
    std::uint32_t outputHeight) const
{
	ComputePassOperations::DispatchSized<DirectShadowSignalNoRayQueryPass>(context, m_runtime, parameters, outputWidth, outputHeight);
}

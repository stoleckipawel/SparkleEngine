#include "../../../PCH.h"

#include "Passes/Lighting/Shadows/DirectShadowSignalPass.h"

#include "FrameGraph/Execution/PassCommandContext.h"
#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "RayTracing/RayTracingShaderFeatureFlags.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

DirectShadowSignalPass::DirectShadowSignalPass(const ComputePassPipelineRuntime& runtime) noexcept :
    m_runtime(runtime)
{
}

const DirectShadowSignalPass::ParameterMetadata& DirectShadowSignalPass::GetParameterMetadata() noexcept
{
	return ComputePassOperations::BuildParameterMetadata<DirectShadowSignalPass>();
}

const RenderPassDefinition& DirectShadowSignalPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassOperations::BuildDefinition(
	    PassName,
	    RendererShaderPackages::DirectShadowSignal,
	    L"DirectShadowSignal_BindingLayout",
	    L"DirectShadowSignal_Pipeline",
	    RayTracingShaderFeatureFlags::InlineRayQuery);
	return definition;
}

void DirectShadowSignalPass::Execute(
    PassCommandContext& context,
    ParameterInstance& parameters,
    std::uint32_t outputWidth,
    std::uint32_t outputHeight) const
{
	ComputePassOperations::DispatchSized<DirectShadowSignalPass>(context, m_runtime, parameters, outputWidth, outputHeight);
}

#include "../../../PCH.h"

#include "Passes/Lighting/Direct/DirectLightingPass.h"

#include "Passes/Lighting/Shadows/ShadowVisibility.h"
#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "FrameGraph/Execution/PassCommandContext.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

DirectLightingPass::DirectLightingPass(const ComputePassPipelineRuntime& runtime) noexcept :
    m_runtime(runtime)
{
}

const DirectLightingPass::ParameterMetadata& DirectLightingPass::GetParameterMetadata() noexcept
{
	return ComputePassOperations::BuildParameterMetadata<DirectLightingPass>();
}

const RenderPassDefinition& DirectLightingPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassOperations::BuildDefinition(
	    PassName,
	    RendererShaderPackages::DirectLighting,
	    L"DirectLighting_BindingLayout",
	    L"DirectLighting_Pipeline");
	return definition;
}

void DirectLightingPass::Execute(
    PassCommandContext& context,
    ParameterInstance& parameters,
    std::uint32_t outputWidth,
    std::uint32_t outputHeight) const
{
	ComputePassOperations::DispatchSized<DirectLightingPass>(context, m_runtime, parameters, outputWidth, outputHeight);
}

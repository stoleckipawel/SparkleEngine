#include "../../../PCH.h"
#include "Passes/Lighting/Direct/DirectLightReservoirTemporalPass.h"

#include "Passes/Lighting/Shadows/ShadowVisibility.h"
#include "FrameGraph/Execution/PassCommandContext.h"
#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

DirectLightReservoirTemporalPass::DirectLightReservoirTemporalPass(const ComputePassPipelineRuntime& runtime) noexcept :
    m_runtime(runtime)
{
}

const DirectLightReservoirTemporalPass::ParameterMetadata& DirectLightReservoirTemporalPass::GetParameterMetadata() noexcept
{
	return ComputePassOperations::BuildParameterMetadata<DirectLightReservoirTemporalPass>();
}

const RenderPassDefinition& DirectLightReservoirTemporalPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassOperations::BuildDefinition(
	    PassName,
	    RendererShaderPackages::DirectLightReservoirTemporal,
	    L"DirectLightReservoirTemporal_BindingLayout",
	    L"DirectLightReservoirTemporal_Pipeline");
	return definition;
}

void DirectLightReservoirTemporalPass::Execute(
    PassCommandContext& context,
    ParameterInstance& parameters,
    std::uint32_t outputWidth,
    std::uint32_t outputHeight) const
{
	ComputePassOperations::DispatchSized<DirectLightReservoirTemporalPass>(context, m_runtime, parameters, outputWidth, outputHeight);
}

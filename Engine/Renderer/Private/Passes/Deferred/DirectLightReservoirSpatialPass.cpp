#include "../../PCH.h"
#include "Passes/Deferred/DirectLightReservoirSpatialPass.h"

#include "Frame/Lighting/ShadowVisibility.h"
#include "FrameGraph/Execution/PassCommandContext.h"
#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

DirectLightReservoirSpatialPass::DirectLightReservoirSpatialPass(const ComputePassPipelineRuntime& runtime) noexcept :
    m_runtime(runtime)
{
}

const DirectLightReservoirSpatialPass::ParameterMetadata& DirectLightReservoirSpatialPass::GetParameterMetadata() noexcept
{
	return ComputePassOperations::BuildParameterMetadata<DirectLightReservoirSpatialPass>();
}

const RenderPassDefinition& DirectLightReservoirSpatialPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassOperations::BuildDefinition(
	    PassName,
	    RendererShaderPackages::DirectLightReservoirSpatial,
	    L"DirectLightReservoirSpatial_BindingLayout",
	    L"DirectLightReservoirSpatial_Pipeline");
	return definition;
}

void DirectLightReservoirSpatialPass::Execute(
    PassCommandContext& context,
    ParameterInstance& parameters,
    std::uint32_t outputWidth,
    std::uint32_t outputHeight) const
{
	ComputePassOperations::DispatchSized<DirectLightReservoirSpatialPass>(context, m_runtime, parameters, outputWidth, outputHeight);
}

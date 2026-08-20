#include "../../PCH.h"
#include "Passes/Presentation/ToneMappingPass.h"

#include "Frame/Core/FrameRenderFormats.h"
#include "FrameGraph/Execution/PassCommandContext.h"
#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

ToneMappingPass::ToneMappingPass(const ComputePassPipelineRuntime& runtime) noexcept :
    m_runtime(runtime)
{
}

const ToneMappingPass::ParameterMetadata& ToneMappingPass::GetParameterMetadata() noexcept
{
	return ComputePassOperations::BuildParameterMetadata<ToneMappingPass>();
}

const RenderPassDefinition& ToneMappingPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassOperations::BuildDefinition(
	    PassName,
	    RendererShaderPackages::ToneMapping,
	    L"ToneMapping_BindingLayout",
	    L"ToneMapping_Pipeline");
	return definition;
}

void ToneMappingPass::Execute(
    PassCommandContext& context,
    ParameterInstance& parameters,
    std::uint32_t outputWidth,
    std::uint32_t outputHeight) const
{
	ComputePassOperations::DispatchSized<ToneMappingPass>(context, m_runtime, parameters, outputWidth, outputHeight);
}

#include "../../PCH.h"
#include "Passes/Presentation/ToneMappingPass.h"

#include "Frame/Core/FrameRenderFormats.h"
#include "Frame/Presentation/ToneMappingSettings.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
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
    PassExecutionContext& context,
    ParameterInstance& parameters,
    std::uint32_t outputWidth,
    std::uint32_t outputHeight) const
{
	parameters->ToneMappingConstants = BuildToneMappingUniformData(context.Runtime.DisplaySettings.ToneMapper);
	ComputePassOperations::DispatchSized<ToneMappingPass>(context, m_runtime, parameters, outputWidth, outputHeight);
}

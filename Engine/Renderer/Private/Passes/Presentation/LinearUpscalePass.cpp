#include "../../PCH.h"
#include "Passes/Presentation/LinearUpscalePass.h"

#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"

LinearUpscalePass::LinearUpscalePass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const LinearUpscalePass::ParameterMetadata& LinearUpscalePass::GetParameterMetadata() noexcept
{
	return ComputePassOperations::BuildParameterMetadata<LinearUpscalePass>();
}

const RenderPassDefinition& LinearUpscalePass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassOperations::BuildDefinition(
	    PassName,
	    RendererShaderPackages::LinearUpscale,
	    L"LinearUpscale_BindingLayout",
	    L"LinearUpscale_Pipeline");
	return definition;
}

void LinearUpscalePass::Execute(
    PassExecutionContext& context,
    ParameterInstance& parameters,
    std::uint32_t outputWidth,
    std::uint32_t outputHeight) const
{
	parameters->SamplerLinearClamp = RhiSamplerDesc{
	    .MinMagFilter = RhiSamplerMinMagFilter::Linear,
	    .MipFilter = RhiSamplerMipFilter::Linear,
	    .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Clamp)};
	ComputePassOperations::DispatchSized<LinearUpscalePass>(context, m_runtime, parameters, outputWidth, outputHeight);
}

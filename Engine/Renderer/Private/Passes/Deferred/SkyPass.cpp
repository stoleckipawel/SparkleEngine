#include "../../PCH.h"
#include "Passes/Deferred/SkyPass.h"

#include "Frame/Core/FrameContext.h"
#include "FrameGraph/PassRuntimeContext.h"
#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

SkyPass::SkyPass(const ComputePassPipelineRuntime& runtime) noexcept :
    m_runtime(runtime)
{
}

const SkyPass::ParameterMetadata& SkyPass::GetParameterMetadata() noexcept
{
	return ComputePassOperations::BuildParameterMetadata<SkyPass>();
}

const RenderPassDefinition& SkyPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition =
	    ComputePassOperations::BuildDefinition(PassName, RendererShaderPackages::Sky, L"Sky_BindingLayout", L"Sky_Pipeline");
	return definition;
}

void SkyPass::SetParameters(ParameterInstance& parameters, const FrameContext& frame) const
{
	parameters->View = frame.view.uniform;
	parameters->ViewCamera = frame.view.cameraUniform;
	parameters->ViewTemporal = frame.view.temporalUniform;
	parameters->Sky = MakeSkyUniformData(frame.preparedScene.sky);
	parameters->SamplerLinearClamp = RhiSamplerDesc{
	    .MinMagFilter = RhiSamplerMinMagFilter::Linear,
	    .MipFilter = RhiSamplerMipFilter::Linear,
	    .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Clamp)};
}

void SkyPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	SetParameters(parameters, context.Frame);
	ComputePassOperations::DispatchSized<SkyPass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.view.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.view.viewport.Height));
}

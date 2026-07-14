#include "../../PCH.h"
#include "Passes/Deferred/SkyPass.h"

#include "Frame/Core/FrameContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Passes/Core/ComputePassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

SkyPass::SkyPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const SkyPass::ParameterMetadata& SkyPass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<SkyPass>();
}

const RenderPassDefinition& SkyPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition =
	    ComputePassUtilities::BuildDefinition(PassName, RendererShaderPackages::Sky, L"Sky_BindingLayout", L"Sky_PipelineState");
	return definition;
}

void SkyPass::SetParameters(ParameterInstance& parameters, const FrameContext& frame, const PassRuntimeServices& passRuntimeServices) const
{
	parameters->PerFrame = passRuntimeServices.PerFrame;
	parameters->PerView = frame.mainView.perViewData;
	parameters->PerTemporal = frame.mainView.perTemporalData;
	parameters->Sky = MakeSkyUniformData(frame.sceneData.sky);
	parameters->SamplerLinearClamp = RhiSamplerDesc{
	    .MinMagFilter = RhiSamplerMinMagFilter::Linear,
	    .MipFilter = RhiSamplerMipFilter::Linear,
	    .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Clamp)};
}

void SkyPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	SetParameters(parameters, context.Frame, context.RuntimeServices);
	ComputePassUtilities::DispatchSized<SkyPass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
}

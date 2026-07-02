#include "../../PCH.h"
#include "Passes/Deferred/SkyPass.h"

#include "Frame/Core/RenderViewData.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
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
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    "SkyShaderPackage",
	    RendererShaderPackages::Sky,
	    L"Sky_BindingLayout",
	    L"Sky_PipelineState");
	return definition;
}

void SkyPass::DeclareResources(
    FrameGraphBuilder& builder,
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer,
    ParameterInstance& parameters)
{
	parameters->SceneColor = builder.CreateUAV(sceneTargets.SceneColor);
	parameters->GBufferDeviceZ = builder.CreateSRV(gbuffer.DeviceZ);
}

void SkyPass::SetParameters(ParameterInstance& parameters, const RenderViewData& viewData, const PassRuntimeServices& passRuntimeServices) const
{
	parameters->PerFrame = passRuntimeServices.PerFrame;
	parameters->PerView = viewData.perViewData;
	parameters->SkyTexture = m_environmentMapBinding.GetTextureBinding(passRuntimeServices);
	parameters->SamplerLinearClamp = RhiSamplerDesc{
	    .MinMagFilter = RhiSamplerMinMagFilter::Linear,
	    .MipFilter = RhiSamplerMipFilter::Linear,
	    .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Clamp)};
}

void SkyPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	SetParameters(parameters, context.Frame.mainView, context.RuntimeServices);
	ComputePassUtilities::DispatchSized<SkyPass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
}

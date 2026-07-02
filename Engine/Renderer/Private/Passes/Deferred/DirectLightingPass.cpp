#include "../../PCH.h"
#include "Passes/Deferred/DirectLightingPass.h"

#include "Core/Public/Diagnostics/Trace.h"
#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
#include "Frame/Lighting/ShadowVisibility.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Diagnostics/PassExecutionDiagnostics.h"
#include "Passes/Core/ComputePassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

namespace DirectLightingPassDetails
{
	void PopulateResources(
	    FrameGraphBuilder& builder,
	    const LightingRenderTargets& lighting,
	    const GBufferRenderTargets& gbuffer,
	    const DirectShadowSignalResources& shadowSignals,
	    DirectLightingPass::ParameterInstance& parameters)
	{
		parameters->DirectDiffuse = builder.CreateUAV(lighting.DirectDiffuse);
		parameters->DirectSpecular = builder.CreateUAV(lighting.DirectSpecular);
		parameters->DirectSubsurface = builder.CreateUAV(lighting.DirectSubsurface);
		parameters->ShadowVisibilitySignal = builder.CreateSRV(shadowSignals.Visibility);
		parameters->ShadowLightSample = builder.CreateSRV(shadowSignals.LightSample);
		parameters->GBufferBaseColor = builder.CreateSRV(gbuffer.BaseColor);
		parameters->GBufferNormal = builder.CreateSRV(gbuffer.Normal);
		parameters->GBufferMaterial = builder.CreateSRV(gbuffer.Material);
		parameters->GBufferSubsurface = builder.CreateSRV(gbuffer.Subsurface);
		parameters->GBufferDeviceZ = builder.CreateSRV(gbuffer.DeviceZ);
	}
}  // namespace DirectLightingPassDetails

DirectLightingPass::DirectLightingPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const DirectLightingPass::ParameterMetadata& DirectLightingPass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<DirectLightingPass>();
}

const RenderPassDefinition& DirectLightingPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    "DirectLightingShaderPackage",
	    RendererShaderPackages::DirectLighting,
	    L"DirectLighting_BindingLayout",
	    L"DirectLighting_PipelineState");
	return definition;
}

void DirectLightingPass::DeclareResources(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer,
    const DirectShadowSignalResources& shadowSignals,
    ParameterInstance& parameters)
{
	DirectLightingPassDetails::PopulateResources(builder, lighting, gbuffer, shadowSignals, parameters);
}

void DirectLightingPass::SetParameters(
    ParameterInstance& parameters,
    const FrameContext& frame,
    const RenderViewData& viewData,
    const PassRuntimeServices& passRuntimeServices) const
{
	parameters->PerFrame = passRuntimeServices.PerFrame;
	parameters->PerView = viewData.perViewData;
	parameters->DirectionalLights = frame.lighting.GetDirectionalLightsShaderResourceView();
	parameters->PointLights = frame.lighting.GetPointLightsShaderResourceView();
	parameters->SpotLights = frame.lighting.GetSpotLightsShaderResourceView();
	parameters->RectLights = frame.lighting.GetRectLightsShaderResourceView();
}

void DirectLightingPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	SetParameters(parameters, context.Frame, context.Frame.mainView, context.RuntimeServices);
	{
		SPARKLE_GPU_SCOPE(context.Diagnostics, "Sampled Direct Lighting");
		ComputePassUtilities::DispatchSized<DirectLightingPass>(
		    context,
		    m_runtime,
		    parameters,
		    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
		    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
	}
}

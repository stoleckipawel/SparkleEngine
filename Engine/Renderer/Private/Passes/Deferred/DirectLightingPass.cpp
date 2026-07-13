#include "../../PCH.h"
#include "Passes/Deferred/DirectLightingPass.h"

#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
#include "Frame/Lighting/ShadowVisibility.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Passes/Core/ComputePassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

DirectLightingPass::DirectLightingPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const DirectLightingPass::ParameterMetadata& DirectLightingPass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<DirectLightingPass>();
}

const RenderPassDefinition& DirectLightingPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    RendererShaderPackages::DirectLighting,
	    L"DirectLighting_BindingLayout",
	    L"DirectLighting_PipelineState");
	return definition;
}

void DirectLightingPass::DeclareResources(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    FrameGraphTextureHandle sceneDepth,
    const GBufferRenderTargets& gbuffer,
    const DirectShadowSignalResources& shadowSignals,
    FrameGraphBufferHandle directionalLights,
    FrameGraphBufferHandle pointLights,
    FrameGraphBufferHandle spotLights,
    FrameGraphBufferHandle rectLights,
    ParameterInstance& parameters)
{
	parameters->DirectDiffuse = builder.CreateUAV(lighting.DirectDiffuse);
	parameters->DirectSpecular = builder.CreateUAV(lighting.DirectSpecular);
	parameters->DirectSubsurface = builder.CreateUAV(lighting.DirectSubsurface);
	parameters->ShadowVisibilitySignal = builder.CreateSRV(shadowSignals.Visibility);
	parameters->CurrentReservoirSample = builder.CreateSRV(shadowSignals.CurrentReservoirSample);
	parameters->CurrentReservoirWeight = builder.CreateSRV(shadowSignals.CurrentReservoirWeight);
	parameters->GBufferBaseColor = builder.CreateSRV(gbuffer.BaseColor);
	parameters->GBufferNormal = builder.CreateSRV(gbuffer.Normal);
	parameters->GBufferMaterial = builder.CreateSRV(gbuffer.Material);
	parameters->GBufferSubsurface = builder.CreateSRV(gbuffer.Subsurface);
	parameters->SceneDepth = builder.CreateSRV(sceneDepth);
	parameters->DirectionalLights = builder.CreateSRV(directionalLights);
	parameters->PointLights = builder.CreateSRV(pointLights);
	parameters->SpotLights = builder.CreateSRV(spotLights);
	parameters->RectLights = builder.CreateSRV(rectLights);
}

void DirectLightingPass::SetParameters(
    ParameterInstance& parameters,
    const FrameContext& frame,
    const RenderViewData& viewData,
    const PassRuntimeServices& passRuntimeServices) const
{
	parameters->PerFrame = passRuntimeServices.PerFrame;
	parameters->PerView = viewData.perViewData;
	parameters->PerTemporal = viewData.perTemporalData;
	parameters->ViewLighting = frame.lighting.GetConstants();
}

void DirectLightingPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	SetParameters(parameters, context.Frame, context.Frame.mainView, context.RuntimeServices);
	{
		ComputePassUtilities::DispatchSized<DirectLightingPass>(
		    context,
		    m_runtime,
		    parameters,
		    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
		    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
	}
}

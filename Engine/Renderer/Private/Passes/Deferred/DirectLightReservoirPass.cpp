#include "../../PCH.h"
#include "Passes/Deferred/DirectLightReservoirPass.h"

#include "Core/Public/Diagnostics/Trace.h"
#include "Diagnostics/PassExecutionDiagnostics.h"
#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
#include "Frame/Lighting/ShadowVisibility.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Passes/Bindings/LightingPassBinding.h"
#include "Passes/Core/ComputePassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

namespace DirectLightReservoirPassDetails
{
	template <typename TParameterInstance>
	void BindGBuffer(FrameGraphBuilder& builder, const GBufferRenderTargets& gbuffer, TParameterInstance& parameters)
	{
		parameters->GBufferBaseColor = builder.CreateSRV(gbuffer.BaseColor);
		parameters->GBufferNormal = builder.CreateSRV(gbuffer.Normal);
		parameters->GBufferMaterial = builder.CreateSRV(gbuffer.Material);
		parameters->GBufferSubsurface = builder.CreateSRV(gbuffer.Subsurface);
		parameters->GBufferDeviceZ = builder.CreateSRV(gbuffer.DeviceZ);
	}

	template <typename TParameterInstance>
	void SetCommonParameters(
	    TParameterInstance& parameters,
	    const FrameContext& frame,
	    const RenderViewData& viewData,
	    const PassRuntimeServices& passRuntimeServices)
	{
		parameters->PerFrame = passRuntimeServices.PerFrame;
		parameters->PerView = viewData.perViewData;
		LightingPassBinding::SetParameters(parameters, frame);
	}
}

DirectLightReservoirTemporalPass::DirectLightReservoirTemporalPass(const ComputePassPipelineRuntime& runtime) noexcept :
    m_runtime(runtime)
{
}

const DirectLightReservoirTemporalPass::ParameterMetadata& DirectLightReservoirTemporalPass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<DirectLightReservoirTemporalPass>();
}

const RenderPassDefinition& DirectLightReservoirTemporalPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    "DirectLightReservoirTemporalShaderPackage",
	    RendererShaderPackages::DirectLightReservoirTemporal,
	    L"DirectLightReservoirTemporal_BindingLayout",
	    L"DirectLightReservoirTemporal_PipelineState");
	return definition;
}

void DirectLightReservoirTemporalPass::DeclareResources(
    FrameGraphBuilder& builder,
    const GBufferRenderTargets& gbuffer,
    const DirectShadowSignalResources& shadowSignals,
    ParameterInstance& parameters)
{
	parameters->TemporalReservoirSample = builder.CreateUAV(shadowSignals.TemporalReservoirSample);
	parameters->TemporalReservoirWeight = builder.CreateUAV(shadowSignals.TemporalReservoirWeight);
	parameters->PreviousReservoirSample = builder.CreateSRV(shadowSignals.PreviousReservoirSample);
	parameters->PreviousReservoirWeight = builder.CreateSRV(shadowSignals.PreviousReservoirWeight);
	parameters->PreviousReservoirSurface = builder.CreateSRV(shadowSignals.PreviousReservoirSurface);
	DirectLightReservoirPassDetails::BindGBuffer(builder, gbuffer, parameters);
	parameters->GBufferMotionVector = builder.CreateSRV(gbuffer.MotionVector);
}

void DirectLightReservoirTemporalPass::SetParameters(
    ParameterInstance& parameters,
    const FrameContext& frame,
    const RenderViewData& viewData,
    const PassRuntimeServices& passRuntimeServices) const
{
	DirectLightReservoirPassDetails::SetCommonParameters(parameters, frame, viewData, passRuntimeServices);
	PerTemporalConstantBufferData reservoirTemporalData = viewData.perTemporalData;
	if (!passRuntimeServices.DirectLightReservoirHistoryValid)
	{
		reservoirTemporalData.HistoryValid = 0u;
	}
	parameters->PerTemporal = reservoirTemporalData;
}

void DirectLightReservoirTemporalPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	SetParameters(parameters, context.Frame, context.Frame.mainView, context.RuntimeServices);
	{
		SPARKLE_GPU_SCOPE(context.Diagnostics, "Direct Light Reservoir Temporal");
		ComputePassUtilities::DispatchSized<DirectLightReservoirTemporalPass>(
		    context,
		    m_runtime,
		    parameters,
		    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
		    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
	}
}

DirectLightReservoirSpatialPass::DirectLightReservoirSpatialPass(const ComputePassPipelineRuntime& runtime) noexcept :
    m_runtime(runtime)
{
}

const DirectLightReservoirSpatialPass::ParameterMetadata& DirectLightReservoirSpatialPass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<DirectLightReservoirSpatialPass>();
}

const RenderPassDefinition& DirectLightReservoirSpatialPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    "DirectLightReservoirSpatialShaderPackage",
	    RendererShaderPackages::DirectLightReservoirSpatial,
	    L"DirectLightReservoirSpatial_BindingLayout",
	    L"DirectLightReservoirSpatial_PipelineState");
	return definition;
}

void DirectLightReservoirSpatialPass::DeclareResources(
    FrameGraphBuilder& builder,
    const GBufferRenderTargets& gbuffer,
    const DirectShadowSignalResources& shadowSignals,
    ParameterInstance& parameters)
{
	parameters->TemporalReservoirSample = builder.CreateSRV(shadowSignals.TemporalReservoirSample);
	parameters->TemporalReservoirWeight = builder.CreateSRV(shadowSignals.TemporalReservoirWeight);
	parameters->CurrentReservoirSample = builder.CreateUAV(shadowSignals.CurrentReservoirSample);
	parameters->CurrentReservoirWeight = builder.CreateUAV(shadowSignals.CurrentReservoirWeight);
	parameters->CurrentReservoirSurface = builder.CreateUAV(shadowSignals.CurrentReservoirSurface);
	DirectLightReservoirPassDetails::BindGBuffer(builder, gbuffer, parameters);
}

void DirectLightReservoirSpatialPass::SetParameters(
    ParameterInstance& parameters,
    const FrameContext& frame,
    const RenderViewData& viewData,
    const PassRuntimeServices& passRuntimeServices) const
{
	DirectLightReservoirPassDetails::SetCommonParameters(parameters, frame, viewData, passRuntimeServices);
}

void DirectLightReservoirSpatialPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	SetParameters(parameters, context.Frame, context.Frame.mainView, context.RuntimeServices);
	{
		SPARKLE_GPU_SCOPE(context.Diagnostics, "Direct Light Reservoir Spatial");
		ComputePassUtilities::DispatchSized<DirectLightReservoirSpatialPass>(
		    context,
		    m_runtime,
		    parameters,
		    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
		    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
	}
}

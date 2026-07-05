#include "../../PCH.h"
#include "Passes/Deferred/DirectShadowSignalPass.h"

#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
#include "Frame/Lighting/ShadowVisibility.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Passes/Bindings/LightingPassBinding.h"
#include "Passes/Bindings/RayTracedShadowPassBinding.h"
#include "Passes/Core/ComputePassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

namespace DirectShadowSignalPassDetails
{
	constexpr CookedShaderPackageFeatureFlags DescriptorRayQueryFeatures =
	    CookedShaderPackageFeatureFlags::UsesInlineRayQuery |
	    CookedShaderPackageFeatureFlags::UsesAccelerationStructure |
	    CookedShaderPackageFeatureFlags::UsesDescriptorIndexing;

	constexpr CookedShaderPackageFeatureFlags DeviceAddressRayQueryFeatures =
	    CookedShaderPackageFeatureFlags::UsesInlineRayQuery |
	    CookedShaderPackageFeatureFlags::UsesAccelerationStructure |
	    CookedShaderPackageFeatureFlags::UsesAccelerationStructureDeviceAddress |
	    CookedShaderPackageFeatureFlags::UsesDescriptorIndexing;

	template <typename TParameterInstance>
	void PopulateRayResources(
	    FrameGraphBuilder& builder,
	    const GBufferRenderTargets& gbuffer,
	    const DirectShadowSignalResources& shadowSignals,
	    TParameterInstance& parameters)
	{
		parameters->ShadowVisibilitySignal = builder.CreateUAV(shadowSignals.Visibility);
		parameters->CurrentReservoirSample = builder.CreateSRV(shadowSignals.CurrentReservoirSample);
		parameters->CurrentReservoirWeight = builder.CreateSRV(shadowSignals.CurrentReservoirWeight);
		parameters->GBufferNormal = builder.CreateSRV(gbuffer.Normal);
		parameters->GBufferDeviceZ = builder.CreateSRV(gbuffer.DeviceZ);
	}

	void PopulateNoRayResources(
	    FrameGraphBuilder& builder,
	    const GBufferRenderTargets& gbuffer,
	    const DirectShadowSignalResources& shadowSignals,
	    DirectShadowSignalNoRayQueryPass::ParameterInstance& parameters)
	{
		parameters->ShadowVisibilitySignal = builder.CreateUAV(shadowSignals.Visibility);
		parameters->CurrentReservoirSample = builder.CreateSRV(shadowSignals.CurrentReservoirSample);
		parameters->CurrentReservoirWeight = builder.CreateSRV(shadowSignals.CurrentReservoirWeight);
		parameters->GBufferDeviceZ = builder.CreateSRV(gbuffer.DeviceZ);
	}

	template <typename TParameterInstance>
	void PopulateRayQueryParameters(
	    TParameterInstance& parameters,
	    const FrameContext& frame,
	    const RenderViewData& viewData,
	    const PassRuntimeServices& passRuntimeServices,
	    bool hasSceneTlas)
	{
		parameters->PerFrame = passRuntimeServices.PerFrame;
		parameters->PerView = viewData.perViewData;
		LightingPassBinding::SetParameters(parameters, frame);
		RayTracedShadowPassBinding::SetRayQueryParameters(parameters, frame, passRuntimeServices, hasSceneTlas);
	}
}

DirectShadowSignalNoRayQueryPass::DirectShadowSignalNoRayQueryPass(const ComputePassPipelineRuntime& runtime) noexcept :
    m_runtime(runtime)
{
}

const DirectShadowSignalNoRayQueryPass::ParameterMetadata& DirectShadowSignalNoRayQueryPass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<DirectShadowSignalNoRayQueryPass>();
}

const RenderPassDefinition& DirectShadowSignalNoRayQueryPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    RendererShaderPackages::DirectShadowSignalNoRayQuery,
	    L"DirectShadowSignalNoRayQuery_BindingLayout",
	    L"DirectShadowSignalNoRayQuery_PipelineState");
	return definition;
}

DirectShadowSignalPass::DirectShadowSignalPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const DirectShadowSignalPass::ParameterMetadata& DirectShadowSignalPass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<DirectShadowSignalPass>();
}

const RenderPassDefinition& DirectShadowSignalPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    RendererShaderPackages::DirectShadowSignal,
	    L"DirectShadowSignal_BindingLayout",
	    L"DirectShadowSignal_PipelineState",
	    DirectShadowSignalPassDetails::DescriptorRayQueryFeatures);
	return definition;
}

DirectShadowSignalDeviceAddressPass::DirectShadowSignalDeviceAddressPass(const ComputePassPipelineRuntime& runtime) noexcept :
    m_runtime(runtime)
{
}

const DirectShadowSignalDeviceAddressPass::ParameterMetadata& DirectShadowSignalDeviceAddressPass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<DirectShadowSignalDeviceAddressPass>();
}

const RenderPassDefinition& DirectShadowSignalDeviceAddressPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    RendererShaderPackages::DirectShadowSignalDeviceAddress,
	    L"DirectShadowSignalDeviceAddress_BindingLayout",
	    L"DirectShadowSignalDeviceAddress_PipelineState",
	    DirectShadowSignalPassDetails::DeviceAddressRayQueryFeatures);
	return definition;
}

void DirectShadowSignalNoRayQueryPass::DeclareResources(
    FrameGraphBuilder& builder,
    const GBufferRenderTargets& gbuffer,
    const DirectShadowSignalResources& shadowSignals,
    ParameterInstance& parameters)
{
	DirectShadowSignalPassDetails::PopulateNoRayResources(builder, gbuffer, shadowSignals, parameters);
}

void DirectShadowSignalPass::DeclareResources(
    FrameGraphBuilder& builder,
    const GBufferRenderTargets& gbuffer,
    FrameGraphAccelerationStructureHandle sceneTlas,
    const DirectShadowSignalResources& shadowSignals,
    ParameterInstance& parameters)
{
	DirectShadowSignalPassDetails::PopulateRayResources(builder, gbuffer, shadowSignals, parameters);
	parameters->SceneTlas = builder.Read(sceneTlas);
}

void DirectShadowSignalDeviceAddressPass::DeclareResources(
    FrameGraphBuilder& builder,
    const GBufferRenderTargets& gbuffer,
    const DirectShadowSignalResources& shadowSignals,
    ParameterInstance& parameters)
{
	DirectShadowSignalPassDetails::PopulateRayResources(builder, gbuffer, shadowSignals, parameters);
}

void DirectShadowSignalNoRayQueryPass::SetParameters(
    ParameterInstance& parameters,
    const FrameContext& frame,
    const RenderViewData& viewData,
    const PassRuntimeServices& passRuntimeServices) const
{
	parameters->PerFrame = passRuntimeServices.PerFrame;
	parameters->PerView = viewData.perViewData;
	LightingPassBinding::SetParameters(parameters, frame);
}

void DirectShadowSignalPass::SetParameters(
    ParameterInstance& parameters,
    const FrameContext& frame,
    const RenderViewData& viewData,
    const PassRuntimeServices& passRuntimeServices,
    bool hasSceneTlas) const
{
	DirectShadowSignalPassDetails::PopulateRayQueryParameters(parameters, frame, viewData, passRuntimeServices, hasSceneTlas);
}

void DirectShadowSignalDeviceAddressPass::SetParameters(
    ParameterInstance& parameters,
    const FrameContext& frame,
    const RenderViewData& viewData,
    const PassRuntimeServices& passRuntimeServices,
    bool hasSceneTlas) const
{
	DirectShadowSignalPassDetails::PopulateRayQueryParameters(parameters, frame, viewData, passRuntimeServices, hasSceneTlas);
}

void DirectShadowSignalNoRayQueryPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	SetParameters(parameters, context.Frame, context.Frame.mainView, context.RuntimeServices);
	ComputePassUtilities::DispatchSized<DirectShadowSignalNoRayQueryPass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
}

void DirectShadowSignalPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	SetParameters(parameters, context.Frame, context.Frame.mainView, context.RuntimeServices, context.Frame.rayTracingScene.HasTraceableInstances());
	{
		ComputePassUtilities::DispatchSized<DirectShadowSignalPass>(
		    context,
		    m_runtime,
		    parameters,
		    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
		    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
	}
}

void DirectShadowSignalDeviceAddressPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	SetParameters(parameters, context.Frame, context.Frame.mainView, context.RuntimeServices, context.Frame.rayTracingScene.HasTraceableInstances());
	{
		ComputePassUtilities::DispatchSized<DirectShadowSignalDeviceAddressPass>(
		    context,
		    m_runtime,
		    parameters,
		    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
		    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
	}
}

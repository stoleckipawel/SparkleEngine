#include "../../PCH.h"
#include "Passes/Deferred/DirectLightingPass.h"

#include "Core/Public/Diagnostics/Trace.h"
#include "Core/Public/Math/MathUtils.h"
#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Diagnostics/PassExecutionDiagnostics.h"
#include "Passes/Bindings/LightingPassBinding.h"
#include "Passes/Bindings/MaterialTextureTablePassBinding.h"
#include "Passes/Bindings/RayTracingHitDataPassBinding.h"
#include "Passes/Core/PassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Passes/Core/ShaderPass.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassData.h"
#include "RayTracing/RayTracingPassCapabilityQuery.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"

#include <cassert>

namespace DirectLightingPassDetails
{
	ComputeDispatchDesc BuildDispatchDesc(const RenderViewData& viewData) noexcept
	{
		return ComputeDispatchDesc{
		    MathUtils::DivideRoundUp(static_cast<std::uint32_t>(viewData.viewport.Width), DirectLightingPass::ThreadGroupSizeX),
		    MathUtils::DivideRoundUp(static_cast<std::uint32_t>(viewData.viewport.Height), DirectLightingPass::ThreadGroupSizeY),
		    1};
	}

	template <typename TParameterInstance>
	void PopulateCommonResources(
	    FrameGraphBuilder& builder,
	    const LightingRenderTargets& lighting,
	    const GBufferRenderTargets& gbuffer,
	    FrameGraphTextureHandle shadowVisibilitySignal,
	    TParameterInstance& parameters)
	{
		parameters->DirectDiffuse = builder.CreateUAV(lighting.DirectDiffuse);
		parameters->DirectSpecular = builder.CreateUAV(lighting.DirectSpecular);
		parameters->DirectSubsurface = builder.CreateUAV(lighting.DirectSubsurface);
		parameters->ShadowVisibilitySignal = builder.CreateUAV(shadowVisibilitySignal);
		parameters->GBufferBaseColor = builder.CreateSRV(gbuffer.BaseColor);
		parameters->GBufferNormal = builder.CreateSRV(gbuffer.Normal);
		parameters->GBufferMaterial = builder.CreateSRV(gbuffer.Material);
		parameters->GBufferSubsurface = builder.CreateSRV(gbuffer.Subsurface);
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
		RayTracingHitDataPassBinding::SetTriangleMaterialParameters(parameters, frame);
		parameters->MaterialTextureSampler =
		    RhiSamplerDesc{
		        .MinMagFilter = RhiSamplerMinMagFilter::Linear,
		        .MipFilter = RhiSamplerMipFilter::Linear,
		        .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Wrap),
		        .MaxAnisotropy = RhiSamplerAnisotropy::X1};

		const RayTracingPassCapabilities rayTracingCapabilities =
		    RayTracingPassCapabilityQuery::Build(frame, passRuntimeServices.RayTracing);
		const bool materialTextureTableAvailable = MaterialTextureTablePassBinding::Bind(parameters, frame);
		const bool hasAlphaTestResources = rayTracingCapabilities.TriangleMaterialDataAvailable && materialTextureTableAvailable;
		parameters->RayTracedShadows = RayTracedShadowPassData::Build(
		    passRuntimeServices.RayTracing,
		    hasSceneTlas,
		    hasAlphaTestResources,
		    frame.rayTracingHitData.GetInstanceCount(),
		    frame.rayTracingHitData.GetMaterialCount());
	}
}  // namespace DirectLightingPassDetails

DirectLightingPass::DirectLightingPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

DirectLightingNoRayQueryPass::DirectLightingNoRayQueryPass(const ComputePassPipelineRuntime& runtime) noexcept :
    m_runtime(runtime)
{
}

const DirectLightingNoRayQueryPass::ParameterMetadata& DirectLightingNoRayQueryPass::GetParameterMetadata() noexcept
{
	static const ParameterMetadata metadata = []
	{
		const ParameterMetadata localMetadata = ShaderParameterStructBuilder<Parameters>::BuildMetadata(PassName);
		const bool valid = ValidateShaderPassLayout(localMetadata.GetLayout(), ShaderPassKind::Compute, PassName);
		assert(valid);
		return localMetadata;
	}();

	return metadata;
}

const RenderPassDefinition& DirectLightingNoRayQueryPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition{
	    .PassName = PassName,
	    .PackageDeclarationName = "DirectLightingNoRayQueryShaderPackage",
	    .ShaderPackage =
	        ShaderPackageDefinition{
	            .PackageId = RendererShaderPackages::DirectLightingNoRayQuery.data(),
	            .BindingLayoutId = RendererShaderPackages::DirectLightingNoRayQuery.data(),
	            .ExpectedStages = ShaderStageMask::Compute,
	            .RequiredFeatures = CookedShaderPackageFeatureFlags::None},
	    .PipelineKind = RenderPassDefinitionPipelineKind::Compute,
	    .BindingLayoutDebugName = L"DirectLightingNoRayQuery_BindingLayout",
	    .PipelineStateDebugName = L"DirectLightingNoRayQuery_PipelineState"};
	return definition;
}

const DirectLightingPass::ParameterMetadata& DirectLightingPass::GetParameterMetadata() noexcept
{
	static const ParameterMetadata metadata = []
	{
		const ParameterMetadata localMetadata = ShaderParameterStructBuilder<Parameters>::BuildMetadata(PassName);
		const bool valid = ValidateShaderPassLayout(localMetadata.GetLayout(), ShaderPassKind::Compute, PassName);
		assert(valid);
		return localMetadata;
	}();

	return metadata;
}

const RenderPassDefinition& DirectLightingPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition{
	    .PassName = PassName,
	    .PackageDeclarationName = "DirectLightingShaderPackage",
	    .ShaderPackage =
	        ShaderPackageDefinition{
	            .PackageId = RendererShaderPackages::DirectLighting.data(),
	            .BindingLayoutId = RendererShaderPackages::DirectLighting.data(),
	            .ExpectedStages = ShaderStageMask::Compute,
	            .RequiredFeatures =
	                CookedShaderPackageFeatureFlags::UsesInlineRayQuery |
	                CookedShaderPackageFeatureFlags::UsesAccelerationStructure |
	                CookedShaderPackageFeatureFlags::UsesDescriptorIndexing},
	    .PipelineKind = RenderPassDefinitionPipelineKind::Compute,
	    .BindingLayoutDebugName = L"DirectLighting_BindingLayout",
	    .PipelineStateDebugName = L"DirectLighting_PipelineState"};
	return definition;
}

DirectLightingDeviceAddressPass::DirectLightingDeviceAddressPass(const ComputePassPipelineRuntime& runtime) noexcept :
    m_runtime(runtime)
{
}

const DirectLightingDeviceAddressPass::ParameterMetadata& DirectLightingDeviceAddressPass::GetParameterMetadata() noexcept
{
	static const ParameterMetadata metadata = []
	{
		const ParameterMetadata localMetadata = ShaderParameterStructBuilder<Parameters>::BuildMetadata(PassName);
		const bool valid = ValidateShaderPassLayout(localMetadata.GetLayout(), ShaderPassKind::Compute, PassName);
		assert(valid);
		return localMetadata;
	}();

	return metadata;
}

const RenderPassDefinition& DirectLightingDeviceAddressPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition{
	    .PassName = PassName,
	    .PackageDeclarationName = "DirectLightingDeviceAddressShaderPackage",
	    .ShaderPackage =
	        ShaderPackageDefinition{
	            .PackageId = RendererShaderPackages::DirectLightingDeviceAddress.data(),
	            .BindingLayoutId = RendererShaderPackages::DirectLightingDeviceAddress.data(),
	            .ExpectedStages = ShaderStageMask::Compute,
	            .RequiredFeatures =
	                CookedShaderPackageFeatureFlags::UsesInlineRayQuery |
	                CookedShaderPackageFeatureFlags::UsesAccelerationStructure |
	                CookedShaderPackageFeatureFlags::UsesAccelerationStructureDeviceAddress |
	                CookedShaderPackageFeatureFlags::UsesDescriptorIndexing},
	    .PipelineKind = RenderPassDefinitionPipelineKind::Compute,
	    .BindingLayoutDebugName = L"DirectLightingDeviceAddress_BindingLayout",
	    .PipelineStateDebugName = L"DirectLightingDeviceAddress_PipelineState"};
	return definition;
}

void DirectLightingNoRayQueryPass::DeclareResources(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer,
    FrameGraphTextureHandle shadowVisibilitySignal,
    ParameterInstance& parameters)
{
	DirectLightingPassDetails::PopulateCommonResources(builder, lighting, gbuffer, shadowVisibilitySignal, parameters);
}

void DirectLightingPass::DeclareResources(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer,
    FrameGraphAccelerationStructureHandle sceneTlas,
    FrameGraphTextureHandle shadowVisibilitySignal,
    ParameterInstance& parameters)
{
	DirectLightingPassDetails::PopulateCommonResources(builder, lighting, gbuffer, shadowVisibilitySignal, parameters);
	parameters->SceneTlas = builder.Read(sceneTlas);
}

void DirectLightingNoRayQueryPass::SetParameters(
    ParameterInstance& parameters,
    const FrameContext& frame,
    const RenderViewData& viewData,
    const PassRuntimeServices& passRuntimeServices) const
{
	parameters->PerFrame = passRuntimeServices.PerFrame;
	parameters->PerView = viewData.perViewData;
	LightingPassBinding::SetParameters(parameters, frame);
	const bool valid = parameters.Sync();
	assert(valid);
}

void DirectLightingDeviceAddressPass::DeclareResources(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer,
    FrameGraphTextureHandle shadowVisibilitySignal,
    ParameterInstance& parameters)
{
	DirectLightingPassDetails::PopulateCommonResources(builder, lighting, gbuffer, shadowVisibilitySignal, parameters);
}

void DirectLightingPass::SetParameters(
    ParameterInstance& parameters,
    const FrameContext& frame,
    const RenderViewData& viewData,
    const PassRuntimeServices& passRuntimeServices,
    bool hasSceneTlas) const
{
	DirectLightingPassDetails::PopulateRayQueryParameters(parameters, frame, viewData, passRuntimeServices, hasSceneTlas);
	const bool valid = parameters.Sync();
	assert(valid);
}

void DirectLightingDeviceAddressPass::SetParameters(
    ParameterInstance& parameters,
    const FrameContext& frame,
    const RenderViewData& viewData,
    const PassRuntimeServices& passRuntimeServices,
    bool hasSceneTlas) const
{
	DirectLightingPassDetails::PopulateRayQueryParameters(parameters, frame, viewData, passRuntimeServices, hasSceneTlas);
	const bool valid = parameters.Sync();
	assert(valid);
}

void DirectLightingNoRayQueryPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	SetParameters(parameters, context.Frame, context.Frame.mainView, context.RuntimeServices);
	const ComputeDispatchDesc dispatch = DirectLightingPassDetails::BuildDispatchDesc(context.Frame.mainView);
	const bool dispatched = PassUtilities::DispatchComputePassWithRuntime<DirectLightingNoRayQueryPass>(
	    context.Resources,
	    context.Commands,
	    context.RuntimeServices.HardwareInterface,
	    m_runtime,
	    parameters.GetPassParameterSet(),
	    dispatch,
	    PassName);
	assert(dispatched);
}

void DirectLightingPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	SetParameters(parameters, context.Frame, context.Frame.mainView, context.RuntimeServices, context.Frame.rayTracingScene.HasTraceableInstances());
	const ComputeDispatchDesc dispatch = DirectLightingPassDetails::BuildDispatchDesc(context.Frame.mainView);
	const bool dispatched = [&]() noexcept
	{
		SPARKLE_GPU_SCOPE(context.Diagnostics, "Ray Query Dispatch");
		return PassUtilities::DispatchComputePassWithRuntime<DirectLightingPass>(
		    context.Resources,
		    context.Commands,
		    context.RuntimeServices.HardwareInterface,
		    m_runtime,
		    parameters.GetPassParameterSet(),
		    dispatch,
		    PassName);
	}();
	assert(dispatched);
}

void DirectLightingDeviceAddressPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	SetParameters(parameters, context.Frame, context.Frame.mainView, context.RuntimeServices, context.Frame.rayTracingScene.HasTraceableInstances());
	const ComputeDispatchDesc dispatch = DirectLightingPassDetails::BuildDispatchDesc(context.Frame.mainView);
	const bool dispatched = [&]() noexcept
	{
		SPARKLE_GPU_SCOPE(context.Diagnostics, "Ray Query Dispatch");
		return PassUtilities::DispatchComputePassWithRuntime<DirectLightingDeviceAddressPass>(
		    context.Resources,
		    context.Commands,
		    context.RuntimeServices.HardwareInterface,
		    m_runtime,
		    parameters.GetPassParameterSet(),
		    dispatch,
		    PassName);
	}();
	assert(dispatched);
}

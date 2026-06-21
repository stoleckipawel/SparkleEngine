#include "../PCH.h"
#include "Passes/DirectLightingPass.h"

#include "Core/Public/Diagnostics/Trace.h"
#include "Core/Public/Math/MathUtils.h"
#include "Frame/RenderViewData.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Diagnostics/PassExecutionDiagnostics.h"
#include "Passes/PassUtilities.h"
#include "Passes/RenderPassDefinition.h"
#include "Passes/ShaderPass.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "RayTracing/RayTracedShadowPassData.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

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
	    TParameterInstance& parameters)
	{
		parameters->DirectDiffuse = builder.CreateUAV(lighting.DirectDiffuse);
		parameters->DirectSpecular = builder.CreateUAV(lighting.DirectSpecular);
		parameters->DirectSubsurface = builder.CreateUAV(lighting.DirectSubsurface);
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
	                CookedShaderPackageFeatureFlags::UsesInlineRayQuery | CookedShaderPackageFeatureFlags::UsesAccelerationStructure},
	    .PipelineKind = RenderPassDefinitionPipelineKind::Compute,
	    .BindingLayoutDebugName = L"DirectLighting_BindingLayout",
	    .PipelineStateDebugName = L"DirectLighting_PipelineState"};
	return definition;
}

DirectLightingVulkanAddressPass::DirectLightingVulkanAddressPass(const ComputePassPipelineRuntime& runtime) noexcept :
    m_runtime(runtime)
{
}

const DirectLightingVulkanAddressPass::ParameterMetadata& DirectLightingVulkanAddressPass::GetParameterMetadata() noexcept
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

const RenderPassDefinition& DirectLightingVulkanAddressPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition{
	    .PassName = PassName,
	    .PackageDeclarationName = "DirectLightingVulkanAddressShaderPackage",
	    .ShaderPackage =
	        ShaderPackageDefinition{
	            .PackageId = RendererShaderPackages::DirectLightingVulkanAddress.data(),
	            .BindingLayoutId = RendererShaderPackages::DirectLightingVulkanAddress.data(),
	            .ExpectedStages = ShaderStageMask::Compute,
	            .RequiredFeatures =
	                CookedShaderPackageFeatureFlags::UsesInlineRayQuery |
	                CookedShaderPackageFeatureFlags::UsesAccelerationStructure |
	                CookedShaderPackageFeatureFlags::UsesAccelerationStructureDeviceAddress},
	    .PipelineKind = RenderPassDefinitionPipelineKind::Compute,
	    .BindingLayoutDebugName = L"DirectLightingVulkanAddress_BindingLayout",
	    .PipelineStateDebugName = L"DirectLightingVulkanAddress_PipelineState"};
	return definition;
}

void DirectLightingPass::DeclareResources(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer,
    FrameGraphAccelerationStructureHandle sceneTlas,
    ParameterInstance& parameters)
{
	DirectLightingPassDetails::PopulateCommonResources(builder, lighting, gbuffer, parameters);
	parameters->SceneTlas = builder.Read(sceneTlas);
}

void DirectLightingVulkanAddressPass::DeclareResources(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer,
    ParameterInstance& parameters)
{
	DirectLightingPassDetails::PopulateCommonResources(builder, lighting, gbuffer, parameters);
}

void DirectLightingPass::SetParameters(
    ParameterInstance& parameters,
    const RenderViewData& viewData,
    const PassRuntimeServices& passRuntimeServices,
    bool hasSceneTlas) const
{
	parameters->PerFrame = passRuntimeServices.HardwareInterface.GetUploadService().GetPerFrameConstantData();
	parameters->PerView = viewData.perViewData;
	parameters->RayTracedShadows = RayTracedShadowPassData::Build(passRuntimeServices.RayTracing, hasSceneTlas);
	const bool valid = parameters.Sync();
	assert(valid);
}

void DirectLightingVulkanAddressPass::SetParameters(
    ParameterInstance& parameters,
    const RenderViewData& viewData,
    const PassRuntimeServices& passRuntimeServices,
    bool hasSceneTlas) const
{
	parameters->PerFrame = passRuntimeServices.HardwareInterface.GetUploadService().GetPerFrameConstantData();
	parameters->PerView = viewData.perViewData;
	parameters->RayTracedShadows = RayTracedShadowPassData::Build(passRuntimeServices.RayTracing, hasSceneTlas);
	const bool valid = parameters.Sync();
	assert(valid);
}

void DirectLightingPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	SetParameters(parameters, context.Frame.mainView, context.RuntimeServices, context.Frame.rayTracingScene.HasTraceableInstances());
	const ComputeDispatchDesc dispatch = DirectLightingPassDetails::BuildDispatchDesc(context.Frame.mainView);
	const bool dispatched = [&]() noexcept
	{
		SPARKLE_GPU_SCOPE(context.Diagnostics, "Ray Query Dispatch");
		return PassUtilities::DispatchComputePassWithRuntime<DirectLightingPass>(
		    context.Resources,
		    context.Commands,
		    context.RuntimeServices.HardwareInterface,
		    m_runtime,
		    parameters,
		    dispatch,
		    PassName);
	}();
	assert(dispatched);
}

void DirectLightingVulkanAddressPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	SetParameters(parameters, context.Frame.mainView, context.RuntimeServices, context.Frame.rayTracingScene.HasTraceableInstances());
	const ComputeDispatchDesc dispatch = DirectLightingPassDetails::BuildDispatchDesc(context.Frame.mainView);
	const bool dispatched = [&]() noexcept
	{
		SPARKLE_GPU_SCOPE(context.Diagnostics, "Ray Query Dispatch");
		return PassUtilities::DispatchComputePassWithRuntime<DirectLightingVulkanAddressPass>(
		    context.Resources,
		    context.Commands,
		    context.RuntimeServices.HardwareInterface,
		    m_runtime,
		    parameters,
		    dispatch,
		    PassName);
	}();
	assert(dispatched);
}

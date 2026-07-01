#include "../../PCH.h"
#include "Passes/Reference/ReferencePathTracingPass.h"

#include "Core/Public/Math/MathUtils.h"
#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Passes/Bindings/LightingPassBinding.h"
#include "Passes/Bindings/MaterialTextureTablePassBinding.h"
#include "Passes/Bindings/RayTracingHitDataPassBinding.h"
#include "Passes/Bindings/RayTracingScenePassBinding.h"
#include "Passes/Core/PassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Passes/Core/ShaderPass.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "RayTracing/Effects/ReferencePathTracing/ReferencePathTracingSettings.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassData.h"
#include "RayTracing/RayTracingPassCapabilityQuery.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"

#include <cassert>

ReferencePathTracingPass::ReferencePathTracingPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const ReferencePathTracingPass::ParameterMetadata& ReferencePathTracingPass::GetParameterMetadata() noexcept
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

const RenderPassDefinition& ReferencePathTracingPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition{
	    .PassName = PassName,
	    .PackageDeclarationName = "ReferencePathTracingShaderPackage",
	    .ShaderPackage =
	        ShaderPackageDefinition{
	            .PackageId = RendererShaderPackages::ReferencePathTracing.data(),
	            .BindingLayoutId = RendererShaderPackages::ReferencePathTracing.data(),
	            .ExpectedStages = ShaderStageMask::Compute,
	            .RequiredFeatures =
	                CookedShaderPackageFeatureFlags::UsesInlineRayQuery |
	                CookedShaderPackageFeatureFlags::UsesAccelerationStructure |
	                CookedShaderPackageFeatureFlags::UsesDescriptorIndexing},
	    .PipelineKind = RenderPassDefinitionPipelineKind::Compute,
	    .BindingLayoutDebugName = L"ReferencePathTracing_BindingLayout",
	    .PipelineStateDebugName = L"ReferencePathTracing_PipelineState"};
	return definition;
}

void ReferencePathTracingPass::DeclareResources(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    FrameGraphAccelerationStructureHandle sceneTlas,
    ParameterInstance& parameters)
{
	parameters->ReferenceSceneColorTexture = builder.CreateUAV(lighting.ReferenceSceneColor);
	parameters->ReferenceDirectTexture = builder.CreateUAV(lighting.ReferenceDirect);
	parameters->ReferenceIndirectDiffuseTexture = builder.CreateUAV(lighting.ReferenceIndirectDiffuse);
	parameters->ReferenceIndirectSpecularTexture = builder.CreateUAV(lighting.ReferenceIndirectSpecular);
	(void)RayTracingScenePassBinding::BindSceneTlas(
	    builder,
	    sceneTlas,
	    RayTracingSceneTlasShaderAccessMode::Descriptor,
	    parameters);
}

void ReferencePathTracingPass::SetParameters(
    ParameterInstance& parameters,
    const FrameContext& frame,
    const RenderViewData& viewData,
    const PassRuntimeServices& passRuntimeServices,
    RhiDescriptorTableBinding environmentTextureBinding) const
{
	parameters->PerFrame = passRuntimeServices.PerFrame;
	parameters->PerView = viewData.perViewData;
	LightingPassBinding::SetParameters(parameters, frame);
	RayTracingHitDataPassBinding::SetParameters(parameters, frame);
	parameters->SkyTexture = environmentTextureBinding;
	parameters->SamplerLinearClamp = RhiSamplerDesc{
	    .MinMagFilter = RhiSamplerMinMagFilter::Linear,
	    .MipFilter = RhiSamplerMipFilter::Linear,
	    .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Clamp)};
	parameters->MaterialTextureSampler =
	    RhiSamplerDesc{
	        .MinMagFilter = RhiSamplerMinMagFilter::Linear,
	        .MipFilter = RhiSamplerMipFilter::Linear,
	        .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Wrap),
	        .MaxAnisotropy = RhiSamplerAnisotropy::X1};
}

void ReferencePathTracingPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	const ReferencePathTracingSettings settings = BuildReferencePathTracingSettingsFromCVars();
	if (!settings.Enabled)
	{
		return;
	}

	const RayTracingPassCapabilities rayTracingCapabilities =
	    RayTracingPassCapabilityQuery::Build(context.Frame, context.RuntimeServices.RayTracing);
	if (!rayTracingCapabilities.InlineRayQueryAvailable ||
	    !RayTracingScenePassBinding::CanUseSceneTlas(rayTracingCapabilities, RayTracingSceneTlasShaderAccessMode::Descriptor))
	{
		return;
	}
	if (!rayTracingCapabilities.HitDataAvailable || !rayTracingCapabilities.MaterialTextureTableAvailable)
	{
		return;
	}

	const RhiDescriptorTableBinding environmentTextureBinding =
	    m_environmentMapBinding.GetTextureBinding(context.RuntimeServices);
	if (!environmentTextureBinding)
	{
		return;
	}

	const bool materialTextureTableAvailable = MaterialTextureTablePassBinding::Bind(parameters, context.Frame);
	if (!materialTextureTableAvailable)
	{
		return;
	}

	SetParameters(parameters, context.Frame, context.Frame.mainView, context.RuntimeServices, environmentTextureBinding);
	parameters->ReferencePathTracingConstants = ReferencePathTracingUniformData{
	    .SamplesPerPixel = settings.SamplesPerPixel,
	    .BounceCount = settings.BounceCount,
	    .NormalBias = settings.NormalBias,
	    .MaxDistance = settings.MaxDistance};
	parameters->RayTracedShadows = RayTracedShadowPassData::Build(
	    context.RuntimeServices.RayTracing,
	    context.Frame.rayTracingScene.HasTraceableInstances(),
	    rayTracingCapabilities.TriangleMaterialDataAvailable && materialTextureTableAvailable,
	    context.Frame.rayTracingHitData.GetInstanceCount(),
	    context.Frame.rayTracingHitData.GetMaterialCount());
	const bool valid = parameters.Sync();
	assert(valid);

	const ComputeDispatchDesc dispatch{
	    MathUtils::DivideRoundUp(static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width), ThreadGroupSizeX),
	    MathUtils::DivideRoundUp(static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height), ThreadGroupSizeY),
	    1};
	const bool dispatched = PassUtilities::DispatchComputePassWithRuntime<ReferencePathTracingPass>(
	    context.Resources,
	    context.Commands,
	    context.RuntimeServices.HardwareInterface,
	    m_runtime,
	    parameters.GetPassParameterSet(),
	    dispatch,
	    PassName);
	assert(dispatched);
}

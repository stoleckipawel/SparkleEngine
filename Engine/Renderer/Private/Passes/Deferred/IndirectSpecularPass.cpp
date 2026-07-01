#include "../../PCH.h"
#include "Passes/Deferred/IndirectSpecularPass.h"

#include "Core/Public/Diagnostics/Trace.h"
#include "Core/Public/Math/MathUtils.h"
#include "Diagnostics/PassExecutionDiagnostics.h"
#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Passes/Core/PassUtilities.h"
#include "Passes/Bindings/LightingPassBinding.h"
#include "Passes/Bindings/MaterialTextureTablePassBinding.h"
#include "Passes/Bindings/RayTracingHitDataPassBinding.h"
#include "Passes/Bindings/RayTracingScenePassBinding.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Passes/Core/ShaderPass.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "RayTracing/RayTracingPassCapabilityQuery.h"
#include "RayTracing/Effects/IndirectSpecular/IndirectSpecularPassData.h"
#include "RayTracing/Scene/RenderRayTracingPassServices.h"
#include "RayTracing/Effects/IndirectSpecular/IndirectSpecularSettings.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"

#include <cassert>

namespace
{
	constexpr const char* DispatchTimingLabel = "Indirect Specular Ray Query";

	IndirectSpecularSettings ResolveSettings(const PassRuntimeServices& services) noexcept
	{
		const RenderRayTracingPassServices* rayTracingServices = services.RayTracing;
		if (rayTracingServices != nullptr && rayTracingServices->IndirectSpecularSettings != nullptr)
		{
			return *rayTracingServices->IndirectSpecularSettings;
		}

		return BuildIndirectSpecularSettingsFromCVars();
	}

}

IndirectSpecularPass::IndirectSpecularPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const IndirectSpecularPass::ParameterMetadata& IndirectSpecularPass::GetParameterMetadata() noexcept
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

const RenderPassDefinition& IndirectSpecularPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition{
	    .PassName = PassName,
	    .PackageDeclarationName = "IndirectSpecularShaderPackage",
	    .ShaderPackage =
	        ShaderPackageDefinition{
	            .PackageId = RendererShaderPackages::IndirectSpecular.data(),
	            .BindingLayoutId = RendererShaderPackages::IndirectSpecular.data(),
	            .ExpectedStages = ShaderStageMask::Compute,
	            .RequiredFeatures =
	                CookedShaderPackageFeatureFlags::UsesInlineRayQuery |
	                CookedShaderPackageFeatureFlags::UsesAccelerationStructure |
	                CookedShaderPackageFeatureFlags::UsesDescriptorIndexing},
	    .PipelineKind = RenderPassDefinitionPipelineKind::Compute,
	    .BindingLayoutDebugName = L"IndirectSpecular_BindingLayout",
	    .PipelineStateDebugName = L"IndirectSpecular_PipelineState"};
	return definition;
}

void IndirectSpecularPass::DeclareResources(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer,
    FrameGraphAccelerationStructureHandle sceneTlas,
    ParameterInstance& parameters)
{
	parameters->IndirectSpecular = builder.CreateUAV(lighting.IndirectSpecular);
	parameters->IndirectSpecularDemodulatedRadiance = builder.CreateUAV(lighting.IndirectSpecularDemodulatedRadiance);
	parameters->IndirectSpecularSampleGuide = builder.CreateUAV(lighting.IndirectSpecularSampleGuide);
	(void)RayTracingScenePassBinding::BindSceneTlas(
	    builder,
	    sceneTlas,
	    RayTracingSceneTlasShaderAccessMode::Descriptor,
	    parameters);
	parameters->GBufferBaseColor = builder.CreateSRV(gbuffer.BaseColor);
	parameters->GBufferNormal = builder.CreateSRV(gbuffer.Normal);
	parameters->GBufferMaterial = builder.CreateSRV(gbuffer.Material);
	parameters->GBufferDeviceZ = builder.CreateSRV(gbuffer.DeviceZ);
}

void IndirectSpecularPass::SetParameters(
    ParameterInstance& parameters,
    const FrameContext& frame,
    const RenderViewData& viewData,
    const PassRuntimeServices& passRuntimeServices) const
{
	parameters->PerFrame = passRuntimeServices.PerFrame;
	parameters->PerView = viewData.perViewData;
	LightingPassBinding::SetParameters(parameters, frame);
	RayTracingHitDataPassBinding::SetParameters(parameters, frame);
	parameters->SkyTexture = m_environmentMapBinding.GetTextureBinding(passRuntimeServices);
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

void IndirectSpecularPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	const IndirectSpecularSettings settings = ResolveSettings(context.RuntimeServices);
	const RayTracingPassCapabilities rayTracingCapabilities =
	    RayTracingPassCapabilityQuery::Build(context.Frame, context.RuntimeServices.RayTracing);
	const std::uint32_t hitInstanceCount = context.Frame.rayTracingHitData.GetInstanceCount();
	const std::uint32_t hitMaterialCount = context.Frame.rayTracingHitData.GetMaterialCount();
	if (!settings.Enabled)
	{
		return;
	}

	if (!rayTracingCapabilities.InlineRayQueryAvailable ||
	    !RayTracingScenePassBinding::CanUseSceneTlas(rayTracingCapabilities, RayTracingSceneTlasShaderAccessMode::Descriptor))
	{
		return;
	}
	if (!rayTracingCapabilities.HitDataAvailable || !rayTracingCapabilities.MaterialTextureTableAvailable)
	{
		return;
	}

	const bool materialTextureTableAvailable = MaterialTextureTablePassBinding::Bind(parameters, context.Frame);
	if (!materialTextureTableAvailable)
	{
		return;
	}

	SetParameters(parameters, context.Frame, context.Frame.mainView, context.RuntimeServices);
	parameters->IndirectSpecularConstants = IndirectSpecularPassData::Build(
	    settings,
	    rayTracingCapabilities,
	    hitInstanceCount,
	    hitMaterialCount,
	    MaterialTextureTableFixedCapacity);
	const bool valid = parameters.Sync();
	assert(valid);

	const ComputeDispatchDesc dispatch{
	    MathUtils::DivideRoundUp(static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width), ThreadGroupSizeX),
	    MathUtils::DivideRoundUp(static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height), ThreadGroupSizeY),
	    1};
	const bool dispatched = [&]() noexcept
	{
		SPARKLE_GPU_SCOPE(context.Diagnostics, DispatchTimingLabel);
		return PassUtilities::DispatchComputePassWithRuntime<IndirectSpecularPass>(
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

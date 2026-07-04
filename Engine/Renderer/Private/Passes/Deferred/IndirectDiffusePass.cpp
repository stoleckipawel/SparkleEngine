#include "../../PCH.h"
#include "Passes/Deferred/IndirectDiffusePass.h"

#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Passes/Bindings/LightingPassBinding.h"
#include "Passes/Bindings/MaterialTextureTablePassBinding.h"
#include "Passes/Bindings/RayTracingHitDataPassBinding.h"
#include "Passes/Bindings/RayTracingScenePassBinding.h"
#include "Passes/Core/ComputePassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "RayTracing/Effects/IndirectDiffuse/IndirectDiffusePassData.h"
#include "RayTracing/Effects/IndirectDiffuse/IndirectDiffuseSettings.h"
#include "RayTracing/RayTracingPassCapabilityQuery.h"
#include "RayTracing/Scene/RenderRayTracingPassServices.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"

namespace
{
	constexpr CookedShaderPackageFeatureFlags RayQueryFeatures =
	    CookedShaderPackageFeatureFlags::UsesInlineRayQuery |
	    CookedShaderPackageFeatureFlags::UsesAccelerationStructure |
	    CookedShaderPackageFeatureFlags::UsesDescriptorIndexing;

	IndirectDiffuseSettings ResolveSettings(const PassRuntimeServices& services) noexcept
	{
		const RenderRayTracingPassServices* rayTracingServices = services.RayTracing;
		if (rayTracingServices != nullptr && rayTracingServices->IndirectDiffuseSettings != nullptr)
		{
			return *rayTracingServices->IndirectDiffuseSettings;
		}

		return BuildIndirectDiffuseSettingsFromCVars();
	}
}

IndirectDiffusePass::IndirectDiffusePass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const IndirectDiffusePass::ParameterMetadata& IndirectDiffusePass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<IndirectDiffusePass>();
}

const RenderPassDefinition& IndirectDiffusePass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    RendererShaderPackages::IndirectDiffuse,
	    L"IndirectDiffuse_BindingLayout",
	    L"IndirectDiffuse_PipelineState",
	    RayQueryFeatures);
	return definition;
}

void IndirectDiffusePass::DeclareResources(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer,
    FrameGraphAccelerationStructureHandle sceneTlas,
    ParameterInstance& parameters)
{
	parameters->IndirectDiffuseTexture = builder.CreateUAV(lighting.IndirectDiffuse);
	parameters->IndirectDiffuseDemodulatedRadiance = builder.CreateUAV(lighting.IndirectDiffuseDemodulatedRadiance);
	parameters->IndirectDiffuseAlbedo = builder.CreateUAV(lighting.IndirectDiffuseAlbedo);
	parameters->IndirectSpecularAlbedo = builder.CreateUAV(lighting.IndirectSpecularAlbedo);
	parameters->IndirectMaterialGuide = builder.CreateUAV(lighting.IndirectMaterialGuide);
	parameters->IndirectDiffuseSampleGuide = builder.CreateUAV(lighting.IndirectDiffuseSampleGuide);
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

void IndirectDiffusePass::SetParameters(
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

void IndirectDiffusePass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	const IndirectDiffuseSettings settings = ResolveSettings(context.RuntimeServices);
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
	parameters->IndirectDiffuseConstants = IndirectDiffusePassData::Build(
	    settings,
	    rayTracingCapabilities,
	    hitInstanceCount,
	    hitMaterialCount,
	    MaterialTextureTableFixedCapacity);
	ComputePassUtilities::DispatchSized<IndirectDiffusePass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
}

#include "../../PCH.h"
#include "Passes/Deferred/IndirectSpecularPass.h"

#include "Diagnostics/PassExecutionDiagnostics.h"
#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Passes/Core/ComputePassUtilities.h"
#include "Passes/Bindings/LightingPassBinding.h"
#include "Passes/Bindings/MaterialTextureTablePassBinding.h"
#include "Passes/Bindings/RayTracingHitDataPassBinding.h"
#include "Passes/Bindings/RayTracingScenePassBinding.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "RayTracing/RayTracingPassCapabilityQuery.h"
#include "RayTracing/Effects/IndirectSpecular/IndirectSpecularCVars.h"
#include "RayTracing/Effects/IndirectSpecular/IndirectSpecularPassData.h"
#include "RayTracing/Effects/IndirectSpecular/IndirectSpecularSettings.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"

namespace
{
	constexpr const char* DispatchTimingLabel = "Indirect Specular Ray Query";
	constexpr CookedShaderPackageFeatureFlags RayQueryFeatures =
	    CookedShaderPackageFeatureFlags::UsesInlineRayQuery |
	    CookedShaderPackageFeatureFlags::UsesAccelerationStructure |
	    CookedShaderPackageFeatureFlags::UsesDescriptorIndexing;
}

IndirectSpecularPass::IndirectSpecularPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const IndirectSpecularPass::ParameterMetadata& IndirectSpecularPass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<IndirectSpecularPass>();
}

const RenderPassDefinition& IndirectSpecularPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    RendererShaderPackages::IndirectSpecular,
	    L"IndirectSpecular_BindingLayout",
	    L"IndirectSpecular_PipelineState",
	    RayQueryFeatures);
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
	if (!CVarIndirectSpecularEnabled.Get())
	{
		return;
	}

	const IndirectSpecularSettings settings = BuildIndirectSpecularSettings();
	const RayTracingPassCapabilities rayTracingCapabilities =
	    RayTracingPassCapabilityQuery::Build(context.Frame, context.RuntimeServices.RayTracing);
	const std::uint32_t hitInstanceCount = context.Frame.rayTracingHitData.GetInstanceCount();
	const std::uint32_t hitMaterialCount = context.Frame.rayTracingHitData.GetMaterialCount();

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
	{
		ComputePassUtilities::DispatchSized<IndirectSpecularPass>(
		    context,
		    m_runtime,
		    parameters,
		    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
		    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
	}
}

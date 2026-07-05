#include "../../PCH.h"
#include "Passes/Reference/ReferencePathTracingPass.h"

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
#include "RayTracing/Effects/ReferencePathTracing/ReferencePathTracingSettings.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassData.h"
#include "RayTracing/RayTracingPassCapabilityQuery.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"

namespace
{
	constexpr CookedShaderPackageFeatureFlags RayQueryFeatures =
	    CookedShaderPackageFeatureFlags::UsesInlineRayQuery |
	    CookedShaderPackageFeatureFlags::UsesAccelerationStructure |
	    CookedShaderPackageFeatureFlags::UsesDescriptorIndexing;
}

ReferencePathTracingPass::ReferencePathTracingPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const ReferencePathTracingPass::ParameterMetadata& ReferencePathTracingPass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<ReferencePathTracingPass>();
}

const RenderPassDefinition& ReferencePathTracingPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    RendererShaderPackages::ReferencePathTracing,
	    L"ReferencePathTracing_BindingLayout",
	    L"ReferencePathTracing_PipelineState",
	    RayQueryFeatures);
	return definition;
}

void ReferencePathTracingPass::DeclareResources(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle referenceSceneColor,
    FrameGraphAccelerationStructureHandle sceneTlas,
    ParameterInstance& parameters)
{
	parameters->ReferenceSceneColorTexture = builder.CreateUAV(referenceSceneColor);
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
	ComputePassUtilities::DispatchSized<ReferencePathTracingPass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
}

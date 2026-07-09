#include "../../PCH.h"
#include "Passes/RayTracing/RaytracedGBufferPass.h"

#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Passes/Bindings/MaterialTextureTablePassBinding.h"
#include "Passes/Bindings/RayTracingHitDataPassBinding.h"
#include "Passes/Bindings/RayTracingScenePassBinding.h"
#include "Passes/Core/ComputePassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
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

RaytracedGBufferPass::RaytracedGBufferPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const RaytracedGBufferPass::ParameterMetadata& RaytracedGBufferPass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<RaytracedGBufferPass>();
}

const RenderPassDefinition& RaytracedGBufferPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    RendererShaderPackages::RaytracedGBuffer,
	    L"RaytracedGBuffer_BindingLayout",
	    L"RaytracedGBuffer_PipelineState",
	    RayQueryFeatures);
	return definition;
}

void RaytracedGBufferPass::DeclareResources(
    FrameGraphBuilder& builder,
    const GBufferRenderTargets& targets,
    FrameGraphAccelerationStructureHandle sceneTlas,
    ParameterInstance& parameters)
{
	parameters->GBufferBaseColor = builder.CreateUAV(targets.BaseColor);
	parameters->GBufferNormal = builder.CreateUAV(targets.Normal);
	parameters->GBufferMaterial = builder.CreateUAV(targets.Material);
	parameters->GBufferEmissive = builder.CreateUAV(targets.Emissive);
	parameters->GBufferSubsurface = builder.CreateUAV(targets.Subsurface);
	parameters->GBufferDeviceZ = builder.CreateUAV(targets.DeviceZ);
	parameters->GBufferMotionVector = builder.CreateUAV(targets.MotionVector);
	(void)RayTracingScenePassBinding::BindSceneTlas(
	    builder,
	    sceneTlas,
	    RayTracingSceneTlasShaderAccessMode::Descriptor,
	    parameters);
}

void RaytracedGBufferPass::SetParameters(
    ParameterInstance& parameters,
    const FrameContext& frame,
    const RenderViewData& viewData,
    const PassRuntimeServices& passRuntimeServices) const
{
	parameters->PerFrame = passRuntimeServices.PerFrame;
	parameters->PerView = viewData.perViewData;
	parameters->PerTemporal = viewData.perTemporalData;
	RayTracingHitDataPassBinding::SetTemporalSurfaceParameters(parameters, frame);
	parameters->MaterialTextureSampler =
	    RhiSamplerDesc{
	        .MinMagFilter = RhiSamplerMinMagFilter::Linear,
	        .MipFilter = RhiSamplerMipFilter::Linear,
	        .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Wrap),
	        .MaxAnisotropy = RhiSamplerAnisotropy::X1};
}

void RaytracedGBufferPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
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

	const bool materialTextureTableAvailable = MaterialTextureTablePassBinding::Bind(parameters, context.Frame);
	if (!materialTextureTableAvailable)
	{
		return;
	}

	SetParameters(parameters, context.Frame, context.Frame.mainView, context.RuntimeServices);
	parameters->RaytracedGBufferConstants = RaytracedGBufferUniformData{
	    .RayTracingHitDataAvailable = rayTracingCapabilities.HitDataAvailable ? 1u : 0u,
	    .RayTracingHitInstanceCount = context.Frame.rayTracingHitData.GetInstanceCount(),
	    .RayTracingHitMaterialCount = context.Frame.rayTracingHitData.GetMaterialCount()};
	ComputePassUtilities::DispatchSized<RaytracedGBufferPass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
}

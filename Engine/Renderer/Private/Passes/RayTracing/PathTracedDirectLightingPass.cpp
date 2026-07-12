#include "../../PCH.h"
#include "Passes/RayTracing/PathTracedDirectLightingPass.h"

#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Passes/Core/ComputePassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "RayTracing/RayTracingShaderFeatureFlags.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "RayTracing/Effects/PathTracedLighting/PathTracedLightingSettings.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassData.h"
#include "RayTracing/RayTracingPassCapabilityQuery.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"
#include "SceneData/MaterialTextureTableCapability.h"

void PathTracedDirectLightingPassParameters::Describe(ShaderParameterStructBuilder<PathTracedDirectLightingPassParameters>& builder)
{
	builder.RWTexture("DirectDiffuse", &PathTracedDirectLightingPassParameters::DirectDiffuse, ShaderStageVisibility::Compute);
	builder.RWTexture("DirectSpecular", &PathTracedDirectLightingPassParameters::DirectSpecular, ShaderStageVisibility::Compute);
	builder.RWTexture("DirectSubsurface", &PathTracedDirectLightingPassParameters::DirectSubsurface, ShaderStageVisibility::Compute);
	builder.AccelerationStructure("SceneTlas", &PathTracedDirectLightingPassParameters::SceneTlas, ShaderStageVisibility::Compute);
	builder.Uniform("PerFrame", &PathTracedDirectLightingPassParameters::PerFrame, ShaderStageVisibility::Compute);
	builder.Uniform("PerView", &PathTracedDirectLightingPassParameters::PerView, ShaderStageVisibility::Compute);
	builder.Uniform("PerTemporal", &PathTracedDirectLightingPassParameters::PerTemporal, ShaderStageVisibility::Compute);
	builder.Uniform("ViewLighting", &PathTracedDirectLightingPassParameters::ViewLighting, ShaderStageVisibility::Compute);
	builder.Uniform("RayTracedShadows", &PathTracedDirectLightingPassParameters::RayTracedShadows, ShaderStageVisibility::Compute);
	builder.Uniform(
	    "PathTracedLightingConstants",
	    &PathTracedDirectLightingPassParameters::PathTracedLightingConstants,
	    ShaderStageVisibility::Compute);
	builder.ReadTexture("GBufferBaseColor", &PathTracedDirectLightingPassParameters::GBufferBaseColor, ShaderStageVisibility::Compute);
	builder.ReadTexture("GBufferNormal", &PathTracedDirectLightingPassParameters::GBufferNormal, ShaderStageVisibility::Compute);
	builder.ReadTexture("GBufferMaterial", &PathTracedDirectLightingPassParameters::GBufferMaterial, ShaderStageVisibility::Compute);
	builder.ReadTexture("GBufferSubsurface", &PathTracedDirectLightingPassParameters::GBufferSubsurface, ShaderStageVisibility::Compute);
	builder.ReadTexture("SceneDepth", &PathTracedDirectLightingPassParameters::SceneDepth, ShaderStageVisibility::Compute);
	builder.ReadBuffer("DirectionalLights", &PathTracedDirectLightingPassParameters::DirectionalLights, ShaderStageVisibility::Compute);
	builder.ReadBuffer("PointLights", &PathTracedDirectLightingPassParameters::PointLights, ShaderStageVisibility::Compute);
	builder.ReadBuffer("SpotLights", &PathTracedDirectLightingPassParameters::SpotLights, ShaderStageVisibility::Compute);
	builder.ReadBuffer("RectLights", &PathTracedDirectLightingPassParameters::RectLights, ShaderStageVisibility::Compute);
	builder.ReadBuffer(
	    "RayTracingHitVertices",
	    &PathTracedDirectLightingPassParameters::RayTracingHitVertices,
	    ShaderStageVisibility::Compute);
	builder.ReadBuffer(
	    "RayTracingHitIndices",
	    &PathTracedDirectLightingPassParameters::RayTracingHitIndices,
	    ShaderStageVisibility::Compute);
	builder.ReadBuffer(
	    "RayTracingHitInstances",
	    &PathTracedDirectLightingPassParameters::RayTracingHitInstances,
	    ShaderStageVisibility::Compute);
	builder.ReadBuffer(
	    "RayTracingHitMaterials",
	    &PathTracedDirectLightingPassParameters::RayTracingHitMaterials,
	    ShaderStageVisibility::Compute);
	builder.ReadTexture(
	    "MaterialTextureTable",
	    &PathTracedDirectLightingPassParameters::MaterialTextureTable,
	    ShaderStageVisibility::Compute);
	builder.Sampler(
	    "MaterialTextureSampler",
	    &PathTracedDirectLightingPassParameters::MaterialTextureSampler,
	    ShaderStageVisibility::Compute);
}

PathTracedDirectLightingPass::PathTracedDirectLightingPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const PathTracedDirectLightingPass::ParameterMetadata& PathTracedDirectLightingPass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<PathTracedDirectLightingPass>();
}
const RenderPassDefinition& PathTracedDirectLightingPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    RendererShaderPackages::PathTracedDirectLighting,
	    L"PathTracedDirectLighting_BindingLayout",
	    L"PathTracedDirectLighting_PipelineState",
	    RayTracingShaderFeatureFlags::DescriptorRayQuery);
	return definition;
}

void PathTracedDirectLightingPass::DeclareResources(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer,
    FrameGraphAccelerationStructureHandle sceneTlas,
    ParameterInstance& parameters)
{
	parameters->DirectDiffuse = builder.CreateUAV(lighting.DirectDiffuse);
	parameters->DirectSpecular = builder.CreateUAV(lighting.DirectSpecular);
	parameters->DirectSubsurface = builder.CreateUAV(lighting.DirectSubsurface);
	parameters->SceneTlas = builder.Read(sceneTlas);
	parameters->GBufferBaseColor = builder.CreateSRV(gbuffer.BaseColor);
	parameters->GBufferNormal = builder.CreateSRV(gbuffer.Normal);
	parameters->GBufferMaterial = builder.CreateSRV(gbuffer.Material);
	parameters->GBufferSubsurface = builder.CreateSRV(gbuffer.Subsurface);
	parameters->SceneDepth = builder.CreateSRV(sceneTargets.SceneDepth);
}

void PathTracedDirectLightingPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	const RayTracingPassCapabilities capabilities = RayTracingPassCapabilityQuery::Build(context.Frame, context.RuntimeServices.RayTracing);
	if (!capabilities.InlineRayQueryAvailable ||
	    !RayTracingPassCapabilityQuery::CanUseSceneTlas(capabilities, RayTracingSceneTlasShaderAccessMode::Descriptor))
	{
		return;
	}
	parameters->PerFrame = context.RuntimeServices.PerFrame;
	parameters->PerView = context.Frame.mainView.perViewData;
	parameters->PerTemporal = context.Frame.mainView.perTemporalData;
	parameters->ViewLighting = context.Frame.lighting.GetConstants();
	parameters->DirectionalLights = context.Frame.lighting.GetDirectionalLightsShaderResourceView();
	parameters->PointLights = context.Frame.lighting.GetPointLightsShaderResourceView();
	parameters->SpotLights = context.Frame.lighting.GetSpotLightsShaderResourceView();
	parameters->RectLights = context.Frame.lighting.GetRectLightsShaderResourceView();
	parameters->RayTracingHitVertices = context.Frame.rayTracingHitData.GetVertexShaderResourceView();
	parameters->RayTracingHitIndices = context.Frame.rayTracingHitData.GetIndexShaderResourceView();
	parameters->RayTracingHitInstances = context.Frame.rayTracingHitData.GetInstanceShaderResourceView();
	parameters->RayTracingHitMaterials = context.Frame.rayTracingHitData.GetMaterialShaderResourceView();
	parameters->MaterialTextureSampler = RhiSamplerDesc{
	    .MinMagFilter = RhiSamplerMinMagFilter::Linear,
	    .MipFilter = RhiSamplerMipFilter::Linear,
	    .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Wrap),
	    .MaxAnisotropy = RhiSamplerAnisotropy::X1};

	const RenderBindingSet* materialTextureTable = context.Frame.sceneData.materialTextureTable;
	const std::uint32_t descriptorCount = context.Frame.sceneData.materialTextureTableDescriptorCount;
	const bool materialTextureTableAvailable =
	    context.Frame.sceneData.materialTextureTableValid && materialTextureTable != nullptr && *materialTextureTable &&
	    descriptorCount > 0u && descriptorCount <= MaterialTextureTableFixedCapacity &&
	    materialTextureTable->GetDescriptorCount() >= descriptorCount;
	if (materialTextureTableAvailable)
	{
		parameters->MaterialTextureTable = materialTextureTable->GetTableBinding(0);
	}
	parameters->RayTracedShadows = RayTracedShadowPassData::Build(
	    context.RuntimeServices.RayTracing,
	    context.Frame.rayTracingScene.HasTraceableInstances(),
	    capabilities.TriangleMaterialDataAvailable && materialTextureTableAvailable,
	    context.Frame.rayTracingHitData.GetInstanceCount(),
	    context.Frame.rayTracingHitData.GetMaterialCount());

	const PathTracedLightingSettings settings = BuildPathTracedLightingSettings();
	parameters->PathTracedLightingConstants = PathTracedLightingUniformData{
	    .SamplesPerPixel = settings.SamplesPerPixel,
	    .BounceCount = settings.BounceCount,
	    .NormalBias = settings.NormalBias,
	    .MaxDistance = settings.MaxDistance};
	ComputePassUtilities::DispatchSized<PathTracedDirectLightingPass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
}

#include "../../PCH.h"
#include "Passes/RayTracing/RestirIndirectSpatialPass.h"

#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/Core/ComputePassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "RayTracing/RayTracingShaderFeatureFlags.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "RayTracing/Effects/RestirLighting/RestirIndirectLightingSettings.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassData.h"
#include "RayTracing/RayTracingPassCapabilityQuery.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"

void RestirIndirectSpatialPassParameters::Describe(ShaderParameterStructBuilder<RestirIndirectSpatialPassParameters>& builder)
{
	builder.ReadTexture(
	    "TemporalReservoirSampleTexture",
	    &RestirIndirectSpatialPassParameters::TemporalReservoirSampleTexture,
	    ShaderStageVisibility::Compute);
	builder.ReadTexture(
	    "TemporalReservoirWeightTexture",
	    &RestirIndirectSpatialPassParameters::TemporalReservoirWeightTexture,
	    ShaderStageVisibility::Compute);
	builder.RWTexture(
	    "CurrentReservoirSampleTexture",
	    &RestirIndirectSpatialPassParameters::CurrentReservoirSampleTexture,
	    ShaderStageVisibility::Compute);
	builder.RWTexture(
	    "CurrentReservoirWeightTexture",
	    &RestirIndirectSpatialPassParameters::CurrentReservoirWeightTexture,
	    ShaderStageVisibility::Compute);
	builder.RWTexture(
	    "CurrentReservoirSurfaceTexture",
	    &RestirIndirectSpatialPassParameters::CurrentReservoirSurfaceTexture,
	    ShaderStageVisibility::Compute);
	builder.AccelerationStructure("SceneTlas", &RestirIndirectSpatialPassParameters::SceneTlas, ShaderStageVisibility::Compute);
	builder.Uniform("PerFrame", &RestirIndirectSpatialPassParameters::PerFrame, ShaderStageVisibility::Compute);
	builder.Uniform("PerView", &RestirIndirectSpatialPassParameters::PerView, ShaderStageVisibility::Compute);
	builder.Uniform("PerTemporal", &RestirIndirectSpatialPassParameters::PerTemporal, ShaderStageVisibility::Compute);
	builder.Uniform("ViewLighting", &RestirIndirectSpatialPassParameters::ViewLighting, ShaderStageVisibility::Compute);
	builder.Uniform("RayTracedShadows", &RestirIndirectSpatialPassParameters::RayTracedShadows, ShaderStageVisibility::Compute);
	builder.Uniform("Sky", &RestirIndirectSpatialPassParameters::Sky, ShaderStageVisibility::Compute);
	builder.Uniform(
	    "RestirIndirectConstants",
	    &RestirIndirectSpatialPassParameters::RestirIndirectConstants,
	    ShaderStageVisibility::Compute);
	builder.ReadTexture("GBufferBaseColor", &RestirIndirectSpatialPassParameters::GBufferBaseColor, ShaderStageVisibility::Compute);
	builder.ReadTexture("GBufferNormal", &RestirIndirectSpatialPassParameters::GBufferNormal, ShaderStageVisibility::Compute);
	builder.ReadTexture("GBufferMaterial", &RestirIndirectSpatialPassParameters::GBufferMaterial, ShaderStageVisibility::Compute);
	builder.ReadTexture("SceneDepth", &RestirIndirectSpatialPassParameters::SceneDepth, ShaderStageVisibility::Compute);
	builder.ReadTexture("SkyTexture", &RestirIndirectSpatialPassParameters::SkyTexture, ShaderStageVisibility::Compute);
	builder.Sampler("SamplerLinearClamp", &RestirIndirectSpatialPassParameters::SamplerLinearClamp, ShaderStageVisibility::Compute);
	builder.ReadBuffer(
	    "RayTracingHitVertices",
	    &RestirIndirectSpatialPassParameters::RayTracingHitVertices,
	    ShaderStageVisibility::Compute);
	builder.ReadBuffer("RayTracingHitIndices", &RestirIndirectSpatialPassParameters::RayTracingHitIndices, ShaderStageVisibility::Compute);
	builder.ReadBuffer(
	    "RayTracingHitInstances",
	    &RestirIndirectSpatialPassParameters::RayTracingHitInstances,
	    ShaderStageVisibility::Compute);
	builder.ReadBuffer(
	    "RayTracingHitMaterials",
	    &RestirIndirectSpatialPassParameters::RayTracingHitMaterials,
	    ShaderStageVisibility::Compute);
	builder.ReadBuffer("MeshInstances", &RestirIndirectSpatialPassParameters::MeshInstances, ShaderStageVisibility::Compute);
	builder.ReadBuffer("SkinInfluences", &RestirIndirectSpatialPassParameters::SkinInfluences, ShaderStageVisibility::Compute);
	builder.ReadBuffer("JointMatrices", &RestirIndirectSpatialPassParameters::JointMatrices, ShaderStageVisibility::Compute);
	builder.ReadBuffer("DirectionalLights", &RestirIndirectSpatialPassParameters::DirectionalLights, ShaderStageVisibility::Compute);
	builder.ReadBuffer("PointLights", &RestirIndirectSpatialPassParameters::PointLights, ShaderStageVisibility::Compute);
	builder.ReadBuffer("SpotLights", &RestirIndirectSpatialPassParameters::SpotLights, ShaderStageVisibility::Compute);
	builder.ReadBuffer("RectLights", &RestirIndirectSpatialPassParameters::RectLights, ShaderStageVisibility::Compute);
	builder.ReadTexture("MaterialTextureTable", &RestirIndirectSpatialPassParameters::MaterialTextureTable, ShaderStageVisibility::Compute);
	builder.Sampler("MaterialTextureSampler", &RestirIndirectSpatialPassParameters::MaterialTextureSampler, ShaderStageVisibility::Compute);
}

const RestirIndirectSpatialPass::ParameterMetadata& RestirIndirectSpatialPass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<RestirIndirectSpatialPass>();
}

const RenderPassDefinition& RestirIndirectSpatialPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    RendererShaderPackages::RestirIndirectSpatial,
	    L"RestirIndirectSpatial_BindingLayout",
	    L"RestirIndirectSpatial_PipelineState",
	    RayTracingShaderFeatureFlags::DescriptorRayQuery);
	return definition;
}

void RestirIndirectSpatialPass::DeclareResources(
    FrameGraphBuilder& builder,
    const SceneRenderTargets& scene,
    const GBufferRenderTargets& gbuffer,
    FrameGraphTextureHandle temporalSample,
    FrameGraphTextureHandle temporalWeight,
    FrameGraphTextureHandle currentSample,
    FrameGraphTextureHandle currentWeight,
    FrameGraphTextureHandle currentSurface,
    FrameGraphAccelerationStructureHandle sceneTlas,
    FrameGraphTextureHandle sky,
    FrameGraphBufferHandle directionalLights,
    FrameGraphBufferHandle pointLights,
    FrameGraphBufferHandle spotLights,
    FrameGraphBufferHandle rectLights,
    FrameGraphBufferHandle hitVertices,
    FrameGraphBufferHandle hitSkinInfluences,
    FrameGraphBufferHandle hitIndices,
    FrameGraphBufferHandle hitInstances,
    FrameGraphBufferHandle hitMaterials,
    FrameGraphBufferHandle meshInstances,
    FrameGraphBufferHandle jointMatrices,
    ParameterInstance& parameters)
{
	parameters->TemporalReservoirSampleTexture = builder.CreateSRV(temporalSample);
	parameters->TemporalReservoirWeightTexture = builder.CreateSRV(temporalWeight);
	parameters->CurrentReservoirSampleTexture = builder.CreateUAV(currentSample);
	parameters->CurrentReservoirWeightTexture = builder.CreateUAV(currentWeight);
	parameters->CurrentReservoirSurfaceTexture = builder.CreateUAV(currentSurface);
	parameters->SceneTlas = builder.Read(sceneTlas);
	parameters->GBufferBaseColor = builder.CreateSRV(gbuffer.BaseColor);
	parameters->GBufferNormal = builder.CreateSRV(gbuffer.Normal);
	parameters->GBufferMaterial = builder.CreateSRV(gbuffer.Material);
	parameters->SceneDepth = builder.CreateSRV(scene.SceneDepth);
	parameters->SkyTexture = builder.CreateSRV(sky);
	parameters->DirectionalLights = builder.CreateSRV(directionalLights);
	parameters->PointLights = builder.CreateSRV(pointLights);
	parameters->SpotLights = builder.CreateSRV(spotLights);
	parameters->RectLights = builder.CreateSRV(rectLights);
	parameters->RayTracingHitVertices = builder.CreateSRV(hitVertices);
	parameters->SkinInfluences = builder.CreateSRV(hitSkinInfluences);
	parameters->RayTracingHitIndices = builder.CreateSRV(hitIndices);
	parameters->RayTracingHitInstances = builder.CreateSRV(hitInstances);
	parameters->RayTracingHitMaterials = builder.CreateSRV(hitMaterials);
	parameters->MeshInstances = builder.CreateSRV(meshInstances);
	parameters->JointMatrices = builder.CreateSRV(jointMatrices);
}

void RestirIndirectSpatialPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	const RayTracingPassCapabilities capabilities = RayTracingPassCapabilityQuery::Build(context.Frame, context.RuntimeServices.RayTracing);
	if (!capabilities.InlineRayQueryAvailable || !capabilities.HitDataAvailable || !capabilities.MaterialTextureTableAvailable ||
	    !RayTracingPassCapabilityQuery::CanUseSceneTlas(capabilities, RayTracingSceneTlasShaderAccessMode::Descriptor))
	{
		return;
	}

	const RenderBindingSet* materialTextureTable = context.Frame.sceneData.materialTextureTable;
	const std::uint32_t descriptorCount = context.Frame.sceneData.materialTextureTableDescriptorCount;
	if (!context.Frame.sceneData.materialTextureTableValid || materialTextureTable == nullptr || !*materialTextureTable ||
	    descriptorCount == 0u || descriptorCount > MaterialTextureTableFixedCapacity ||
	    materialTextureTable->GetDescriptorCount() < descriptorCount)
	{
		return;
	}

	parameters->PerFrame = context.RuntimeServices.PerFrame;
	parameters->PerView = context.Frame.mainView.perViewData;
	parameters->PerTemporal = context.Frame.mainView.perTemporalData;
	parameters->ViewLighting = context.Frame.lighting.GetConstants();
	parameters->Sky = MakeSkyUniformData(context.Frame.sceneData.sky);
	parameters->MaterialTextureTable = materialTextureTable->GetTableBinding(0);
	parameters->SamplerLinearClamp = RhiSamplerDesc{
	    .MinMagFilter = RhiSamplerMinMagFilter::Linear,
	    .MipFilter = RhiSamplerMipFilter::Linear,
	    .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Clamp)};
	parameters->MaterialTextureSampler = RhiSamplerDesc{
	    .MinMagFilter = RhiSamplerMinMagFilter::Linear,
	    .MipFilter = RhiSamplerMipFilter::Linear,
	    .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Wrap),
	    .MaxAnisotropy = RhiSamplerAnisotropy::X1};
	parameters->RayTracedShadows = RayTracedShadowPassData::Build(
	    context.RuntimeServices.RayTracing,
	    context.Frame.rayTracingScene.HasTraceableInstances(),
	    capabilities.TriangleMaterialDataAvailable,
	    context.Frame.rayTracingHitData.GetInstanceCount(),
	    context.Frame.rayTracingHitData.GetMaterialCount());

	const RestirIndirectLightingSettings settings = BuildRestirIndirectLightingSettings();
	parameters->RestirIndirectConstants = RestirIndirectLightingUniformData{
	    .BounceCount = settings.BounceCount,
	    .NormalBias = settings.NormalBias,
	    .MaxDistance = settings.MaxDistance};
	ComputePassUtilities::DispatchSized<RestirIndirectSpatialPass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
}

#include "../../PCH.h"
#include "Passes/RayTracing/RestirIndirectResolvePass.h"

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

void RestirIndirectResolvePassParameters::Describe(ShaderParameterStructBuilder<RestirIndirectResolvePassParameters>& builder)
{
	builder.ReadTexture(
	    "CurrentReservoirSampleTexture",
	    &RestirIndirectResolvePassParameters::CurrentReservoirSampleTexture,
	    ShaderStageVisibility::Compute);
	builder.ReadTexture(
	    "CurrentReservoirWeightTexture",
	    &RestirIndirectResolvePassParameters::CurrentReservoirWeightTexture,
	    ShaderStageVisibility::Compute);
	builder.RWTexture("IndirectDiffuse", &RestirIndirectResolvePassParameters::IndirectDiffuse, ShaderStageVisibility::Compute);
	builder.RWTexture("IndirectSpecular", &RestirIndirectResolvePassParameters::IndirectSpecular, ShaderStageVisibility::Compute);
	builder.RWTexture(
	    "RayReconstructionDiffuseAlbedo",
	    &RestirIndirectResolvePassParameters::RayReconstructionDiffuseAlbedo,
	    ShaderStageVisibility::Compute);
	builder.RWTexture(
	    "RayReconstructionSpecularAlbedo",
	    &RestirIndirectResolvePassParameters::RayReconstructionSpecularAlbedo,
	    ShaderStageVisibility::Compute);
	builder.RWTexture(
	    "RayReconstructionRoughness",
	    &RestirIndirectResolvePassParameters::RayReconstructionRoughness,
	    ShaderStageVisibility::Compute);
	builder.RWTexture(
	    "RayReconstructionSpecularHitDistance",
	    &RestirIndirectResolvePassParameters::RayReconstructionSpecularHitDistance,
	    ShaderStageVisibility::Compute);
	builder.AccelerationStructure("SceneTlas", &RestirIndirectResolvePassParameters::SceneTlas, ShaderStageVisibility::Compute);
	builder.Uniform("PerFrame", &RestirIndirectResolvePassParameters::PerFrame, ShaderStageVisibility::Compute);
	builder.Uniform("PerView", &RestirIndirectResolvePassParameters::PerView, ShaderStageVisibility::Compute);
	builder.Uniform("PerTemporal", &RestirIndirectResolvePassParameters::PerTemporal, ShaderStageVisibility::Compute);
	builder.Uniform("ViewLighting", &RestirIndirectResolvePassParameters::ViewLighting, ShaderStageVisibility::Compute);
	builder.Uniform("RayTracedShadows", &RestirIndirectResolvePassParameters::RayTracedShadows, ShaderStageVisibility::Compute);
	builder.Uniform("Sky", &RestirIndirectResolvePassParameters::Sky, ShaderStageVisibility::Compute);
	builder.Uniform(
	    "RestirIndirectConstants",
	    &RestirIndirectResolvePassParameters::RestirIndirectConstants,
	    ShaderStageVisibility::Compute);
	builder.ReadTexture("GBufferBaseColor", &RestirIndirectResolvePassParameters::GBufferBaseColor, ShaderStageVisibility::Compute);
	builder.ReadTexture("GBufferNormal", &RestirIndirectResolvePassParameters::GBufferNormal, ShaderStageVisibility::Compute);
	builder.ReadTexture("GBufferMaterial", &RestirIndirectResolvePassParameters::GBufferMaterial, ShaderStageVisibility::Compute);
	builder.ReadTexture("SceneDepth", &RestirIndirectResolvePassParameters::SceneDepth, ShaderStageVisibility::Compute);
	builder.ReadTexture("SkyTexture", &RestirIndirectResolvePassParameters::SkyTexture, ShaderStageVisibility::Compute);
	builder.Sampler("SamplerLinearClamp", &RestirIndirectResolvePassParameters::SamplerLinearClamp, ShaderStageVisibility::Compute);
	builder.ReadBuffer(
	    "RayTracingHitVertices",
	    &RestirIndirectResolvePassParameters::RayTracingHitVertices,
	    ShaderStageVisibility::Compute);
	builder.ReadBuffer("RayTracingHitIndices", &RestirIndirectResolvePassParameters::RayTracingHitIndices, ShaderStageVisibility::Compute);
	builder.ReadBuffer(
	    "RayTracingHitInstances",
	    &RestirIndirectResolvePassParameters::RayTracingHitInstances,
	    ShaderStageVisibility::Compute);
	builder.ReadBuffer(
	    "RayTracingHitMaterials",
	    &RestirIndirectResolvePassParameters::RayTracingHitMaterials,
	    ShaderStageVisibility::Compute);
	builder.ReadBuffer("MeshInstances", &RestirIndirectResolvePassParameters::MeshInstances, ShaderStageVisibility::Compute);
	builder.ReadBuffer("SkinInfluences", &RestirIndirectResolvePassParameters::SkinInfluences, ShaderStageVisibility::Compute);
	builder.ReadBuffer("JointMatrices", &RestirIndirectResolvePassParameters::JointMatrices, ShaderStageVisibility::Compute);
	builder.ReadBuffer("DirectionalLights", &RestirIndirectResolvePassParameters::DirectionalLights, ShaderStageVisibility::Compute);
	builder.ReadBuffer("PointLights", &RestirIndirectResolvePassParameters::PointLights, ShaderStageVisibility::Compute);
	builder.ReadBuffer("SpotLights", &RestirIndirectResolvePassParameters::SpotLights, ShaderStageVisibility::Compute);
	builder.ReadBuffer("RectLights", &RestirIndirectResolvePassParameters::RectLights, ShaderStageVisibility::Compute);
	builder.ReadTexture("MaterialTextureTable", &RestirIndirectResolvePassParameters::MaterialTextureTable, ShaderStageVisibility::Compute);
	builder.Sampler("MaterialTextureSampler", &RestirIndirectResolvePassParameters::MaterialTextureSampler, ShaderStageVisibility::Compute);
}

const RestirIndirectResolvePass::ParameterMetadata& RestirIndirectResolvePass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<RestirIndirectResolvePass>();
}

const RenderPassDefinition& RestirIndirectResolvePass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    RendererShaderPackages::RestirIndirectResolve,
	    L"RestirIndirectResolve_BindingLayout",
	    L"RestirIndirectResolve_PipelineState",
	    RayTracingShaderFeatureFlags::DescriptorRayQuery);
	return definition;
}

void RestirIndirectResolvePass::DeclareResources(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const SceneRenderTargets& scene,
    const GBufferRenderTargets& gbuffer,
    FrameGraphTextureHandle currentSample,
    FrameGraphTextureHandle currentWeight,
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
	parameters->CurrentReservoirSampleTexture = builder.CreateSRV(currentSample);
	parameters->CurrentReservoirWeightTexture = builder.CreateSRV(currentWeight);
	parameters->IndirectDiffuse = builder.CreateUAV(lighting.IndirectDiffuse);
	parameters->IndirectSpecular = builder.CreateUAV(lighting.IndirectSpecular);
	parameters->RayReconstructionDiffuseAlbedo = builder.CreateUAV(lighting.ReconstructionGuides.DiffuseAlbedo);
	parameters->RayReconstructionSpecularAlbedo = builder.CreateUAV(lighting.ReconstructionGuides.SpecularAlbedo);
	parameters->RayReconstructionRoughness = builder.CreateUAV(lighting.ReconstructionGuides.Roughness);
	parameters->RayReconstructionSpecularHitDistance = builder.CreateUAV(lighting.ReconstructionGuides.SpecularHitDistance);
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

void RestirIndirectResolvePass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
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
	parameters->Sky = SkyUniformData{
	    .Color = context.Frame.sceneData.sky.color,
	    .Intensity = context.Frame.sceneData.sky.intensity,
	    .Enabled = context.Frame.sceneData.sky.enabled ? 1u : 0u};
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
	ComputePassUtilities::DispatchSized<RestirIndirectResolvePass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
}

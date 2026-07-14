#include "../../PCH.h"
#include "Passes/RayTracing/RestirIndirectTemporalPass.h"

#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Passes/Core/ComputePassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "RayTracing/RayTracingShaderFeatureFlags.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "RayTracing/Effects/RestirLighting/RestirIndirectLightingSettings.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassData.h"
#include "RayTracing/RayTracingPassCapabilityQuery.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"

void RestirIndirectTemporalPassParameters::Describe(ShaderParameterStructBuilder<RestirIndirectTemporalPassParameters>& builder)
{
	builder.RWTexture(
	    "TemporalReservoirSampleTexture",
	    &RestirIndirectTemporalPassParameters::TemporalReservoirSampleTexture,
	    ShaderStageVisibility::Compute);
	builder.RWTexture(
	    "TemporalReservoirWeightTexture",
	    &RestirIndirectTemporalPassParameters::TemporalReservoirWeightTexture,
	    ShaderStageVisibility::Compute);
	builder.ReadTexture(
	    "PreviousReservoirSampleTexture",
	    &RestirIndirectTemporalPassParameters::PreviousReservoirSampleTexture,
	    ShaderStageVisibility::Compute);
	builder.ReadTexture(
	    "PreviousReservoirWeightTexture",
	    &RestirIndirectTemporalPassParameters::PreviousReservoirWeightTexture,
	    ShaderStageVisibility::Compute);
	builder.ReadTexture(
	    "PreviousReservoirSurfaceTexture",
	    &RestirIndirectTemporalPassParameters::PreviousReservoirSurfaceTexture,
	    ShaderStageVisibility::Compute);
	builder.ReadTexture("GBufferMotionVector", &RestirIndirectTemporalPassParameters::GBufferMotionVector, ShaderStageVisibility::Compute);
	builder.AccelerationStructure("SceneTlas", &RestirIndirectTemporalPassParameters::SceneTlas, ShaderStageVisibility::Compute);
	builder.Uniform("PerFrame", &RestirIndirectTemporalPassParameters::PerFrame, ShaderStageVisibility::Compute);
	builder.Uniform("PerView", &RestirIndirectTemporalPassParameters::PerView, ShaderStageVisibility::Compute);
	builder.Uniform("PerTemporal", &RestirIndirectTemporalPassParameters::PerTemporal, ShaderStageVisibility::Compute);
	builder.Uniform("ViewLighting", &RestirIndirectTemporalPassParameters::ViewLighting, ShaderStageVisibility::Compute);
	builder.Uniform("RayTracedShadows", &RestirIndirectTemporalPassParameters::RayTracedShadows, ShaderStageVisibility::Compute);
	builder.Uniform("Sky", &RestirIndirectTemporalPassParameters::Sky, ShaderStageVisibility::Compute);
	builder.Uniform(
	    "RestirIndirectConstants",
	    &RestirIndirectTemporalPassParameters::RestirIndirectConstants,
	    ShaderStageVisibility::Compute);
	builder.ReadTexture("GBufferBaseColor", &RestirIndirectTemporalPassParameters::GBufferBaseColor, ShaderStageVisibility::Compute);
	builder.ReadTexture("GBufferNormal", &RestirIndirectTemporalPassParameters::GBufferNormal, ShaderStageVisibility::Compute);
	builder.ReadTexture("GBufferMaterial", &RestirIndirectTemporalPassParameters::GBufferMaterial, ShaderStageVisibility::Compute);
	builder.ReadTexture("SceneDepth", &RestirIndirectTemporalPassParameters::SceneDepth, ShaderStageVisibility::Compute);
	builder.ReadTexture("SkyTexture", &RestirIndirectTemporalPassParameters::SkyTexture, ShaderStageVisibility::Compute);
	builder.Sampler("SamplerLinearClamp", &RestirIndirectTemporalPassParameters::SamplerLinearClamp, ShaderStageVisibility::Compute);
	builder.ReadBuffer(
	    "RayTracingHitVertices",
	    &RestirIndirectTemporalPassParameters::RayTracingHitVertices,
	    ShaderStageVisibility::Compute);
	builder.ReadBuffer("RayTracingHitIndices", &RestirIndirectTemporalPassParameters::RayTracingHitIndices, ShaderStageVisibility::Compute);
	builder.ReadBuffer(
	    "RayTracingHitInstances",
	    &RestirIndirectTemporalPassParameters::RayTracingHitInstances,
	    ShaderStageVisibility::Compute);
	builder.ReadBuffer(
	    "RayTracingHitMaterials",
	    &RestirIndirectTemporalPassParameters::RayTracingHitMaterials,
	    ShaderStageVisibility::Compute);
	builder.ReadBuffer("MeshInstances", &RestirIndirectTemporalPassParameters::MeshInstances, ShaderStageVisibility::Compute);
	builder.ReadBuffer("SkinInfluences", &RestirIndirectTemporalPassParameters::SkinInfluences, ShaderStageVisibility::Compute);
	builder.ReadBuffer("JointMatrices", &RestirIndirectTemporalPassParameters::JointMatrices, ShaderStageVisibility::Compute);
	builder.ReadBuffer("DirectionalLights", &RestirIndirectTemporalPassParameters::DirectionalLights, ShaderStageVisibility::Compute);
	builder.ReadBuffer("PointLights", &RestirIndirectTemporalPassParameters::PointLights, ShaderStageVisibility::Compute);
	builder.ReadBuffer("SpotLights", &RestirIndirectTemporalPassParameters::SpotLights, ShaderStageVisibility::Compute);
	builder.ReadBuffer("RectLights", &RestirIndirectTemporalPassParameters::RectLights, ShaderStageVisibility::Compute);
	builder.ReadTexture(
	    "MaterialTextureTable",
	    &RestirIndirectTemporalPassParameters::MaterialTextureTable,
	    ShaderStageVisibility::Compute);
	builder.Sampler(
	    "MaterialTextureSampler",
	    &RestirIndirectTemporalPassParameters::MaterialTextureSampler,
	    ShaderStageVisibility::Compute);
}

const RestirIndirectTemporalPass::ParameterMetadata& RestirIndirectTemporalPass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<RestirIndirectTemporalPass>();
}

const RenderPassDefinition& RestirIndirectTemporalPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    RendererShaderPackages::RestirIndirectTemporal,
	    L"RestirIndirectTemporal_BindingLayout",
	    L"RestirIndirectTemporal_PipelineState",
	    RayTracingShaderFeatureFlags::DescriptorRayQuery);
	return definition;
}

void RestirIndirectTemporalPass::DeclareResources(
    FrameGraphBuilder& builder,
    const SceneRenderTargets& scene,
    const GBufferRenderTargets& gbuffer,
    FrameGraphTextureHandle temporalSample,
    FrameGraphTextureHandle temporalWeight,
    FrameGraphTextureHandle previousSample,
    FrameGraphTextureHandle previousWeight,
    FrameGraphTextureHandle previousSurface,
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
	parameters->TemporalReservoirSampleTexture = builder.CreateUAV(temporalSample);
	parameters->TemporalReservoirWeightTexture = builder.CreateUAV(temporalWeight);
	parameters->PreviousReservoirSampleTexture = builder.CreateSRV(previousSample);
	parameters->PreviousReservoirWeightTexture = builder.CreateSRV(previousWeight);
	parameters->PreviousReservoirSurfaceTexture = builder.CreateSRV(previousSurface);
	parameters->GBufferMotionVector = builder.CreateSRV(gbuffer.MotionVector);
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

void RestirIndirectTemporalPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
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

	PerTemporalConstantBufferData temporalData = context.Frame.mainView.perTemporalData;
	if (!context.RuntimeServices.History.RestirIndirectReservoir)
	{
		temporalData.HistoryValid = 0u;
	}
	parameters->PerTemporal = temporalData;
	ComputePassUtilities::DispatchSized<RestirIndirectTemporalPass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
}

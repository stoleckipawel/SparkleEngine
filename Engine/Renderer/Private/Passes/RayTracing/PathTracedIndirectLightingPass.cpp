#include "../../PCH.h"
#include "Passes/RayTracing/PathTracedIndirectLightingPass.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Passes/Core/ComputePassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassData.h"
#include "RayTracing/RayTracingShaderFeatureFlags.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "RayTracing/Effects/PathTracedLighting/PathTracedLightingSettings.h"
#include "RayTracing/RayTracingPassCapabilityQuery.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"
#include "Textures/TextureManager.h"

void PathTracedIndirectLightingPassParameters::Describe(ShaderParameterStructBuilder<PathTracedIndirectLightingPassParameters>& builder)
{
	builder.AccelerationStructure("SceneTlas", &PathTracedIndirectLightingPassParameters::SceneTlas, ShaderStageVisibility::Compute);
	builder.Uniform("PerFrame", &PathTracedIndirectLightingPassParameters::PerFrame, ShaderStageVisibility::Compute);
	builder.Uniform("PerView", &PathTracedIndirectLightingPassParameters::PerView, ShaderStageVisibility::Compute);
	builder.Uniform("PerTemporal", &PathTracedIndirectLightingPassParameters::PerTemporal, ShaderStageVisibility::Compute);
	builder.Uniform("ViewLighting", &PathTracedIndirectLightingPassParameters::ViewLighting, ShaderStageVisibility::Compute);
	builder.Uniform("RayTracedShadows", &PathTracedIndirectLightingPassParameters::RayTracedShadows, ShaderStageVisibility::Compute);
	builder.ReadTexture("GBufferBaseColor", &PathTracedIndirectLightingPassParameters::GBufferBaseColor, ShaderStageVisibility::Compute);
	builder.ReadTexture("GBufferNormal", &PathTracedIndirectLightingPassParameters::GBufferNormal, ShaderStageVisibility::Compute);
	builder.ReadTexture("GBufferMaterial", &PathTracedIndirectLightingPassParameters::GBufferMaterial, ShaderStageVisibility::Compute);
	builder.ReadTexture("SceneDepth", &PathTracedIndirectLightingPassParameters::SceneDepth, ShaderStageVisibility::Compute);
	builder.ReadTexture("SkyTexture", &PathTracedIndirectLightingPassParameters::SkyTexture, ShaderStageVisibility::Compute);
	builder.Sampler("SamplerLinearClamp", &PathTracedIndirectLightingPassParameters::SamplerLinearClamp, ShaderStageVisibility::Compute);
	builder.ReadBuffer("RayTracingHitVertices", &PathTracedIndirectLightingPassParameters::RayTracingHitVertices, ShaderStageVisibility::Compute);
	builder.ReadBuffer("RayTracingHitIndices", &PathTracedIndirectLightingPassParameters::RayTracingHitIndices, ShaderStageVisibility::Compute);
	builder.ReadBuffer("RayTracingHitInstances", &PathTracedIndirectLightingPassParameters::RayTracingHitInstances, ShaderStageVisibility::Compute);
	builder.ReadBuffer("RayTracingHitMaterials", &PathTracedIndirectLightingPassParameters::RayTracingHitMaterials, ShaderStageVisibility::Compute);
	builder.ReadBuffer("MeshInstances", &PathTracedIndirectLightingPassParameters::MeshInstances, ShaderStageVisibility::Compute);
	builder.ReadBuffer("SkinInfluences", &PathTracedIndirectLightingPassParameters::SkinInfluences, ShaderStageVisibility::Compute);
	builder.ReadBuffer("JointMatrices", &PathTracedIndirectLightingPassParameters::JointMatrices, ShaderStageVisibility::Compute);
	builder.ReadBuffer("DirectionalLights", &PathTracedIndirectLightingPassParameters::DirectionalLights, ShaderStageVisibility::Compute);
	builder.ReadBuffer("PointLights", &PathTracedIndirectLightingPassParameters::PointLights, ShaderStageVisibility::Compute);
	builder.ReadBuffer("SpotLights", &PathTracedIndirectLightingPassParameters::SpotLights, ShaderStageVisibility::Compute);
	builder.ReadBuffer("RectLights", &PathTracedIndirectLightingPassParameters::RectLights, ShaderStageVisibility::Compute);
	builder.ReadTexture("MaterialTextureTable", &PathTracedIndirectLightingPassParameters::MaterialTextureTable, ShaderStageVisibility::Compute);
	builder.Sampler("MaterialTextureSampler", &PathTracedIndirectLightingPassParameters::MaterialTextureSampler, ShaderStageVisibility::Compute);
	builder.RWTexture("IndirectDiffuse", &PathTracedIndirectLightingPassParameters::IndirectDiffuse, ShaderStageVisibility::Compute);
	builder.RWTexture("IndirectSpecular", &PathTracedIndirectLightingPassParameters::IndirectSpecular, ShaderStageVisibility::Compute);
	builder.Uniform(
	    "PathTracedLightingConstants",
	    &PathTracedIndirectLightingPassParameters::PathTracedLightingConstants,
	    ShaderStageVisibility::Compute);
}

PathTracedIndirectLightingPass::PathTracedIndirectLightingPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const PathTracedIndirectLightingPass::ParameterMetadata& PathTracedIndirectLightingPass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<PathTracedIndirectLightingPass>();
}

const RenderPassDefinition& PathTracedIndirectLightingPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    RendererShaderPackages::PathTracedIndirectLighting,
	    L"PathTracedIndirectLighting_BindingLayout",
	    L"PathTracedIndirectLighting_PipelineState",
	    RayTracingShaderFeatureFlags::DescriptorRayQuery);
	return definition;
}

void PathTracedIndirectLightingPass::DeclareResources(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer,
    FrameGraphAccelerationStructureHandle sceneTlas,
    ParameterInstance& parameters)
{
	parameters->IndirectDiffuse = builder.CreateUAV(lighting.IndirectDiffuse);
	parameters->IndirectSpecular = builder.CreateUAV(lighting.IndirectSpecular);
	parameters->SceneTlas = builder.Read(sceneTlas);
	parameters->GBufferBaseColor = builder.CreateSRV(gbuffer.BaseColor);
	parameters->GBufferNormal = builder.CreateSRV(gbuffer.Normal);
	parameters->GBufferMaterial = builder.CreateSRV(gbuffer.Material);
	parameters->SceneDepth = builder.CreateSRV(sceneTargets.SceneDepth);
}

void PathTracedIndirectLightingPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	const RayTracingPassCapabilities capabilities = RayTracingPassCapabilityQuery::Build(context.Frame, context.RuntimeServices.RayTracing);
	if (!capabilities.InlineRayQueryAvailable || !capabilities.HitDataAvailable || !capabilities.MaterialTextureTableAvailable ||
	    !RayTracingPassCapabilityQuery::CanUseSceneTlas(capabilities, RayTracingSceneTlasShaderAccessMode::Descriptor))
	{
		return;
	}

	const TextureManager* textures = context.RuntimeServices.Textures;
	const RhiDescriptorTableBinding environment =
	    textures != nullptr ? textures->GetShaderResourceBinding(textures->ResolveEnvironmentMapTexture()) : RhiDescriptorTableBinding{};
	const RenderBindingSet* materialTextureTable = context.Frame.sceneData.materialTextureTable;
	const std::uint32_t descriptorCount = context.Frame.sceneData.materialTextureTableDescriptorCount;
	if (!environment || !context.Frame.sceneData.materialTextureTableValid || materialTextureTable == nullptr || !*materialTextureTable ||
	    descriptorCount == 0u || descriptorCount > MaterialTextureTableFixedCapacity ||
	    materialTextureTable->GetDescriptorCount() < descriptorCount)
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
	parameters->MeshInstances = context.Frame.meshInstances.GetShaderResourceView();
	parameters->SkinInfluences = context.Frame.rayTracingHitData.GetSkinInfluenceShaderResourceView();
	parameters->JointMatrices = context.Frame.skinning.GetShaderResourceView();
	parameters->SkyTexture = environment;
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
	const PathTracedLightingSettings settings = BuildPathTracedLightingSettings();
	parameters->PathTracedLightingConstants = PathTracedLightingUniformData{
	    .SamplesPerPixel = settings.SamplesPerPixel,
	    .BounceCount = settings.BounceCount,
	    .NormalBias = settings.NormalBias,
	    .MaxDistance = settings.MaxDistance};
	ComputePassUtilities::DispatchSized<PathTracedIndirectLightingPass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
}

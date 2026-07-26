#include "../../PCH.h"
#include "Passes/RayTracing/RestirIndirectResolvePass.h"

#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
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
	builder.ReadBuffer(
	    "MorphTargetDeltas",
	    &RestirIndirectResolvePassParameters::MorphTargetDeltas,
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
	builder.ReadBuffer("MorphWeights", &RestirIndirectResolvePassParameters::MorphWeights, ShaderStageVisibility::Compute);
	builder.ReadBuffer("DirectionalLights", &RestirIndirectResolvePassParameters::DirectionalLights, ShaderStageVisibility::Compute);
	builder.ReadBuffer("PointLights", &RestirIndirectResolvePassParameters::PointLights, ShaderStageVisibility::Compute);
	builder.ReadBuffer("SpotLights", &RestirIndirectResolvePassParameters::SpotLights, ShaderStageVisibility::Compute);
	builder.ReadBuffer("RectLights", &RestirIndirectResolvePassParameters::RectLights, ShaderStageVisibility::Compute);
	builder.ReadTexture("MaterialTextureTable", &RestirIndirectResolvePassParameters::MaterialTextureTable, ShaderStageVisibility::Compute);
	builder.Sampler("MaterialTextureSampler", &RestirIndirectResolvePassParameters::MaterialTextureSampler, ShaderStageVisibility::Compute);
}

RestirIndirectResolvePass::RestirIndirectResolvePass(
    const ComputePassPipelineRuntime& runtime) noexcept :
	m_runtime(runtime)
{
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

void RestirIndirectResolvePass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	const RayTracingPassCapabilities capabilities = RayTracingPassCapabilityQuery::Build(context.Frame, context.RuntimeServices.RayTracing);
	if (!capabilities.InlineRayQueryAvailable || !capabilities.HitDataAvailable || !capabilities.MaterialTextureTableAvailable ||
	    !RayTracingPassCapabilityQuery::CanUseSceneTlas(capabilities, RayTracingSceneTlasShaderAccessMode::Descriptor))
	{
		return;
	}

	parameters->PerFrame = context.RuntimeServices.PerFrame;
	parameters->PerView = context.Frame.mainView.perViewData;
	parameters->PerTemporal = context.Frame.mainView.perTemporalData;
	parameters->ViewLighting = context.Frame.sceneGpuData->Lighting.Constants;
	parameters->Sky = MakeSkyUniformData(context.Frame.sceneData.sky);
	parameters->MaterialTextureTable = context.Frame.sceneData.materialTextureTable.Binding;
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
	    context.Frame.sceneGpuData->RayTracing.InstanceCount,
	    context.Frame.sceneGpuData->RayTracing.MaterialCount);

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

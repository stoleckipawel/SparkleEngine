#include "../../PCH.h"

#include "Scene/GpuScene/RenderSceneGpuBindings.h"
#include "Passes/RayTracing/RestirIndirectResolvePass.h"

#include "FrameGraph/Execution/PassCommandContext.h"
#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "RayTracing/RayTracingShaderFeatureFlags.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"
#include "RayTracing/Effects/RestirLighting/RestirIndirectLightingSettings.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassData.h"
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
	builder.Uniform("View", &RestirIndirectResolvePassParameters::View, ShaderStageVisibility::Compute);
	builder.Uniform("ViewCamera", &RestirIndirectResolvePassParameters::ViewCamera, ShaderStageVisibility::Compute);
	builder.Uniform("ViewTemporal", &RestirIndirectResolvePassParameters::ViewTemporal, ShaderStageVisibility::Compute);
	builder.Uniform("SceneLighting", &RestirIndirectResolvePassParameters::SceneLighting, ShaderStageVisibility::Compute);
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
	builder.ReadBuffer("MorphTargetDeltas", &RestirIndirectResolvePassParameters::MorphTargetDeltas, ShaderStageVisibility::Compute);
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

RestirIndirectResolvePass::RestirIndirectResolvePass(const ComputePassPipelineRuntime& runtime) noexcept :
    m_runtime(runtime)
{
}

const RestirIndirectResolvePass::ParameterMetadata& RestirIndirectResolvePass::GetParameterMetadata() noexcept
{
	return ComputePassOperations::BuildParameterMetadata<RestirIndirectResolvePass>();
}

const RenderPassDefinition& RestirIndirectResolvePass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassOperations::BuildDefinition(
	    PassName,
	    RendererShaderPackages::RestirIndirectResolve,
	    L"RestirIndirectResolve_BindingLayout",
	    L"RestirIndirectResolve_Pipeline",
	    RayTracingShaderFeatureFlags::InlineRayQuery);
	return definition;
}

void RestirIndirectResolvePass::Execute(
    PassCommandContext& context,
    ParameterInstance& parameters,
    std::uint32_t outputWidth,
    std::uint32_t outputHeight) const
{
	ComputePassOperations::DispatchSized<RestirIndirectResolvePass>(context, m_runtime, parameters, outputWidth, outputHeight);
}

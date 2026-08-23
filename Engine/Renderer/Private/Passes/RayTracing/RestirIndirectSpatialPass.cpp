#include "../../PCH.h"

#include "Scene/GpuScene/RenderSceneGpuBindings.h"
#include "Passes/RayTracing/RestirIndirectSpatialPass.h"

#include "FrameGraph/Execution/PassCommandContext.h"
#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "RayTracing/RayTracingShaderFeatureFlags.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"
#include "RayTracing/Effects/RestirLighting/RestirIndirectLightingSettings.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassData.h"
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
	builder.Uniform("Frame", &RestirIndirectSpatialPassParameters::Frame, ShaderStageVisibility::Compute);
	builder.Uniform("View", &RestirIndirectSpatialPassParameters::View, ShaderStageVisibility::Compute);
	builder.Uniform("ViewCamera", &RestirIndirectSpatialPassParameters::ViewCamera, ShaderStageVisibility::Compute);
	builder.Uniform("ViewTemporal", &RestirIndirectSpatialPassParameters::ViewTemporal, ShaderStageVisibility::Compute);
	builder.Uniform("SceneLighting", &RestirIndirectSpatialPassParameters::SceneLighting, ShaderStageVisibility::Compute);
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
	builder.ReadBuffer("MorphTargetDeltas", &RestirIndirectSpatialPassParameters::MorphTargetDeltas, ShaderStageVisibility::Compute);
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
	builder.ReadBuffer("MorphWeights", &RestirIndirectSpatialPassParameters::MorphWeights, ShaderStageVisibility::Compute);
	builder.ReadBuffer("DirectionalLights", &RestirIndirectSpatialPassParameters::DirectionalLights, ShaderStageVisibility::Compute);
	builder.ReadBuffer("PointLights", &RestirIndirectSpatialPassParameters::PointLights, ShaderStageVisibility::Compute);
	builder.ReadBuffer("SpotLights", &RestirIndirectSpatialPassParameters::SpotLights, ShaderStageVisibility::Compute);
	builder.ReadBuffer("RectLights", &RestirIndirectSpatialPassParameters::RectLights, ShaderStageVisibility::Compute);
	builder.ReadTexture("MaterialTextureTable", &RestirIndirectSpatialPassParameters::MaterialTextureTable, ShaderStageVisibility::Compute);
	builder.Sampler("MaterialTextureSampler", &RestirIndirectSpatialPassParameters::MaterialTextureSampler, ShaderStageVisibility::Compute);
}

RestirIndirectSpatialPass::RestirIndirectSpatialPass(const ComputePassPipelineRuntime& runtime) noexcept :
    m_runtime(runtime)
{
}

const RestirIndirectSpatialPass::ParameterMetadata& RestirIndirectSpatialPass::GetParameterMetadata() noexcept
{
	return ComputePassOperations::BuildParameterMetadata<RestirIndirectSpatialPass>();
}

const RenderPassDefinition& RestirIndirectSpatialPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassOperations::BuildDefinition(
	    PassName,
	    RendererShaderPackages::RestirIndirectSpatial,
	    L"RestirIndirectSpatial_BindingLayout",
	    L"RestirIndirectSpatial_Pipeline",
	    RayTracingShaderFeatureFlags::InlineRayQuery);
	return definition;
}

void RestirIndirectSpatialPass::Execute(
    PassCommandContext& context,
    ParameterInstance& parameters,
    std::uint32_t outputWidth,
    std::uint32_t outputHeight) const
{
	ComputePassOperations::DispatchSized<RestirIndirectSpatialPass>(context, m_runtime, parameters, outputWidth, outputHeight);
}

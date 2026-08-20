#include "../../PCH.h"

#include "Passes/RayTracing/PathTracedDirectLightingPass.h"

#include "FrameGraph/Execution/PassCommandContext.h"
#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "RayTracing/RayTracingShaderFeatureFlags.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

void PathTracedDirectLightingPassParameters::Describe(ShaderParameterStructBuilder<PathTracedDirectLightingPassParameters>& builder)
{
	builder.RWTexture("DirectDiffuse", &PathTracedDirectLightingPassParameters::DirectDiffuse, ShaderStageVisibility::Compute);
	builder.RWTexture("DirectSpecular", &PathTracedDirectLightingPassParameters::DirectSpecular, ShaderStageVisibility::Compute);
	builder.RWTexture("DirectSubsurface", &PathTracedDirectLightingPassParameters::DirectSubsurface, ShaderStageVisibility::Compute);
	builder.AccelerationStructure("SceneTlas", &PathTracedDirectLightingPassParameters::SceneTlas, ShaderStageVisibility::Compute);
	builder.Uniform("Frame", &PathTracedDirectLightingPassParameters::Frame, ShaderStageVisibility::Compute);
	builder.Uniform("View", &PathTracedDirectLightingPassParameters::View, ShaderStageVisibility::Compute);
	builder.Uniform("ViewCamera", &PathTracedDirectLightingPassParameters::ViewCamera, ShaderStageVisibility::Compute);
	builder.Uniform("ViewTemporal", &PathTracedDirectLightingPassParameters::ViewTemporal, ShaderStageVisibility::Compute);
	builder.Uniform("SceneLighting", &PathTracedDirectLightingPassParameters::SceneLighting, ShaderStageVisibility::Compute);
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

PathTracedDirectLightingPass::PathTracedDirectLightingPass(const ComputePassPipelineRuntime& runtime) noexcept :
    m_runtime(runtime)
{
}

const PathTracedDirectLightingPass::ParameterMetadata& PathTracedDirectLightingPass::GetParameterMetadata() noexcept
{
	return ComputePassOperations::BuildParameterMetadata<PathTracedDirectLightingPass>();
}
const RenderPassDefinition& PathTracedDirectLightingPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassOperations::BuildDefinition(
	    PassName,
	    RendererShaderPackages::PathTracedDirectLighting,
	    L"PathTracedDirectLighting_BindingLayout",
	    L"PathTracedDirectLighting_Pipeline",
	    RayTracingShaderFeatureFlags::DescriptorRayQuery);
	return definition;
}

void PathTracedDirectLightingPass::Execute(
    PassCommandContext& context,
    ParameterInstance& parameters,
    std::uint32_t outputWidth,
    std::uint32_t outputHeight) const
{
	ComputePassOperations::DispatchSized<PathTracedDirectLightingPass>(context, m_runtime, parameters, outputWidth, outputHeight);
}

#include "../../PCH.h"

#include "Passes/RayTracing/RaytracedGBufferPass.h"

#include "FrameGraph/Execution/PassCommandContext.h"
#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "RayTracing/RayTracingShaderFeatureFlags.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

void RaytracedGBufferPassParameters::Describe(ShaderParameterStructBuilder<RaytracedGBufferPassParameters>& builder)
{
	builder.RWTexture("GBufferBaseColor", &RaytracedGBufferPassParameters::GBufferBaseColor, ShaderStageVisibility::Compute);
	builder.RWTexture("GBufferNormal", &RaytracedGBufferPassParameters::GBufferNormal, ShaderStageVisibility::Compute);
	builder.RWTexture("GBufferMaterial", &RaytracedGBufferPassParameters::GBufferMaterial, ShaderStageVisibility::Compute);
	builder.RWTexture("GBufferEmissive", &RaytracedGBufferPassParameters::GBufferEmissive, ShaderStageVisibility::Compute);
	builder.RWTexture("GBufferSubsurface", &RaytracedGBufferPassParameters::GBufferSubsurface, ShaderStageVisibility::Compute);
	builder.RWTexture("GBufferDeviceZ", &RaytracedGBufferPassParameters::GBufferDeviceZ, ShaderStageVisibility::Compute);
	builder.RWTexture("GBufferMotionVector", &RaytracedGBufferPassParameters::GBufferMotionVector, ShaderStageVisibility::Compute);
	builder.AccelerationStructure("SceneTlas", &RaytracedGBufferPassParameters::SceneTlas, ShaderStageVisibility::Compute);
	builder.Uniform("View", &RaytracedGBufferPassParameters::View, ShaderStageVisibility::Compute);
	builder.Uniform("ViewCamera", &RaytracedGBufferPassParameters::ViewCamera, ShaderStageVisibility::Compute);
	builder.Uniform("ViewTemporal", &RaytracedGBufferPassParameters::ViewTemporal, ShaderStageVisibility::Compute);
	builder.Uniform(
	    "RaytracedGBufferConstants",
	    &RaytracedGBufferPassParameters::RaytracedGBufferConstants,
	    ShaderStageVisibility::Compute);
	builder.ReadBuffer("RayTracingHitVertices", &RaytracedGBufferPassParameters::RayTracingHitVertices, ShaderStageVisibility::Compute);
	builder.ReadBuffer("MorphTargetDeltas", &RaytracedGBufferPassParameters::MorphTargetDeltas, ShaderStageVisibility::Compute);
	builder.ReadBuffer("RayTracingHitIndices", &RaytracedGBufferPassParameters::RayTracingHitIndices, ShaderStageVisibility::Compute);
	builder.ReadBuffer("RayTracingHitInstances", &RaytracedGBufferPassParameters::RayTracingHitInstances, ShaderStageVisibility::Compute);
	builder.ReadBuffer("RayTracingHitMaterials", &RaytracedGBufferPassParameters::RayTracingHitMaterials, ShaderStageVisibility::Compute);
	builder.ReadBuffer("MeshInstances", &RaytracedGBufferPassParameters::MeshInstances, ShaderStageVisibility::Compute);
	builder.ReadBuffer("SkinInfluences", &RaytracedGBufferPassParameters::SkinInfluences, ShaderStageVisibility::Compute);
	builder.ReadBuffer("JointMatrices", &RaytracedGBufferPassParameters::JointMatrices, ShaderStageVisibility::Compute);
	builder.ReadBuffer("PreviousJointMatrices", &RaytracedGBufferPassParameters::PreviousJointMatrices, ShaderStageVisibility::Compute);
	builder.ReadBuffer("MorphWeights", &RaytracedGBufferPassParameters::MorphWeights, ShaderStageVisibility::Compute);
	builder.ReadBuffer("PreviousMorphWeights", &RaytracedGBufferPassParameters::PreviousMorphWeights, ShaderStageVisibility::Compute);
	builder.ReadTexture("MaterialTextureTable", &RaytracedGBufferPassParameters::MaterialTextureTable, ShaderStageVisibility::Compute);
	builder.Sampler("MaterialTextureSampler", &RaytracedGBufferPassParameters::MaterialTextureSampler, ShaderStageVisibility::Compute);
}

RaytracedGBufferPass::RaytracedGBufferPass(const ComputePassPipelineRuntime& runtime) noexcept :
    m_runtime(runtime)
{
}

const RaytracedGBufferPass::ParameterMetadata& RaytracedGBufferPass::GetParameterMetadata() noexcept
{
	return ComputePassOperations::BuildParameterMetadata<RaytracedGBufferPass>();
}

const RenderPassDefinition& RaytracedGBufferPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassOperations::BuildDefinition(
	    PassName,
	    RendererShaderPackages::RaytracedGBuffer,
	    L"RaytracedGBuffer_BindingLayout",
	    L"RaytracedGBuffer_Pipeline",
	    RayTracingShaderFeatureFlags::InlineRayQuery);
	return definition;
}

void RaytracedGBufferPass::Execute(
    PassCommandContext& context,
    ParameterInstance& parameters,
    std::uint32_t outputWidth,
    std::uint32_t outputHeight) const
{
	ComputePassOperations::DispatchSized<RaytracedGBufferPass>(context, m_runtime, parameters, outputWidth, outputHeight);
}

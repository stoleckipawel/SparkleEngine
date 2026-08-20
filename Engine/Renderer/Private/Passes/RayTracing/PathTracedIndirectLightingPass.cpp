#include "../../PCH.h"

#include "Scene/GpuScene/RenderSceneGpuBindings.h"
#include "Passes/RayTracing/PathTracedIndirectLightingPass.h"

#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeContext.h"
#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassData.h"
#include "RayTracing/RayTracingShaderFeatureFlags.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "RayTracing/Effects/PathTracedLighting/PathTracedLightingSettings.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"

void PathTracedIndirectLightingPassParameters::Describe(ShaderParameterStructBuilder<PathTracedIndirectLightingPassParameters>& builder)
{
	builder.AccelerationStructure("SceneTlas", &PathTracedIndirectLightingPassParameters::SceneTlas, ShaderStageVisibility::Compute);
	builder.Uniform("Frame", &PathTracedIndirectLightingPassParameters::Frame, ShaderStageVisibility::Compute);
	builder.Uniform("View", &PathTracedIndirectLightingPassParameters::View, ShaderStageVisibility::Compute);
	builder.Uniform("ViewCamera", &PathTracedIndirectLightingPassParameters::ViewCamera, ShaderStageVisibility::Compute);
	builder.Uniform("ViewTemporal", &PathTracedIndirectLightingPassParameters::ViewTemporal, ShaderStageVisibility::Compute);
	builder.Uniform("SceneLighting", &PathTracedIndirectLightingPassParameters::SceneLighting, ShaderStageVisibility::Compute);
	builder.Uniform("RayTracedShadows", &PathTracedIndirectLightingPassParameters::RayTracedShadows, ShaderStageVisibility::Compute);
	builder.Uniform("Sky", &PathTracedIndirectLightingPassParameters::Sky, ShaderStageVisibility::Compute);
	builder.ReadTexture("GBufferBaseColor", &PathTracedIndirectLightingPassParameters::GBufferBaseColor, ShaderStageVisibility::Compute);
	builder.ReadTexture("GBufferNormal", &PathTracedIndirectLightingPassParameters::GBufferNormal, ShaderStageVisibility::Compute);
	builder.ReadTexture("GBufferMaterial", &PathTracedIndirectLightingPassParameters::GBufferMaterial, ShaderStageVisibility::Compute);
	builder.ReadTexture("SceneDepth", &PathTracedIndirectLightingPassParameters::SceneDepth, ShaderStageVisibility::Compute);
	builder.ReadTexture("SkyTexture", &PathTracedIndirectLightingPassParameters::SkyTexture, ShaderStageVisibility::Compute);
	builder.Sampler("SamplerLinearClamp", &PathTracedIndirectLightingPassParameters::SamplerLinearClamp, ShaderStageVisibility::Compute);
	builder.ReadBuffer(
	    "RayTracingHitVertices",
	    &PathTracedIndirectLightingPassParameters::RayTracingHitVertices,
	    ShaderStageVisibility::Compute);
	builder.ReadBuffer("MorphTargetDeltas", &PathTracedIndirectLightingPassParameters::MorphTargetDeltas, ShaderStageVisibility::Compute);
	builder.ReadBuffer(
	    "RayTracingHitIndices",
	    &PathTracedIndirectLightingPassParameters::RayTracingHitIndices,
	    ShaderStageVisibility::Compute);
	builder.ReadBuffer(
	    "RayTracingHitInstances",
	    &PathTracedIndirectLightingPassParameters::RayTracingHitInstances,
	    ShaderStageVisibility::Compute);
	builder.ReadBuffer(
	    "RayTracingHitMaterials",
	    &PathTracedIndirectLightingPassParameters::RayTracingHitMaterials,
	    ShaderStageVisibility::Compute);
	builder.ReadBuffer("MeshInstances", &PathTracedIndirectLightingPassParameters::MeshInstances, ShaderStageVisibility::Compute);
	builder.ReadBuffer("SkinInfluences", &PathTracedIndirectLightingPassParameters::SkinInfluences, ShaderStageVisibility::Compute);
	builder.ReadBuffer("JointMatrices", &PathTracedIndirectLightingPassParameters::JointMatrices, ShaderStageVisibility::Compute);
	builder.ReadBuffer("MorphWeights", &PathTracedIndirectLightingPassParameters::MorphWeights, ShaderStageVisibility::Compute);
	builder.ReadBuffer("DirectionalLights", &PathTracedIndirectLightingPassParameters::DirectionalLights, ShaderStageVisibility::Compute);
	builder.ReadBuffer("PointLights", &PathTracedIndirectLightingPassParameters::PointLights, ShaderStageVisibility::Compute);
	builder.ReadBuffer("SpotLights", &PathTracedIndirectLightingPassParameters::SpotLights, ShaderStageVisibility::Compute);
	builder.ReadBuffer("RectLights", &PathTracedIndirectLightingPassParameters::RectLights, ShaderStageVisibility::Compute);
	builder.ReadTexture(
	    "MaterialTextureTable",
	    &PathTracedIndirectLightingPassParameters::MaterialTextureTable,
	    ShaderStageVisibility::Compute);
	builder.Sampler(
	    "MaterialTextureSampler",
	    &PathTracedIndirectLightingPassParameters::MaterialTextureSampler,
	    ShaderStageVisibility::Compute);
	builder.RWTexture("IndirectDiffuse", &PathTracedIndirectLightingPassParameters::IndirectDiffuse, ShaderStageVisibility::Compute);
	builder.RWTexture("IndirectSpecular", &PathTracedIndirectLightingPassParameters::IndirectSpecular, ShaderStageVisibility::Compute);
	builder.Uniform(
	    "PathTracedLightingConstants",
	    &PathTracedIndirectLightingPassParameters::PathTracedLightingConstants,
	    ShaderStageVisibility::Compute);
}

PathTracedIndirectLightingPass::PathTracedIndirectLightingPass(const ComputePassPipelineRuntime& runtime) noexcept :
    m_runtime(runtime)
{
}

const PathTracedIndirectLightingPass::ParameterMetadata& PathTracedIndirectLightingPass::GetParameterMetadata() noexcept
{
	return ComputePassOperations::BuildParameterMetadata<PathTracedIndirectLightingPass>();
}

const RenderPassDefinition& PathTracedIndirectLightingPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassOperations::BuildDefinition(
	    PassName,
	    RendererShaderPackages::PathTracedIndirectLighting,
	    L"PathTracedIndirectLighting_BindingLayout",
	    L"PathTracedIndirectLighting_Pipeline",
	    RayTracingShaderFeatureFlags::DescriptorRayQuery);
	return definition;
}

void PathTracedIndirectLightingPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	parameters->Frame = context.Runtime.Frame;
	parameters->View = context.Frame.view.uniform;
	parameters->ViewCamera = context.Frame.view.cameraUniform;
	parameters->ViewTemporal = context.Frame.view.temporalUniform;
	parameters->SceneLighting = context.Frame.preparedScene.gpuBindings->Lighting.Uniform;
	parameters->Sky = MakeSkyUniformData(context.Frame.preparedScene.sky);
	parameters->MaterialTextureTable = context.Frame.preparedScene.materialTextureTable.Binding;
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
	    context.Runtime.RayTracing,
	    context.Frame.preparedScene.gpuBindings->RayTracing.InstanceCount > 0u,
	    context.Frame.preparedScene.gpuBindings->RayTracing.InstanceCount,
	    context.Frame.preparedScene.gpuBindings->RayTracing.MaterialCount);
	const PathTracedLightingSettings settings = BuildPathTracedLightingSettings();
	parameters->PathTracedLightingConstants = PathTracedLightingUniformData{
	    .SamplesPerPixel = settings.SamplesPerPixel,
	    .BounceCount = settings.BounceCount,
	    .NormalBias = settings.NormalBias,
	    .MaxDistance = settings.MaxDistance};
	ComputePassOperations::DispatchSized<PathTracedIndirectLightingPass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.view.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.view.viewport.Height));
}

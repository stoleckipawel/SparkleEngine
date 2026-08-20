#include "../../PCH.h"

#include "Scene/GpuScene/RenderSceneGpuBindings.h"
#include "Passes/RayTracing/PathTracedDirectLightingPass.h"

#include "Frame/Core/FrameContext.h"
#include "View/RenderView.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeContext.h"
#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "RayTracing/RayTracingShaderFeatureFlags.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "RayTracing/Effects/PathTracedLighting/PathTracedLightingSettings.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassData.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"

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

void PathTracedDirectLightingPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	parameters->Frame = context.Runtime.Frame;
	parameters->View = context.Frame.view.uniform;
	parameters->ViewCamera = context.Frame.view.cameraUniform;
	parameters->ViewTemporal = context.Frame.view.temporalUniform;
	parameters->SceneLighting = context.Frame.preparedScene.gpuBindings->Lighting.Uniform;
	parameters->MaterialTextureSampler = RhiSamplerDesc{
	    .MinMagFilter = RhiSamplerMinMagFilter::Linear,
	    .MipFilter = RhiSamplerMipFilter::Linear,
	    .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Wrap),
	    .MaxAnisotropy = RhiSamplerAnisotropy::X1};

	parameters->MaterialTextureTable = context.Frame.preparedScene.materialTextureTable.Binding;
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
	ComputePassOperations::DispatchSized<PathTracedDirectLightingPass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.view.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.view.viewport.Height));
}

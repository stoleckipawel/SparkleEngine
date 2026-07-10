#include "../../PCH.h"
#include "Passes/RayTracing/PathTracedDirectLightingPass.h"

#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Passes/Bindings/LightingPassBinding.h"
#include "Passes/Bindings/RayTracedShadowPassBinding.h"
#include "Passes/Bindings/RayTracingScenePassBinding.h"
#include "Passes/Core/ComputePassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "RayTracing/RayTracingShaderFeatureFlags.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "RayTracing/Effects/PathTracedLighting/PathTracedLightingSettings.h"
#include "RayTracing/RayTracingPassCapabilityQuery.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

void PathTracedDirectLightingPassParameters::Describe(ShaderParameterStructBuilder<PathTracedDirectLightingPassParameters>& builder)
{
	builder.RWTexture("DirectDiffuse", &PathTracedDirectLightingPassParameters::DirectDiffuse, ShaderStageVisibility::Compute);
	builder.RWTexture("DirectSpecular", &PathTracedDirectLightingPassParameters::DirectSpecular, ShaderStageVisibility::Compute);
	builder.RWTexture("DirectSubsurface", &PathTracedDirectLightingPassParameters::DirectSubsurface, ShaderStageVisibility::Compute);
	builder.AccelerationStructure("SceneTlas", &PathTracedDirectLightingPassParameters::SceneTlas, ShaderStageVisibility::Compute);
	builder.Uniform("PerFrame", &PathTracedDirectLightingPassParameters::PerFrame, ShaderStageVisibility::Compute);
	builder.Uniform("PerView", &PathTracedDirectLightingPassParameters::PerView, ShaderStageVisibility::Compute);
	builder.Uniform("ViewLighting", &PathTracedDirectLightingPassParameters::ViewLighting, ShaderStageVisibility::Compute);
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

PathTracedDirectLightingPass::PathTracedDirectLightingPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const PathTracedDirectLightingPass::ParameterMetadata& PathTracedDirectLightingPass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<PathTracedDirectLightingPass>();
}
const RenderPassDefinition& PathTracedDirectLightingPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    RendererShaderPackages::PathTracedDirectLighting,
	    L"PathTracedDirectLighting_BindingLayout",
	    L"PathTracedDirectLighting_PipelineState",
	    RayTracingShaderFeatureFlags::DescriptorRayQuery);
	return definition;
}

void PathTracedDirectLightingPass::DeclareResources(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer,
    FrameGraphAccelerationStructureHandle sceneTlas,
    ParameterInstance& parameters)
{
	parameters->DirectDiffuse = builder.CreateUAV(lighting.DirectDiffuse);
	parameters->DirectSpecular = builder.CreateUAV(lighting.DirectSpecular);
	parameters->DirectSubsurface = builder.CreateUAV(lighting.DirectSubsurface);
	(void) RayTracingScenePassBinding::BindSceneTlas(builder, sceneTlas, RayTracingSceneTlasShaderAccessMode::Descriptor, parameters);
	parameters->GBufferBaseColor = builder.CreateSRV(gbuffer.BaseColor);
	parameters->GBufferNormal = builder.CreateSRV(gbuffer.Normal);
	parameters->GBufferMaterial = builder.CreateSRV(gbuffer.Material);
	parameters->GBufferSubsurface = builder.CreateSRV(gbuffer.Subsurface);
	parameters->SceneDepth = builder.CreateSRV(sceneTargets.SceneDepth);
}

void PathTracedDirectLightingPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	const RayTracingPassCapabilities capabilities = RayTracingPassCapabilityQuery::Build(context.Frame, context.RuntimeServices.RayTracing);
	if (!capabilities.InlineRayQueryAvailable ||
	    !RayTracingScenePassBinding::CanUseSceneTlas(capabilities, RayTracingSceneTlasShaderAccessMode::Descriptor))
	{
		return;
	}
	parameters->PerFrame = context.RuntimeServices.PerFrame;
	parameters->PerView = context.Frame.mainView.perViewData;
	LightingPassBinding::SetParameters(parameters, context.Frame);
	RayTracedShadowPassBinding::SetRayQueryParameters(
	    parameters,
	    context.Frame,
	    context.RuntimeServices,
	    context.Frame.rayTracingScene.HasTraceableInstances());

	const PathTracedLightingSettings settings = BuildPathTracedLightingSettings();
	parameters->PathTracedLightingConstants = PathTracedLightingUniformData{
	    .SamplesPerPixel = settings.SamplesPerPixel,
	    .BounceCount = settings.BounceCount,
	    .NormalBias = settings.NormalBias,
	    .MaxDistance = settings.MaxDistance};
	ComputePassUtilities::DispatchSized<PathTracedDirectLightingPass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
}

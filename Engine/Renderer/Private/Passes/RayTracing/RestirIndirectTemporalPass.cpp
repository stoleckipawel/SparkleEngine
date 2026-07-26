#include "../../PCH.h"
#include "Passes/RayTracing/RestirIndirectTemporalPass.h"

#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
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
	builder.ReadBuffer(
	    "MorphTargetDeltas",
	    &RestirIndirectTemporalPassParameters::MorphTargetDeltas,
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
	builder.ReadBuffer("MorphWeights", &RestirIndirectTemporalPassParameters::MorphWeights, ShaderStageVisibility::Compute);
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

RestirIndirectTemporalPass::RestirIndirectTemporalPass(
    const ComputePassPipelineRuntime& runtime) noexcept :
	m_runtime(runtime)
{
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

void RestirIndirectTemporalPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
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

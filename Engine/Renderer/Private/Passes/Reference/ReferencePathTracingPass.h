#pragma once

#include "Frame/Reference/ReferenceRenderTargets.h"
#include "Passes/Bindings/EnvironmentMapPassBinding.h"
#include "RayTracing/Effects/ReferencePathTracing/ReferencePathTracingUniformData.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowUniformData.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"
#include "RHI/Public/Descriptors/RhiDescriptorHandles.h"
#include "RHI/Public/Resources/RenderConstantBufferData.h"
#include "RHI/Public/Resources/RenderViewLightingData.h"
#include "SceneData/MaterialTextureTableCapability.h"

#include <cstdint>

class FrameGraphBuilder;
struct ComputePassPipelineRuntime;
struct FrameContext;
struct PassExecutionContext;
struct PassRuntimeServices;
struct RenderPassDefinition;
struct RenderViewData;

struct ReferencePathTracingPassParameters
{
	ShaderRWTexture2D<void> ReferenceSceneColorTexture;
	ShaderRWTexture2D<void> ReferenceDirectTexture;
	ShaderRWTexture2D<void> ReferenceIndirectDiffuseTexture;
	ShaderRWTexture2D<void> ReferenceIndirectSpecularTexture;
	ShaderRWTexture2D<void> ReferencePrimaryDeviceDepthTexture;
	ShaderRWTexture2D<void> ReferencePrimaryNormalTexture;
	ShaderRWTexture2D<void> ReferencePrimaryDiffuseAlbedoTexture;
	ShaderRWTexture2D<void> ReferencePrimarySpecularAlbedoTexture;
	ShaderRWTexture2D<void> ReferencePrimaryMaterialGuideTexture;
	ShaderRWTexture2D<void> ReferencePrimaryPathSampleGuideTexture;
	ShaderAccelerationStructure SceneTlas;
	ShaderUniform<PerFrameConstantBufferData> PerFrame;
	ShaderUniform<PerViewConstantBufferData> PerView;
	ShaderUniform<ViewLightingData> ViewLighting;
	ShaderUniform<RayTracedShadowUniformData> RayTracedShadows;
	ShaderUniform<ReferencePathTracingUniformData> ReferencePathTracingConstants;
	ShaderTexture2DSRV SkyTexture;
	ShaderSamplerSet SamplerLinearClamp;
	ShaderBufferSRV RayTracingHitVertices;
	ShaderBufferSRV RayTracingHitIndices;
	ShaderBufferSRV RayTracingHitInstances;
	ShaderBufferSRV RayTracingHitMaterials;
	ShaderBufferSRV MeshInstances;
	ShaderBufferSRV DirectionalLights;
	ShaderBufferSRV PointLights;
	ShaderBufferSRV SpotLights;
	ShaderBufferSRV RectLights;
	ShaderTexture2DTableSRV<MaterialTextureTableFixedCapacity> MaterialTextureTable;
	ShaderSamplerSet MaterialTextureSampler;

	static void Describe(ShaderParameterStructBuilder<ReferencePathTracingPassParameters>& builder)
	{
		builder.RWTexture(
		    "ReferenceSceneColorTexture",
		    &ReferencePathTracingPassParameters::ReferenceSceneColorTexture,
		    ShaderStageVisibility::Compute);
		builder.RWTexture(
		    "ReferenceDirectTexture",
		    &ReferencePathTracingPassParameters::ReferenceDirectTexture,
		    ShaderStageVisibility::Compute);
		builder.RWTexture(
		    "ReferenceIndirectDiffuseTexture",
		    &ReferencePathTracingPassParameters::ReferenceIndirectDiffuseTexture,
		    ShaderStageVisibility::Compute);
		builder.RWTexture(
		    "ReferenceIndirectSpecularTexture",
		    &ReferencePathTracingPassParameters::ReferenceIndirectSpecularTexture,
		    ShaderStageVisibility::Compute);
		builder.RWTexture(
		    "ReferencePrimaryDeviceDepthTexture",
		    &ReferencePathTracingPassParameters::ReferencePrimaryDeviceDepthTexture,
		    ShaderStageVisibility::Compute);
		builder.RWTexture(
		    "ReferencePrimaryNormalTexture",
		    &ReferencePathTracingPassParameters::ReferencePrimaryNormalTexture,
		    ShaderStageVisibility::Compute);
		builder.RWTexture(
		    "ReferencePrimaryDiffuseAlbedoTexture",
		    &ReferencePathTracingPassParameters::ReferencePrimaryDiffuseAlbedoTexture,
		    ShaderStageVisibility::Compute);
		builder.RWTexture(
		    "ReferencePrimarySpecularAlbedoTexture",
		    &ReferencePathTracingPassParameters::ReferencePrimarySpecularAlbedoTexture,
		    ShaderStageVisibility::Compute);
		builder.RWTexture(
		    "ReferencePrimaryMaterialGuideTexture",
		    &ReferencePathTracingPassParameters::ReferencePrimaryMaterialGuideTexture,
		    ShaderStageVisibility::Compute);
		builder.RWTexture(
		    "ReferencePrimaryPathSampleGuideTexture",
		    &ReferencePathTracingPassParameters::ReferencePrimaryPathSampleGuideTexture,
		    ShaderStageVisibility::Compute);
		builder.AccelerationStructure("SceneTlas", &ReferencePathTracingPassParameters::SceneTlas, ShaderStageVisibility::Compute);
		builder.Uniform("PerFrame", &ReferencePathTracingPassParameters::PerFrame, ShaderStageVisibility::Compute);
		builder.Uniform("PerView", &ReferencePathTracingPassParameters::PerView, ShaderStageVisibility::Compute);
		builder.Uniform("ViewLighting", &ReferencePathTracingPassParameters::ViewLighting, ShaderStageVisibility::Compute);
		builder.Uniform("RayTracedShadows", &ReferencePathTracingPassParameters::RayTracedShadows, ShaderStageVisibility::Compute);
		builder.Uniform(
		    "ReferencePathTracingConstants",
		    &ReferencePathTracingPassParameters::ReferencePathTracingConstants,
		    ShaderStageVisibility::Compute);
		builder.ReadTexture("SkyTexture", &ReferencePathTracingPassParameters::SkyTexture, ShaderStageVisibility::Compute);
		builder.Sampler("SamplerLinearClamp", &ReferencePathTracingPassParameters::SamplerLinearClamp, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RayTracingHitVertices", &ReferencePathTracingPassParameters::RayTracingHitVertices, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RayTracingHitIndices", &ReferencePathTracingPassParameters::RayTracingHitIndices, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RayTracingHitInstances", &ReferencePathTracingPassParameters::RayTracingHitInstances, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RayTracingHitMaterials", &ReferencePathTracingPassParameters::RayTracingHitMaterials, ShaderStageVisibility::Compute);
		builder.ReadBuffer("MeshInstances", &ReferencePathTracingPassParameters::MeshInstances, ShaderStageVisibility::Compute);
		builder.ReadBuffer("DirectionalLights", &ReferencePathTracingPassParameters::DirectionalLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("PointLights", &ReferencePathTracingPassParameters::PointLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("SpotLights", &ReferencePathTracingPassParameters::SpotLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RectLights", &ReferencePathTracingPassParameters::RectLights, ShaderStageVisibility::Compute);
		builder.ReadTexture("MaterialTextureTable", &ReferencePathTracingPassParameters::MaterialTextureTable, ShaderStageVisibility::Compute);
		builder.Sampler("MaterialTextureSampler", &ReferencePathTracingPassParameters::MaterialTextureSampler, ShaderStageVisibility::Compute);
	}
};

class ReferencePathTracingPass final
{
  public:
	static constexpr const char* PassName = "ReferencePathTracing";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	using Parameters = ReferencePathTracingPassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

	explicit ReferencePathTracingPass(const ComputePassPipelineRuntime& runtime) noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	static void DeclareResources(
	    FrameGraphBuilder& builder,
	    const ReferenceRenderTargets& targets,
	    FrameGraphAccelerationStructureHandle sceneTlas,
	    ParameterInstance& parameters);
	void Execute(PassExecutionContext& context, ParameterInstance& parameters) const;

  private:
	void SetParameters(
	    ParameterInstance& parameters,
	    const FrameContext& frame,
	    const RenderViewData& viewData,
	    const PassRuntimeServices& passRuntimeServices,
	    RhiDescriptorTableBinding environmentTextureBinding) const;

	const ComputePassPipelineRuntime& m_runtime;
	mutable EnvironmentMapPassBinding m_environmentMapBinding;
};

#pragma once

#include "Frame/Targets/FrameRenderTargets.h"
#include "Passes/Bindings/EnvironmentMapPassBinding.h"
#include "RayTracing/Effects/IndirectDiffuse/IndirectDiffuseUniformData.h"
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

struct IndirectDiffusePassParameters
{
	ShaderRWTexture2D<void> IndirectDiffuseTexture;
	ShaderRWTexture2D<void> IndirectDiffuseDemodulatedRadiance;
	ShaderRWTexture2D<void> IndirectDiffuseAlbedo;
	ShaderRWTexture2D<void> IndirectSpecularAlbedo;
	ShaderRWTexture2D<void> IndirectMaterialGuide;
	ShaderRWTexture2D<void> IndirectDiffuseSampleGuide;
	ShaderAccelerationStructure SceneTlas;
	ShaderUniform<PerFrameConstantBufferData> PerFrame;
	ShaderUniform<PerViewConstantBufferData> PerView;
	ShaderUniform<ViewLightingData> ViewLighting;
	ShaderUniform<IndirectDiffuseUniformData> IndirectDiffuseConstants;
	ShaderTexture2D<void> GBufferBaseColor;
	ShaderTexture2D<void> GBufferNormal;
	ShaderTexture2D<void> GBufferMaterial;
	ShaderTexture2D<void> GBufferDeviceZ;
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

	static void Describe(ShaderParameterStructBuilder<IndirectDiffusePassParameters>& builder)
	{
		builder.RWTexture("IndirectDiffuseTexture", &IndirectDiffusePassParameters::IndirectDiffuseTexture, ShaderStageVisibility::Compute);
		builder.RWTexture(
		    "IndirectDiffuseDemodulatedRadiance",
		    &IndirectDiffusePassParameters::IndirectDiffuseDemodulatedRadiance,
		    ShaderStageVisibility::Compute);
		builder.RWTexture(
		    "IndirectDiffuseAlbedo",
		    &IndirectDiffusePassParameters::IndirectDiffuseAlbedo,
		    ShaderStageVisibility::Compute);
		builder.RWTexture(
		    "IndirectSpecularAlbedo",
		    &IndirectDiffusePassParameters::IndirectSpecularAlbedo,
		    ShaderStageVisibility::Compute);
		builder.RWTexture(
		    "IndirectMaterialGuide",
		    &IndirectDiffusePassParameters::IndirectMaterialGuide,
		    ShaderStageVisibility::Compute);
		builder.RWTexture(
		    "IndirectDiffuseSampleGuide",
		    &IndirectDiffusePassParameters::IndirectDiffuseSampleGuide,
		    ShaderStageVisibility::Compute);
		builder.AccelerationStructure("SceneTlas", &IndirectDiffusePassParameters::SceneTlas, ShaderStageVisibility::Compute);
		builder.Uniform("PerFrame", &IndirectDiffusePassParameters::PerFrame, ShaderStageVisibility::Compute);
		builder.Uniform("PerView", &IndirectDiffusePassParameters::PerView, ShaderStageVisibility::Compute);
		builder.Uniform("ViewLighting", &IndirectDiffusePassParameters::ViewLighting, ShaderStageVisibility::Compute);
		builder.Uniform("IndirectDiffuseConstants", &IndirectDiffusePassParameters::IndirectDiffuseConstants, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferBaseColor", &IndirectDiffusePassParameters::GBufferBaseColor, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferNormal", &IndirectDiffusePassParameters::GBufferNormal, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferMaterial", &IndirectDiffusePassParameters::GBufferMaterial, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferDeviceZ", &IndirectDiffusePassParameters::GBufferDeviceZ, ShaderStageVisibility::Compute);
		builder.ReadTexture("SkyTexture", &IndirectDiffusePassParameters::SkyTexture, ShaderStageVisibility::Compute);
		builder.Sampler("SamplerLinearClamp", &IndirectDiffusePassParameters::SamplerLinearClamp, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RayTracingHitVertices", &IndirectDiffusePassParameters::RayTracingHitVertices, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RayTracingHitIndices", &IndirectDiffusePassParameters::RayTracingHitIndices, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RayTracingHitInstances", &IndirectDiffusePassParameters::RayTracingHitInstances, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RayTracingHitMaterials", &IndirectDiffusePassParameters::RayTracingHitMaterials, ShaderStageVisibility::Compute);
		builder.ReadBuffer("MeshInstances", &IndirectDiffusePassParameters::MeshInstances, ShaderStageVisibility::Compute);
		builder.ReadBuffer("DirectionalLights", &IndirectDiffusePassParameters::DirectionalLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("PointLights", &IndirectDiffusePassParameters::PointLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("SpotLights", &IndirectDiffusePassParameters::SpotLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RectLights", &IndirectDiffusePassParameters::RectLights, ShaderStageVisibility::Compute);
		builder.ReadTexture("MaterialTextureTable", &IndirectDiffusePassParameters::MaterialTextureTable, ShaderStageVisibility::Compute);
		builder.Sampler("MaterialTextureSampler", &IndirectDiffusePassParameters::MaterialTextureSampler, ShaderStageVisibility::Compute);
	}
};

class IndirectDiffusePass final
{
  public:
	static constexpr const char* PassName = "IndirectDiffuse";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	using Parameters = IndirectDiffusePassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

	explicit IndirectDiffusePass(const ComputePassPipelineRuntime& runtime) noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	static void DeclareResources(
	    FrameGraphBuilder& builder,
	    const LightingRenderTargets& lighting,
	    const GBufferRenderTargets& gbuffer,
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

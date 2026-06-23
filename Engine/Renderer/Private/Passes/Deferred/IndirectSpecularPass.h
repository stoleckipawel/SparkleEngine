#pragma once

#include "Frame/Targets/FrameRenderTargets.h"
#include "RayTracing/RayTracingHitData.h"
#include "RayTracing/Effects/IndirectSpecular/IndirectSpecularUniformData.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"
#include "Passes/Bindings/EnvironmentMapPassBinding.h"
#include "RHI/Public/Resources/RenderConstantBufferData.h"
#include "RHI/Public/Resources/RenderViewLightingData.h"
#include "SceneData/MaterialTextureTableCapability.h"

#include <cstdint>

class FrameGraphBuilder;
struct FrameContext;
struct ComputePassPipelineRuntime;
struct PassExecutionContext;
struct PassRuntimeServices;
struct RenderPassDefinition;
struct RenderViewData;

struct IndirectSpecularPassParameters
{
	ShaderRWTexture2D<void> IndirectSpecular;
	ShaderAccelerationStructure SceneTlas;
	ShaderUniform<PerFrameConstantBufferData> PerFrame;
	ShaderUniform<PerViewConstantBufferData> PerView;
	ShaderUniform<ViewLightingData> ViewLighting;
	ShaderUniform<IndirectSpecularUniformData> IndirectSpecularConstants;
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
	ShaderTexture2DTableSRV<MaterialTextureTableFixedCapacity> MaterialTextureTable;
	ShaderSamplerSet MaterialTextureSampler;

	static void Describe(ShaderParameterStructBuilder<IndirectSpecularPassParameters>& builder)
	{
		builder.RWTexture("IndirectSpecular", &IndirectSpecularPassParameters::IndirectSpecular, ShaderStageVisibility::Compute);
		builder.AccelerationStructure("SceneTlas", &IndirectSpecularPassParameters::SceneTlas, ShaderStageVisibility::Compute);
		builder.Uniform("PerFrame", &IndirectSpecularPassParameters::PerFrame, ShaderStageVisibility::Compute);
		builder.Uniform("PerView", &IndirectSpecularPassParameters::PerView, ShaderStageVisibility::Compute);
		builder.Uniform("ViewLighting", &IndirectSpecularPassParameters::ViewLighting, ShaderStageVisibility::Compute);
		builder.Uniform("IndirectSpecularConstants", &IndirectSpecularPassParameters::IndirectSpecularConstants, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferBaseColor", &IndirectSpecularPassParameters::GBufferBaseColor, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferNormal", &IndirectSpecularPassParameters::GBufferNormal, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferMaterial", &IndirectSpecularPassParameters::GBufferMaterial, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferDeviceZ", &IndirectSpecularPassParameters::GBufferDeviceZ, ShaderStageVisibility::Compute);
		builder.ReadTexture("SkyTexture", &IndirectSpecularPassParameters::SkyTexture, ShaderStageVisibility::Compute);
		builder.Sampler("SamplerLinearClamp", &IndirectSpecularPassParameters::SamplerLinearClamp, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RayTracingHitVertices", &IndirectSpecularPassParameters::RayTracingHitVertices, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RayTracingHitIndices", &IndirectSpecularPassParameters::RayTracingHitIndices, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RayTracingHitInstances", &IndirectSpecularPassParameters::RayTracingHitInstances, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RayTracingHitMaterials", &IndirectSpecularPassParameters::RayTracingHitMaterials, ShaderStageVisibility::Compute);
		builder.ReadBuffer("MeshInstances", &IndirectSpecularPassParameters::MeshInstances, ShaderStageVisibility::Compute);
		builder.ReadBuffer("DirectionalLights", &IndirectSpecularPassParameters::DirectionalLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("PointLights", &IndirectSpecularPassParameters::PointLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("SpotLights", &IndirectSpecularPassParameters::SpotLights, ShaderStageVisibility::Compute);
		builder.ReadTexture("MaterialTextureTable", &IndirectSpecularPassParameters::MaterialTextureTable, ShaderStageVisibility::Compute);
		builder.Sampler("MaterialTextureSampler", &IndirectSpecularPassParameters::MaterialTextureSampler, ShaderStageVisibility::Compute);
	}
};

class IndirectSpecularPass final
{
  public:
	static constexpr const char* PassName = "IndirectSpecular";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	using Parameters = IndirectSpecularPassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

	explicit IndirectSpecularPass(const ComputePassPipelineRuntime& runtime) noexcept;

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
	    const PassRuntimeServices& passRuntimeServices) const;

	const ComputePassPipelineRuntime& m_runtime;
	mutable EnvironmentMapPassBinding m_environmentMapBinding;
};

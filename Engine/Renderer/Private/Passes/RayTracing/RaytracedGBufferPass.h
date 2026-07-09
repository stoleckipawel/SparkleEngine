#pragma once

#include "Frame/Targets/FrameRenderTargets.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"
#include "SceneData/MaterialTextureTableCapability.h"
#include "ShaderData/RenderConstantBufferData.h"

#include <cstdint>
#include <type_traits>

class FrameGraphBuilder;
struct ComputePassPipelineRuntime;
struct FrameContext;
struct PassExecutionContext;
struct PassRuntimeServices;
struct RenderPassDefinition;
struct RenderViewData;

struct RaytracedGBufferUniformData
{
	std::uint32_t RayTracingHitDataAvailable = 0u;
	std::uint32_t RayTracingHitInstanceCount = 0u;
	std::uint32_t RayTracingHitMaterialCount = 0u;
	std::uint32_t Padding0 = 0u;
};

static_assert(std::is_standard_layout_v<RaytracedGBufferUniformData>, "RaytracedGBufferUniformData must be standard-layout");
static_assert(std::is_trivially_copyable_v<RaytracedGBufferUniformData>, "RaytracedGBufferUniformData must be trivially copyable");
static_assert(sizeof(RaytracedGBufferUniformData) == 16, "RaytracedGBufferUniformData must match the shader layout");

struct RaytracedGBufferPassParameters
{
	ShaderRWTexture2D<void> GBufferBaseColor;
	ShaderRWTexture2D<void> GBufferNormal;
	ShaderRWTexture2D<void> GBufferMaterial;
	ShaderRWTexture2D<void> GBufferEmissive;
	ShaderRWTexture2D<void> GBufferSubsurface;
	ShaderRWTexture2D<void> GBufferDeviceZ;
	ShaderRWTexture2D<void> GBufferMotionVector;
	ShaderAccelerationStructure SceneTlas;
	ShaderUniform<PerFrameConstantBufferData> PerFrame;
	ShaderUniform<PerViewConstantBufferData> PerView;
	ShaderUniform<PerTemporalConstantBufferData> PerTemporal;
	ShaderUniform<RaytracedGBufferUniformData> RaytracedGBufferConstants;
	ShaderBufferSRV RayTracingHitVertices;
	ShaderBufferSRV RayTracingHitIndices;
	ShaderBufferSRV RayTracingHitInstances;
	ShaderBufferSRV RayTracingHitMaterials;
	ShaderBufferSRV MeshInstances;
	ShaderBufferSRV SkinInfluences;
	ShaderBufferSRV JointMatrices;
	ShaderBufferSRV PreviousJointMatrices;
	ShaderTexture2DTableSRV<MaterialTextureTableFixedCapacity> MaterialTextureTable;
	ShaderSamplerSet MaterialTextureSampler;

	static void Describe(ShaderParameterStructBuilder<RaytracedGBufferPassParameters>& builder)
	{
		builder.RWTexture("GBufferBaseColor", &RaytracedGBufferPassParameters::GBufferBaseColor, ShaderStageVisibility::Compute);
		builder.RWTexture("GBufferNormal", &RaytracedGBufferPassParameters::GBufferNormal, ShaderStageVisibility::Compute);
		builder.RWTexture("GBufferMaterial", &RaytracedGBufferPassParameters::GBufferMaterial, ShaderStageVisibility::Compute);
		builder.RWTexture("GBufferEmissive", &RaytracedGBufferPassParameters::GBufferEmissive, ShaderStageVisibility::Compute);
		builder.RWTexture("GBufferSubsurface", &RaytracedGBufferPassParameters::GBufferSubsurface, ShaderStageVisibility::Compute);
		builder.RWTexture("GBufferDeviceZ", &RaytracedGBufferPassParameters::GBufferDeviceZ, ShaderStageVisibility::Compute);
		builder.RWTexture("GBufferMotionVector", &RaytracedGBufferPassParameters::GBufferMotionVector, ShaderStageVisibility::Compute);
		builder.AccelerationStructure("SceneTlas", &RaytracedGBufferPassParameters::SceneTlas, ShaderStageVisibility::Compute);
		builder.Uniform("PerFrame", &RaytracedGBufferPassParameters::PerFrame, ShaderStageVisibility::Compute);
		builder.Uniform("PerView", &RaytracedGBufferPassParameters::PerView, ShaderStageVisibility::Compute);
		builder.Uniform("PerTemporal", &RaytracedGBufferPassParameters::PerTemporal, ShaderStageVisibility::Compute);
		builder.Uniform(
		    "RaytracedGBufferConstants",
		    &RaytracedGBufferPassParameters::RaytracedGBufferConstants,
		    ShaderStageVisibility::Compute);
		builder.ReadBuffer("RayTracingHitVertices", &RaytracedGBufferPassParameters::RayTracingHitVertices, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RayTracingHitIndices", &RaytracedGBufferPassParameters::RayTracingHitIndices, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RayTracingHitInstances", &RaytracedGBufferPassParameters::RayTracingHitInstances, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RayTracingHitMaterials", &RaytracedGBufferPassParameters::RayTracingHitMaterials, ShaderStageVisibility::Compute);
		builder.ReadBuffer("MeshInstances", &RaytracedGBufferPassParameters::MeshInstances, ShaderStageVisibility::Compute);
		builder.ReadBuffer("SkinInfluences", &RaytracedGBufferPassParameters::SkinInfluences, ShaderStageVisibility::Compute);
		builder.ReadBuffer("JointMatrices", &RaytracedGBufferPassParameters::JointMatrices, ShaderStageVisibility::Compute);
		builder.ReadBuffer("PreviousJointMatrices", &RaytracedGBufferPassParameters::PreviousJointMatrices, ShaderStageVisibility::Compute);
		builder.ReadTexture("MaterialTextureTable", &RaytracedGBufferPassParameters::MaterialTextureTable, ShaderStageVisibility::Compute);
		builder.Sampler("MaterialTextureSampler", &RaytracedGBufferPassParameters::MaterialTextureSampler, ShaderStageVisibility::Compute);
	}
};

class RaytracedGBufferPass final
{
  public:
	static constexpr const char* PassName = "RaytracedGBuffer";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	using Parameters = RaytracedGBufferPassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

	explicit RaytracedGBufferPass(const ComputePassPipelineRuntime& runtime) noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	static void DeclareResources(
	    FrameGraphBuilder& builder,
	    const GBufferRenderTargets& targets,
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
};

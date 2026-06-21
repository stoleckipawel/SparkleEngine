#pragma once

#include "Frame/Targets/FrameRenderTargets.h"
#include "RayTracing/RTIndirectSpecularHitData.h"
#include "RayTracing/RTIndirectSpecularUniformData.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"
#include "RHI/Public/Resources/RenderConstantBufferData.h"

#include <cstdint>

class FrameGraphBuilder;
struct ComputePassPipelineRuntime;
struct PassExecutionContext;
struct PassRuntimeServices;
struct RenderPassDefinition;
struct RenderViewData;

struct RTIndirectSpecularPassParameters
{
	ShaderRWTexture2D<void> IndirectSpecular;
	ShaderAccelerationStructure SceneTlas;
	ShaderUniform<PerFrameConstantBufferData> PerFrame;
	ShaderUniform<PerViewConstantBufferData> PerView;
	ShaderUniform<RTIndirectSpecularUniformData> RTIndirectSpecular;
	ShaderTexture2D<void> GBufferBaseColor;
	ShaderTexture2D<void> GBufferNormal;
	ShaderTexture2D<void> GBufferMaterial;
	ShaderTexture2D<void> GBufferDeviceZ;
	ShaderBuffer<RTIndirectSpecularHitVertex> RTIndirectSpecularHitVertices;
	ShaderBuffer<std::uint32_t> RTIndirectSpecularHitIndices;
	ShaderBuffer<RTIndirectSpecularHitInstance> RTIndirectSpecularHitInstances;
	ShaderBuffer<RTIndirectSpecularHitMaterial> RTIndirectSpecularHitMaterials;
	ShaderBuffer<MeshInstanceData> MeshInstances;

	static void Describe(ShaderParameterStructBuilder<RTIndirectSpecularPassParameters>& builder)
	{
		builder.RWTexture("IndirectSpecular", &RTIndirectSpecularPassParameters::IndirectSpecular, ShaderStageVisibility::Compute);
		builder.AccelerationStructure("SceneTlas", &RTIndirectSpecularPassParameters::SceneTlas, ShaderStageVisibility::Compute);
		builder.Uniform("PerFrame", &RTIndirectSpecularPassParameters::PerFrame, ShaderStageVisibility::Compute);
		builder.Uniform("PerView", &RTIndirectSpecularPassParameters::PerView, ShaderStageVisibility::Compute);
		builder.Uniform("RTIndirectSpecular", &RTIndirectSpecularPassParameters::RTIndirectSpecular, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferBaseColor", &RTIndirectSpecularPassParameters::GBufferBaseColor, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferNormal", &RTIndirectSpecularPassParameters::GBufferNormal, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferMaterial", &RTIndirectSpecularPassParameters::GBufferMaterial, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferDeviceZ", &RTIndirectSpecularPassParameters::GBufferDeviceZ, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RTIndirectSpecularHitVertices", &RTIndirectSpecularPassParameters::RTIndirectSpecularHitVertices, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RTIndirectSpecularHitIndices", &RTIndirectSpecularPassParameters::RTIndirectSpecularHitIndices, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RTIndirectSpecularHitInstances", &RTIndirectSpecularPassParameters::RTIndirectSpecularHitInstances, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RTIndirectSpecularHitMaterials", &RTIndirectSpecularPassParameters::RTIndirectSpecularHitMaterials, ShaderStageVisibility::Compute);
		builder.ReadBuffer("MeshInstances", &RTIndirectSpecularPassParameters::MeshInstances, ShaderStageVisibility::Compute);
	}
};

class RTIndirectSpecularPass final
{
  public:
	static constexpr const char* PassName = "RTIndirectSpecular";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	using Parameters = RTIndirectSpecularPassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

	explicit RTIndirectSpecularPass(const ComputePassPipelineRuntime& runtime) noexcept;

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
	    const RenderViewData& viewData,
	    const PassRuntimeServices& passRuntimeServices) const;

	const ComputePassPipelineRuntime& m_runtime;
};

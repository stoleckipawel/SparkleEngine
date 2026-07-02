#pragma once

#include "Frame/Targets/FrameRenderTargets.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowUniformData.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"
#include "SceneData/MaterialTextureTableCapability.h"

#include "RHI/Public/Resources/RenderConstantBufferData.h"
#include "RHI/Public/Resources/RenderViewLightingData.h"

#include <cstdint>

class FrameGraphBuilder;
struct DirectShadowSignalResources;
struct ComputePassPipelineRuntime;
struct FrameContext;
struct PassExecutionContext;
struct PassRuntimeServices;
struct RenderPassDefinition;
struct RenderViewData;

struct DirectShadowSignalPassParameters
{
	ShaderRWTexture2D<void> ShadowVisibilitySignal;
	ShaderRWTexture2D<void> ShadowLightSample;
	ShaderTexture2D<void> GBufferNormal;
	ShaderTexture2D<void> GBufferDeviceZ;
	ShaderAccelerationStructure SceneTlas;
	ShaderUniform<PerFrameConstantBufferData> PerFrame;
	ShaderUniform<PerViewConstantBufferData> PerView;
	ShaderUniform<ViewLightingData> ViewLighting;
	ShaderUniform<RayTracedShadowUniformData> RayTracedShadows;
	ShaderBufferSRV DirectionalLights;
	ShaderBufferSRV PointLights;
	ShaderBufferSRV SpotLights;
	ShaderBufferSRV RectLights;
	ShaderBufferSRV RayTracingHitVertices;
	ShaderBufferSRV RayTracingHitIndices;
	ShaderBufferSRV RayTracingHitInstances;
	ShaderBufferSRV RayTracingHitMaterials;
	ShaderTexture2DTableSRV<MaterialTextureTableFixedCapacity> MaterialTextureTable;
	ShaderSamplerSet MaterialTextureSampler;

	static void Describe(ShaderParameterStructBuilder<DirectShadowSignalPassParameters>& builder)
	{
		builder.RWTexture("ShadowVisibilitySignal", &DirectShadowSignalPassParameters::ShadowVisibilitySignal, ShaderStageVisibility::Compute);
		builder.RWTexture("ShadowLightSample", &DirectShadowSignalPassParameters::ShadowLightSample, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferNormal", &DirectShadowSignalPassParameters::GBufferNormal, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferDeviceZ", &DirectShadowSignalPassParameters::GBufferDeviceZ, ShaderStageVisibility::Compute);
		builder.AccelerationStructure("SceneTlas", &DirectShadowSignalPassParameters::SceneTlas, ShaderStageVisibility::Compute);
		builder.Uniform("PerFrame", &DirectShadowSignalPassParameters::PerFrame, ShaderStageVisibility::Compute);
		builder.Uniform("PerView", &DirectShadowSignalPassParameters::PerView, ShaderStageVisibility::Compute);
		builder.Uniform("ViewLighting", &DirectShadowSignalPassParameters::ViewLighting, ShaderStageVisibility::Compute);
		builder.Uniform("RayTracedShadows", &DirectShadowSignalPassParameters::RayTracedShadows, ShaderStageVisibility::Compute);
		builder.ReadBuffer("DirectionalLights", &DirectShadowSignalPassParameters::DirectionalLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("PointLights", &DirectShadowSignalPassParameters::PointLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("SpotLights", &DirectShadowSignalPassParameters::SpotLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RectLights", &DirectShadowSignalPassParameters::RectLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RayTracingHitVertices", &DirectShadowSignalPassParameters::RayTracingHitVertices, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RayTracingHitIndices", &DirectShadowSignalPassParameters::RayTracingHitIndices, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RayTracingHitInstances", &DirectShadowSignalPassParameters::RayTracingHitInstances, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RayTracingHitMaterials", &DirectShadowSignalPassParameters::RayTracingHitMaterials, ShaderStageVisibility::Compute);
		builder.ReadTexture("MaterialTextureTable", &DirectShadowSignalPassParameters::MaterialTextureTable, ShaderStageVisibility::Compute);
		builder.Sampler("MaterialTextureSampler", &DirectShadowSignalPassParameters::MaterialTextureSampler, ShaderStageVisibility::Compute);
	}
};

struct DirectShadowSignalNoRayQueryPassParameters
{
	ShaderRWTexture2D<void> ShadowVisibilitySignal;
	ShaderRWTexture2D<void> ShadowLightSample;
	ShaderTexture2D<void> GBufferNormal;
	ShaderTexture2D<void> GBufferDeviceZ;
	ShaderUniform<PerFrameConstantBufferData> PerFrame;
	ShaderUniform<PerViewConstantBufferData> PerView;
	ShaderUniform<ViewLightingData> ViewLighting;
	ShaderBufferSRV DirectionalLights;
	ShaderBufferSRV PointLights;
	ShaderBufferSRV SpotLights;
	ShaderBufferSRV RectLights;

	static void Describe(ShaderParameterStructBuilder<DirectShadowSignalNoRayQueryPassParameters>& builder)
	{
		builder.RWTexture("ShadowVisibilitySignal", &DirectShadowSignalNoRayQueryPassParameters::ShadowVisibilitySignal, ShaderStageVisibility::Compute);
		builder.RWTexture("ShadowLightSample", &DirectShadowSignalNoRayQueryPassParameters::ShadowLightSample, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferNormal", &DirectShadowSignalNoRayQueryPassParameters::GBufferNormal, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferDeviceZ", &DirectShadowSignalNoRayQueryPassParameters::GBufferDeviceZ, ShaderStageVisibility::Compute);
		builder.Uniform("PerFrame", &DirectShadowSignalNoRayQueryPassParameters::PerFrame, ShaderStageVisibility::Compute);
		builder.Uniform("PerView", &DirectShadowSignalNoRayQueryPassParameters::PerView, ShaderStageVisibility::Compute);
		builder.Uniform("ViewLighting", &DirectShadowSignalNoRayQueryPassParameters::ViewLighting, ShaderStageVisibility::Compute);
		builder.ReadBuffer("DirectionalLights", &DirectShadowSignalNoRayQueryPassParameters::DirectionalLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("PointLights", &DirectShadowSignalNoRayQueryPassParameters::PointLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("SpotLights", &DirectShadowSignalNoRayQueryPassParameters::SpotLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RectLights", &DirectShadowSignalNoRayQueryPassParameters::RectLights, ShaderStageVisibility::Compute);
	}
};

struct DirectShadowSignalDeviceAddressPassParameters
{
	ShaderRWTexture2D<void> ShadowVisibilitySignal;
	ShaderRWTexture2D<void> ShadowLightSample;
	ShaderTexture2D<void> GBufferNormal;
	ShaderTexture2D<void> GBufferDeviceZ;
	ShaderUniform<PerFrameConstantBufferData> PerFrame;
	ShaderUniform<PerViewConstantBufferData> PerView;
	ShaderUniform<ViewLightingData> ViewLighting;
	ShaderUniform<RayTracedShadowUniformData> RayTracedShadows;
	ShaderBufferSRV DirectionalLights;
	ShaderBufferSRV PointLights;
	ShaderBufferSRV SpotLights;
	ShaderBufferSRV RectLights;
	ShaderBufferSRV RayTracingHitVertices;
	ShaderBufferSRV RayTracingHitIndices;
	ShaderBufferSRV RayTracingHitInstances;
	ShaderBufferSRV RayTracingHitMaterials;
	ShaderTexture2DTableSRV<MaterialTextureTableFixedCapacity> MaterialTextureTable;
	ShaderSamplerSet MaterialTextureSampler;

	static void Describe(ShaderParameterStructBuilder<DirectShadowSignalDeviceAddressPassParameters>& builder)
	{
		builder.RWTexture("ShadowVisibilitySignal", &DirectShadowSignalDeviceAddressPassParameters::ShadowVisibilitySignal, ShaderStageVisibility::Compute);
		builder.RWTexture("ShadowLightSample", &DirectShadowSignalDeviceAddressPassParameters::ShadowLightSample, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferNormal", &DirectShadowSignalDeviceAddressPassParameters::GBufferNormal, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferDeviceZ", &DirectShadowSignalDeviceAddressPassParameters::GBufferDeviceZ, ShaderStageVisibility::Compute);
		builder.Uniform("PerFrame", &DirectShadowSignalDeviceAddressPassParameters::PerFrame, ShaderStageVisibility::Compute);
		builder.Uniform("PerView", &DirectShadowSignalDeviceAddressPassParameters::PerView, ShaderStageVisibility::Compute);
		builder.Uniform("ViewLighting", &DirectShadowSignalDeviceAddressPassParameters::ViewLighting, ShaderStageVisibility::Compute);
		builder.Uniform("RayTracedShadows", &DirectShadowSignalDeviceAddressPassParameters::RayTracedShadows, ShaderStageVisibility::Compute);
		builder.ReadBuffer("DirectionalLights", &DirectShadowSignalDeviceAddressPassParameters::DirectionalLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("PointLights", &DirectShadowSignalDeviceAddressPassParameters::PointLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("SpotLights", &DirectShadowSignalDeviceAddressPassParameters::SpotLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RectLights", &DirectShadowSignalDeviceAddressPassParameters::RectLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RayTracingHitVertices", &DirectShadowSignalDeviceAddressPassParameters::RayTracingHitVertices, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RayTracingHitIndices", &DirectShadowSignalDeviceAddressPassParameters::RayTracingHitIndices, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RayTracingHitInstances", &DirectShadowSignalDeviceAddressPassParameters::RayTracingHitInstances, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RayTracingHitMaterials", &DirectShadowSignalDeviceAddressPassParameters::RayTracingHitMaterials, ShaderStageVisibility::Compute);
		builder.ReadTexture("MaterialTextureTable", &DirectShadowSignalDeviceAddressPassParameters::MaterialTextureTable, ShaderStageVisibility::Compute);
		builder.Sampler("MaterialTextureSampler", &DirectShadowSignalDeviceAddressPassParameters::MaterialTextureSampler, ShaderStageVisibility::Compute);
	}
};

class DirectShadowSignalNoRayQueryPass final
{
  public:
	static constexpr const char* PassName = "DirectShadowSignalNoRayQuery";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	using Parameters = DirectShadowSignalNoRayQueryPassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

	explicit DirectShadowSignalNoRayQueryPass(const ComputePassPipelineRuntime& runtime) noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	static void DeclareResources(
	    FrameGraphBuilder& builder,
	    const GBufferRenderTargets& gbuffer,
	    const DirectShadowSignalResources& shadowSignals,
	    ParameterInstance& parameters);
	void Execute(PassExecutionContext& context, ParameterInstance& parameters) const;

  private:
	void SetParameters(ParameterInstance& parameters, const FrameContext& frame, const RenderViewData& viewData, const PassRuntimeServices& passRuntimeServices) const;

	const ComputePassPipelineRuntime& m_runtime;
};

class DirectShadowSignalPass final
{
  public:
	static constexpr const char* PassName = "DirectShadowSignal";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	using Parameters = DirectShadowSignalPassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

	explicit DirectShadowSignalPass(const ComputePassPipelineRuntime& runtime) noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	static void DeclareResources(
	    FrameGraphBuilder& builder,
	    const GBufferRenderTargets& gbuffer,
	    FrameGraphAccelerationStructureHandle sceneTlas,
	    const DirectShadowSignalResources& shadowSignals,
	    ParameterInstance& parameters);
	void Execute(PassExecutionContext& context, ParameterInstance& parameters) const;

  private:
	void SetParameters(
	    ParameterInstance& parameters,
	    const FrameContext& frame,
	    const RenderViewData& viewData,
	    const PassRuntimeServices& passRuntimeServices,
	    bool hasSceneTlas) const;

	const ComputePassPipelineRuntime& m_runtime;
};

class DirectShadowSignalDeviceAddressPass final
{
  public:
	static constexpr const char* PassName = "DirectShadowSignalDeviceAddress";
	static constexpr std::uint32_t ThreadGroupSizeX = DirectShadowSignalPass::ThreadGroupSizeX;
	static constexpr std::uint32_t ThreadGroupSizeY = DirectShadowSignalPass::ThreadGroupSizeY;
	using Parameters = DirectShadowSignalDeviceAddressPassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

	explicit DirectShadowSignalDeviceAddressPass(const ComputePassPipelineRuntime& runtime) noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	static void DeclareResources(
	    FrameGraphBuilder& builder,
	    const GBufferRenderTargets& gbuffer,
	    const DirectShadowSignalResources& shadowSignals,
	    ParameterInstance& parameters);
	void Execute(PassExecutionContext& context, ParameterInstance& parameters) const;

  private:
	void SetParameters(
	    ParameterInstance& parameters,
	    const FrameContext& frame,
	    const RenderViewData& viewData,
	    const PassRuntimeServices& passRuntimeServices,
	    bool hasSceneTlas) const;

	const ComputePassPipelineRuntime& m_runtime;
};

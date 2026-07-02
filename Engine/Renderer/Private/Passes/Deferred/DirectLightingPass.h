#pragma once

#include "Frame/Targets/FrameRenderTargets.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowUniformData.h"
#include "SceneData/MaterialTextureTableCapability.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"

#include "RHI/Public/Resources/RenderConstantBufferData.h"
#include "RHI/Public/Resources/RenderViewLightingData.h"

#include <cstdint>

class FrameGraphBuilder;
struct RenderPassDefinition;
struct ComputePassPipelineRuntime;
struct PassExecutionContext;
struct PassRuntimeServices;
struct FrameContext;
struct RenderViewData;

struct DirectLightingPassParameters
{
	ShaderRWTexture2D<void> DirectDiffuse;
	ShaderRWTexture2D<void> DirectSpecular;
	ShaderRWTexture2D<void> DirectSubsurface;
	ShaderTexture2D<void> ShadowVisibilitySignal;
	ShaderTexture2D<void> GBufferBaseColor;
	ShaderTexture2D<void> GBufferNormal;
	ShaderTexture2D<void> GBufferMaterial;
	ShaderTexture2D<void> GBufferSubsurface;
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

	static void Describe(ShaderParameterStructBuilder<DirectLightingPassParameters>& builder)
	{
		builder.RWTexture("DirectDiffuse", &DirectLightingPassParameters::DirectDiffuse, ShaderStageVisibility::Compute);
		builder.RWTexture("DirectSpecular", &DirectLightingPassParameters::DirectSpecular, ShaderStageVisibility::Compute);
		builder.RWTexture("DirectSubsurface", &DirectLightingPassParameters::DirectSubsurface, ShaderStageVisibility::Compute);
		builder.ReadTexture("ShadowVisibilitySignal", &DirectLightingPassParameters::ShadowVisibilitySignal, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferBaseColor", &DirectLightingPassParameters::GBufferBaseColor, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferNormal", &DirectLightingPassParameters::GBufferNormal, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferMaterial", &DirectLightingPassParameters::GBufferMaterial, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferSubsurface", &DirectLightingPassParameters::GBufferSubsurface, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferDeviceZ", &DirectLightingPassParameters::GBufferDeviceZ, ShaderStageVisibility::Compute);
		builder.AccelerationStructure("SceneTlas", &DirectLightingPassParameters::SceneTlas, ShaderStageVisibility::Compute);
		builder.Uniform("PerFrame", &DirectLightingPassParameters::PerFrame, ShaderStageVisibility::Compute);
		builder.Uniform("PerView", &DirectLightingPassParameters::PerView, ShaderStageVisibility::Compute);
		builder.Uniform("ViewLighting", &DirectLightingPassParameters::ViewLighting, ShaderStageVisibility::Compute);
		builder.Uniform("RayTracedShadows", &DirectLightingPassParameters::RayTracedShadows, ShaderStageVisibility::Compute);
		builder.ReadBuffer("DirectionalLights", &DirectLightingPassParameters::DirectionalLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("PointLights", &DirectLightingPassParameters::PointLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("SpotLights", &DirectLightingPassParameters::SpotLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RectLights", &DirectLightingPassParameters::RectLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RayTracingHitVertices", &DirectLightingPassParameters::RayTracingHitVertices, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RayTracingHitIndices", &DirectLightingPassParameters::RayTracingHitIndices, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RayTracingHitInstances", &DirectLightingPassParameters::RayTracingHitInstances, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RayTracingHitMaterials", &DirectLightingPassParameters::RayTracingHitMaterials, ShaderStageVisibility::Compute);
		builder.ReadTexture("MaterialTextureTable", &DirectLightingPassParameters::MaterialTextureTable, ShaderStageVisibility::Compute);
		builder.Sampler("MaterialTextureSampler", &DirectLightingPassParameters::MaterialTextureSampler, ShaderStageVisibility::Compute);
	}
};

struct DirectLightingNoRayQueryPassParameters
{
	ShaderRWTexture2D<void> DirectDiffuse;
	ShaderRWTexture2D<void> DirectSpecular;
	ShaderRWTexture2D<void> DirectSubsurface;
	ShaderTexture2D<void> ShadowVisibilitySignal;
	ShaderTexture2D<void> GBufferBaseColor;
	ShaderTexture2D<void> GBufferNormal;
	ShaderTexture2D<void> GBufferMaterial;
	ShaderTexture2D<void> GBufferSubsurface;
	ShaderTexture2D<void> GBufferDeviceZ;
	ShaderUniform<PerFrameConstantBufferData> PerFrame;
	ShaderUniform<PerViewConstantBufferData> PerView;
	ShaderUniform<ViewLightingData> ViewLighting;
	ShaderBufferSRV DirectionalLights;
	ShaderBufferSRV PointLights;
	ShaderBufferSRV SpotLights;
	ShaderBufferSRV RectLights;

	static void Describe(ShaderParameterStructBuilder<DirectLightingNoRayQueryPassParameters>& builder)
	{
		builder.RWTexture("DirectDiffuse", &DirectLightingNoRayQueryPassParameters::DirectDiffuse, ShaderStageVisibility::Compute);
		builder.RWTexture("DirectSpecular", &DirectLightingNoRayQueryPassParameters::DirectSpecular, ShaderStageVisibility::Compute);
		builder.RWTexture("DirectSubsurface", &DirectLightingNoRayQueryPassParameters::DirectSubsurface, ShaderStageVisibility::Compute);
		builder.ReadTexture("ShadowVisibilitySignal", &DirectLightingNoRayQueryPassParameters::ShadowVisibilitySignal, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferBaseColor", &DirectLightingNoRayQueryPassParameters::GBufferBaseColor, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferNormal", &DirectLightingNoRayQueryPassParameters::GBufferNormal, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferMaterial", &DirectLightingNoRayQueryPassParameters::GBufferMaterial, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferSubsurface", &DirectLightingNoRayQueryPassParameters::GBufferSubsurface, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferDeviceZ", &DirectLightingNoRayQueryPassParameters::GBufferDeviceZ, ShaderStageVisibility::Compute);
		builder.Uniform("PerFrame", &DirectLightingNoRayQueryPassParameters::PerFrame, ShaderStageVisibility::Compute);
		builder.Uniform("PerView", &DirectLightingNoRayQueryPassParameters::PerView, ShaderStageVisibility::Compute);
		builder.Uniform("ViewLighting", &DirectLightingNoRayQueryPassParameters::ViewLighting, ShaderStageVisibility::Compute);
		builder.ReadBuffer("DirectionalLights", &DirectLightingNoRayQueryPassParameters::DirectionalLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("PointLights", &DirectLightingNoRayQueryPassParameters::PointLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("SpotLights", &DirectLightingNoRayQueryPassParameters::SpotLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RectLights", &DirectLightingNoRayQueryPassParameters::RectLights, ShaderStageVisibility::Compute);
	}
};

struct DirectLightingDeviceAddressPassParameters
{
	ShaderRWTexture2D<void> DirectDiffuse;
	ShaderRWTexture2D<void> DirectSpecular;
	ShaderRWTexture2D<void> DirectSubsurface;
	ShaderTexture2D<void> ShadowVisibilitySignal;
	ShaderTexture2D<void> GBufferBaseColor;
	ShaderTexture2D<void> GBufferNormal;
	ShaderTexture2D<void> GBufferMaterial;
	ShaderTexture2D<void> GBufferSubsurface;
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

	static void Describe(ShaderParameterStructBuilder<DirectLightingDeviceAddressPassParameters>& builder)
	{
		builder.RWTexture(
		    "DirectDiffuse",
		    &DirectLightingDeviceAddressPassParameters::DirectDiffuse,
		    ShaderStageVisibility::Compute);
		builder.RWTexture(
		    "DirectSpecular",
		    &DirectLightingDeviceAddressPassParameters::DirectSpecular,
		    ShaderStageVisibility::Compute);
		builder.RWTexture(
		    "DirectSubsurface",
		    &DirectLightingDeviceAddressPassParameters::DirectSubsurface,
		    ShaderStageVisibility::Compute);
		builder.ReadTexture(
		    "ShadowVisibilitySignal",
		    &DirectLightingDeviceAddressPassParameters::ShadowVisibilitySignal,
		    ShaderStageVisibility::Compute);
		builder.ReadTexture(
		    "GBufferBaseColor",
		    &DirectLightingDeviceAddressPassParameters::GBufferBaseColor,
		    ShaderStageVisibility::Compute);
		builder.ReadTexture(
		    "GBufferNormal",
		    &DirectLightingDeviceAddressPassParameters::GBufferNormal,
		    ShaderStageVisibility::Compute);
		builder.ReadTexture(
		    "GBufferMaterial",
		    &DirectLightingDeviceAddressPassParameters::GBufferMaterial,
		    ShaderStageVisibility::Compute);
		builder.ReadTexture(
		    "GBufferSubsurface",
		    &DirectLightingDeviceAddressPassParameters::GBufferSubsurface,
		    ShaderStageVisibility::Compute);
		builder.ReadTexture(
		    "GBufferDeviceZ",
		    &DirectLightingDeviceAddressPassParameters::GBufferDeviceZ,
		    ShaderStageVisibility::Compute);
		builder.Uniform("PerFrame", &DirectLightingDeviceAddressPassParameters::PerFrame, ShaderStageVisibility::Compute);
		builder.Uniform("PerView", &DirectLightingDeviceAddressPassParameters::PerView, ShaderStageVisibility::Compute);
		builder.Uniform("ViewLighting", &DirectLightingDeviceAddressPassParameters::ViewLighting, ShaderStageVisibility::Compute);
		builder.Uniform(
		    "RayTracedShadows",
		    &DirectLightingDeviceAddressPassParameters::RayTracedShadows,
		    ShaderStageVisibility::Compute);
		builder.ReadBuffer("DirectionalLights", &DirectLightingDeviceAddressPassParameters::DirectionalLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("PointLights", &DirectLightingDeviceAddressPassParameters::PointLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("SpotLights", &DirectLightingDeviceAddressPassParameters::SpotLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RectLights", &DirectLightingDeviceAddressPassParameters::RectLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RayTracingHitVertices", &DirectLightingDeviceAddressPassParameters::RayTracingHitVertices, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RayTracingHitIndices", &DirectLightingDeviceAddressPassParameters::RayTracingHitIndices, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RayTracingHitInstances", &DirectLightingDeviceAddressPassParameters::RayTracingHitInstances, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RayTracingHitMaterials", &DirectLightingDeviceAddressPassParameters::RayTracingHitMaterials, ShaderStageVisibility::Compute);
		builder.ReadTexture("MaterialTextureTable", &DirectLightingDeviceAddressPassParameters::MaterialTextureTable, ShaderStageVisibility::Compute);
		builder.Sampler("MaterialTextureSampler", &DirectLightingDeviceAddressPassParameters::MaterialTextureSampler, ShaderStageVisibility::Compute);
	}
};

class DirectLightingNoRayQueryPass final
{
  public:
	static constexpr const char* PassName = "DirectLightingNoRayQuery";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	using Parameters = DirectLightingNoRayQueryPassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

	explicit DirectLightingNoRayQueryPass(const ComputePassPipelineRuntime& runtime) noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	static void DeclareResources(
	    FrameGraphBuilder& builder,
	    const LightingRenderTargets& lighting,
	    const GBufferRenderTargets& gbuffer,
	    FrameGraphTextureHandle shadowVisibilitySignal,
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

class DirectLightingPass final
{
  public:
	static constexpr const char* PassName = "DirectLighting";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	using Parameters = DirectLightingPassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

	explicit DirectLightingPass(const ComputePassPipelineRuntime& runtime) noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	static void DeclareResources(
	    FrameGraphBuilder& builder,
	    const LightingRenderTargets& lighting,
	    const GBufferRenderTargets& gbuffer,
	    FrameGraphAccelerationStructureHandle sceneTlas,
	    FrameGraphTextureHandle shadowVisibilitySignal,
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

class DirectLightingDeviceAddressPass final
{
  public:
	static constexpr const char* PassName = "DirectLightingDeviceAddress";
	static constexpr std::uint32_t ThreadGroupSizeX = DirectLightingPass::ThreadGroupSizeX;
	static constexpr std::uint32_t ThreadGroupSizeY = DirectLightingPass::ThreadGroupSizeY;
	using Parameters = DirectLightingDeviceAddressPassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

	explicit DirectLightingDeviceAddressPass(const ComputePassPipelineRuntime& runtime) noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	static void DeclareResources(
	    FrameGraphBuilder& builder,
	    const LightingRenderTargets& lighting,
	    const GBufferRenderTargets& gbuffer,
	    FrameGraphTextureHandle shadowVisibilitySignal,
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

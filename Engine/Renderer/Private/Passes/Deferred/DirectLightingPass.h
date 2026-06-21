#pragma once

#include "Frame/Targets/FrameRenderTargets.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowUniformData.h"
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

	static void Describe(ShaderParameterStructBuilder<DirectLightingPassParameters>& builder)
	{
		builder.RWTexture("DirectDiffuse", &DirectLightingPassParameters::DirectDiffuse, ShaderStageVisibility::Compute);
		builder.RWTexture("DirectSpecular", &DirectLightingPassParameters::DirectSpecular, ShaderStageVisibility::Compute);
		builder.RWTexture("DirectSubsurface", &DirectLightingPassParameters::DirectSubsurface, ShaderStageVisibility::Compute);
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
	}
};

struct DirectLightingVulkanAddressPassParameters
{
	ShaderRWTexture2D<void> DirectDiffuse;
	ShaderRWTexture2D<void> DirectSpecular;
	ShaderRWTexture2D<void> DirectSubsurface;
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

	static void Describe(ShaderParameterStructBuilder<DirectLightingVulkanAddressPassParameters>& builder)
	{
		builder.RWTexture(
		    "DirectDiffuse",
		    &DirectLightingVulkanAddressPassParameters::DirectDiffuse,
		    ShaderStageVisibility::Compute);
		builder.RWTexture(
		    "DirectSpecular",
		    &DirectLightingVulkanAddressPassParameters::DirectSpecular,
		    ShaderStageVisibility::Compute);
		builder.RWTexture(
		    "DirectSubsurface",
		    &DirectLightingVulkanAddressPassParameters::DirectSubsurface,
		    ShaderStageVisibility::Compute);
		builder.ReadTexture(
		    "GBufferBaseColor",
		    &DirectLightingVulkanAddressPassParameters::GBufferBaseColor,
		    ShaderStageVisibility::Compute);
		builder.ReadTexture(
		    "GBufferNormal",
		    &DirectLightingVulkanAddressPassParameters::GBufferNormal,
		    ShaderStageVisibility::Compute);
		builder.ReadTexture(
		    "GBufferMaterial",
		    &DirectLightingVulkanAddressPassParameters::GBufferMaterial,
		    ShaderStageVisibility::Compute);
		builder.ReadTexture(
		    "GBufferSubsurface",
		    &DirectLightingVulkanAddressPassParameters::GBufferSubsurface,
		    ShaderStageVisibility::Compute);
		builder.ReadTexture(
		    "GBufferDeviceZ",
		    &DirectLightingVulkanAddressPassParameters::GBufferDeviceZ,
		    ShaderStageVisibility::Compute);
		builder.Uniform("PerFrame", &DirectLightingVulkanAddressPassParameters::PerFrame, ShaderStageVisibility::Compute);
		builder.Uniform("PerView", &DirectLightingVulkanAddressPassParameters::PerView, ShaderStageVisibility::Compute);
		builder.Uniform("ViewLighting", &DirectLightingVulkanAddressPassParameters::ViewLighting, ShaderStageVisibility::Compute);
		builder.Uniform(
		    "RayTracedShadows",
		    &DirectLightingVulkanAddressPassParameters::RayTracedShadows,
		    ShaderStageVisibility::Compute);
		builder.ReadBuffer("DirectionalLights", &DirectLightingVulkanAddressPassParameters::DirectionalLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("PointLights", &DirectLightingVulkanAddressPassParameters::PointLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("SpotLights", &DirectLightingVulkanAddressPassParameters::SpotLights, ShaderStageVisibility::Compute);
	}
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

class DirectLightingVulkanAddressPass final
{
  public:
	static constexpr const char* PassName = "DirectLightingVulkanAddress";
	static constexpr std::uint32_t ThreadGroupSizeX = DirectLightingPass::ThreadGroupSizeX;
	static constexpr std::uint32_t ThreadGroupSizeY = DirectLightingPass::ThreadGroupSizeY;
	using Parameters = DirectLightingVulkanAddressPassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

	explicit DirectLightingVulkanAddressPass(const ComputePassPipelineRuntime& runtime) noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	static void DeclareResources(
	    FrameGraphBuilder& builder,
	    const LightingRenderTargets& lighting,
	    const GBufferRenderTargets& gbuffer,
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

#pragma once

#include "Frame/Targets/FrameRenderTargets.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"

#include "ShaderData/RenderConstantBufferData.h"
#include "ShaderData/RenderViewLightingData.h"

#include <cstdint>

class FrameGraphBuilder;
struct ComputePassPipelineRuntime;
struct DirectShadowSignalResources;
struct FrameContext;
struct PassExecutionContext;
struct PassRuntimeServices;
struct RenderPassDefinition;
struct RenderViewData;

struct DirectLightReservoirTemporalPassParameters
{
	ShaderRWTexture2D<void> TemporalReservoirSample;
	ShaderRWTexture2D<void> TemporalReservoirWeight;
	ShaderTexture2D<void> PreviousReservoirSample;
	ShaderTexture2D<void> PreviousReservoirWeight;
	ShaderTexture2D<void> PreviousReservoirSurface;
	ShaderTexture2D<void> GBufferBaseColor;
	ShaderTexture2D<void> GBufferNormal;
	ShaderTexture2D<void> GBufferMaterial;
	ShaderTexture2D<void> GBufferSubsurface;
	ShaderTexture2D<void> GBufferDeviceZ;
	ShaderTexture2D<void> GBufferMotionVector;
	ShaderUniform<PerFrameConstantBufferData> PerFrame;
	ShaderUniform<PerViewConstantBufferData> PerView;
	ShaderUniform<PerTemporalConstantBufferData> PerTemporal;
	ShaderUniform<ViewLightingData> ViewLighting;
	ShaderBufferSRV DirectionalLights;
	ShaderBufferSRV PointLights;
	ShaderBufferSRV SpotLights;
	ShaderBufferSRV RectLights;

	static void Describe(ShaderParameterStructBuilder<DirectLightReservoirTemporalPassParameters>& builder)
	{
		builder.RWTexture(
		    "TemporalReservoirSample",
		    &DirectLightReservoirTemporalPassParameters::TemporalReservoirSample,
		    ShaderStageVisibility::Compute);
		builder.RWTexture(
		    "TemporalReservoirWeight",
		    &DirectLightReservoirTemporalPassParameters::TemporalReservoirWeight,
		    ShaderStageVisibility::Compute);
		builder.ReadTexture(
		    "PreviousReservoirSample",
		    &DirectLightReservoirTemporalPassParameters::PreviousReservoirSample,
		    ShaderStageVisibility::Compute);
		builder.ReadTexture(
		    "PreviousReservoirWeight",
		    &DirectLightReservoirTemporalPassParameters::PreviousReservoirWeight,
		    ShaderStageVisibility::Compute);
		builder.ReadTexture(
		    "PreviousReservoirSurface",
		    &DirectLightReservoirTemporalPassParameters::PreviousReservoirSurface,
		    ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferBaseColor", &DirectLightReservoirTemporalPassParameters::GBufferBaseColor, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferNormal", &DirectLightReservoirTemporalPassParameters::GBufferNormal, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferMaterial", &DirectLightReservoirTemporalPassParameters::GBufferMaterial, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferSubsurface", &DirectLightReservoirTemporalPassParameters::GBufferSubsurface, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferDeviceZ", &DirectLightReservoirTemporalPassParameters::GBufferDeviceZ, ShaderStageVisibility::Compute);
		builder.ReadTexture(
		    "GBufferMotionVector",
		    &DirectLightReservoirTemporalPassParameters::GBufferMotionVector,
		    ShaderStageVisibility::Compute);
		builder.Uniform("PerFrame", &DirectLightReservoirTemporalPassParameters::PerFrame, ShaderStageVisibility::Compute);
		builder.Uniform("PerView", &DirectLightReservoirTemporalPassParameters::PerView, ShaderStageVisibility::Compute);
		builder.Uniform("PerTemporal", &DirectLightReservoirTemporalPassParameters::PerTemporal, ShaderStageVisibility::Compute);
		builder.Uniform("ViewLighting", &DirectLightReservoirTemporalPassParameters::ViewLighting, ShaderStageVisibility::Compute);
		builder.ReadBuffer("DirectionalLights", &DirectLightReservoirTemporalPassParameters::DirectionalLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("PointLights", &DirectLightReservoirTemporalPassParameters::PointLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("SpotLights", &DirectLightReservoirTemporalPassParameters::SpotLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RectLights", &DirectLightReservoirTemporalPassParameters::RectLights, ShaderStageVisibility::Compute);
	}
};

struct DirectLightReservoirSpatialPassParameters
{
	ShaderTexture2D<void> TemporalReservoirSample;
	ShaderTexture2D<void> TemporalReservoirWeight;
	ShaderRWTexture2D<void> CurrentReservoirSample;
	ShaderRWTexture2D<void> CurrentReservoirWeight;
	ShaderRWTexture2D<void> CurrentReservoirSurface;
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

	static void Describe(ShaderParameterStructBuilder<DirectLightReservoirSpatialPassParameters>& builder)
	{
		builder.ReadTexture(
		    "TemporalReservoirSample",
		    &DirectLightReservoirSpatialPassParameters::TemporalReservoirSample,
		    ShaderStageVisibility::Compute);
		builder.ReadTexture(
		    "TemporalReservoirWeight",
		    &DirectLightReservoirSpatialPassParameters::TemporalReservoirWeight,
		    ShaderStageVisibility::Compute);
		builder.RWTexture(
		    "CurrentReservoirSample",
		    &DirectLightReservoirSpatialPassParameters::CurrentReservoirSample,
		    ShaderStageVisibility::Compute);
		builder.RWTexture(
		    "CurrentReservoirWeight",
		    &DirectLightReservoirSpatialPassParameters::CurrentReservoirWeight,
		    ShaderStageVisibility::Compute);
		builder.RWTexture(
		    "CurrentReservoirSurface",
		    &DirectLightReservoirSpatialPassParameters::CurrentReservoirSurface,
		    ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferBaseColor", &DirectLightReservoirSpatialPassParameters::GBufferBaseColor, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferNormal", &DirectLightReservoirSpatialPassParameters::GBufferNormal, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferMaterial", &DirectLightReservoirSpatialPassParameters::GBufferMaterial, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferSubsurface", &DirectLightReservoirSpatialPassParameters::GBufferSubsurface, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferDeviceZ", &DirectLightReservoirSpatialPassParameters::GBufferDeviceZ, ShaderStageVisibility::Compute);
		builder.Uniform("PerFrame", &DirectLightReservoirSpatialPassParameters::PerFrame, ShaderStageVisibility::Compute);
		builder.Uniform("PerView", &DirectLightReservoirSpatialPassParameters::PerView, ShaderStageVisibility::Compute);
		builder.Uniform("ViewLighting", &DirectLightReservoirSpatialPassParameters::ViewLighting, ShaderStageVisibility::Compute);
		builder.ReadBuffer("DirectionalLights", &DirectLightReservoirSpatialPassParameters::DirectionalLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("PointLights", &DirectLightReservoirSpatialPassParameters::PointLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("SpotLights", &DirectLightReservoirSpatialPassParameters::SpotLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RectLights", &DirectLightReservoirSpatialPassParameters::RectLights, ShaderStageVisibility::Compute);
	}
};

class DirectLightReservoirTemporalPass final
{
  public:
	static constexpr const char* PassName = "DirectLightReservoirTemporal";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	using Parameters = DirectLightReservoirTemporalPassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

	explicit DirectLightReservoirTemporalPass(const ComputePassPipelineRuntime& runtime) noexcept;

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
	    const PassRuntimeServices& passRuntimeServices) const;

	const ComputePassPipelineRuntime& m_runtime;
};

class DirectLightReservoirSpatialPass final
{
  public:
	static constexpr const char* PassName = "DirectLightReservoirSpatial";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	using Parameters = DirectLightReservoirSpatialPassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

	explicit DirectLightReservoirSpatialPass(const ComputePassPipelineRuntime& runtime) noexcept;

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
	    const PassRuntimeServices& passRuntimeServices) const;

	const ComputePassPipelineRuntime& m_runtime;
};

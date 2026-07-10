#pragma once

#include "Passes/Deferred/DirectLightReservoirPassCommon.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"

#include <cstdint>

class FrameGraphBuilder;
struct ComputePassPipelineRuntime;
struct DirectShadowSignalResources;
struct FrameContext;
struct PassExecutionContext;
struct PassRuntimeServices;
struct RenderPassDefinition;
struct RenderViewData;

struct DirectLightReservoirSpatialPassParameters : DirectLightReservoirCommonParameters
{
	ShaderTexture2D<void> TemporalReservoirSample;
	ShaderTexture2D<void> TemporalReservoirWeight;
	ShaderRWTexture2D<void> CurrentReservoirSample;
	ShaderRWTexture2D<void> CurrentReservoirWeight;
	ShaderRWTexture2D<void> CurrentReservoirSurface;

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
		DescribeGBuffer(builder);
		DescribeFrame(builder);
		DescribeLighting(builder);
	}
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
	    FrameGraphTextureHandle sceneDepth,
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

#pragma once

#include "Passes/Deferred/DirectLightReservoirPassCommon.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"

#include <cstdint>

struct ComputePassPipelineRuntime;
struct DirectShadowSignalResources;
struct FrameContext;
struct PassExecutionContext;
struct PassRuntimeContext;
struct RenderPassDefinition;
struct RenderViewData;

struct DirectLightReservoirTemporalPassParameters : DirectLightReservoirCommonParameters
{
	ShaderRWTexture2D<void> TemporalReservoirSample;
	ShaderRWTexture2D<void> TemporalReservoirWeight;
	ShaderTexture2D<void> PreviousReservoirSample;
	ShaderTexture2D<void> PreviousReservoirWeight;
	ShaderTexture2D<void> PreviousReservoirSurface;
	ShaderTexture2D<void> GBufferMotionVector;
	ShaderUniform<PerTemporalConstantBufferData> PerTemporal;

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
		DescribeGBuffer(builder);
		builder.ReadTexture(
		    "GBufferMotionVector",
		    &DirectLightReservoirTemporalPassParameters::GBufferMotionVector,
		    ShaderStageVisibility::Compute);
		DescribeFrame(builder);
		builder.Uniform("PerTemporal", &DirectLightReservoirTemporalPassParameters::PerTemporal, ShaderStageVisibility::Compute);
		DescribeLighting(builder);
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
	void Execute(PassExecutionContext& context, ParameterInstance& parameters) const;

  private:
	void SetParameters(
	    ParameterInstance& parameters,
	    const FrameContext& frame,
	    const RenderViewData& viewData,
	    const PassRuntimeContext& passRuntimeContext) const;

	const ComputePassPipelineRuntime& m_runtime;
};

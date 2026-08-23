#pragma once

#include "Passes/Lighting/Direct/DirectLightReservoirPassCommon.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"
#include "ShaderData/ViewTemporalUniformData.h"

#include <cstdint>

struct ComputePassPipelineRuntime;
struct DirectShadowSignalResources;
struct PassCommandContext;
struct RenderPassDefinition;

struct DirectLightReservoirSpatialPassParameters : DirectLightReservoirCommonParameters
{
	ShaderTexture2D<void> TemporalReservoirSample;
	ShaderTexture2D<void> TemporalReservoirWeight;
	ShaderRWTexture2D<void> CurrentReservoirSample;
	ShaderRWTexture2D<void> CurrentReservoirWeight;
	ShaderRWTexture2D<void> CurrentReservoirSurface;
	ShaderUniform<ViewTemporalUniformData> ViewTemporal;

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
		builder.Uniform("ViewTemporal", &DirectLightReservoirSpatialPassParameters::ViewTemporal, ShaderStageVisibility::Compute);
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
	void Execute(PassCommandContext& context, ParameterInstance& parameters, std::uint32_t outputWidth, std::uint32_t outputHeight) const;

private:
	const ComputePassPipelineRuntime& m_runtime;
};

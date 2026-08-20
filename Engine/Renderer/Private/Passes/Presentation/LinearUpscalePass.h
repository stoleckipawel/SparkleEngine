#pragma once

#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"

#include <cstdint>

struct ComputePassPipelineRuntime;
struct PassCommandContext;
struct RenderPassDefinition;

struct LinearUpscalePassParameters
{
	ShaderRWTexture2D<void> ScalingOutputColor;
	ShaderTexture2D<void> ScalingInputColor;
	ShaderSamplerSet SamplerLinearClamp;

	static void Describe(ShaderParameterStructBuilder<LinearUpscalePassParameters>& builder)
	{
		builder.RWTexture("ScalingOutputColor", &LinearUpscalePassParameters::ScalingOutputColor, ShaderStageVisibility::Compute);
		builder.ReadTexture("ScalingInputColor", &LinearUpscalePassParameters::ScalingInputColor, ShaderStageVisibility::Compute);
		builder.Sampler("SamplerLinearClamp", &LinearUpscalePassParameters::SamplerLinearClamp, ShaderStageVisibility::Compute);
	}
};

class LinearUpscalePass final
{
public:
	static constexpr const char* PassName = "LinearUpscale";
	static constexpr std::uint32_t ThreadGroupSizeX = 8u;
	static constexpr std::uint32_t ThreadGroupSizeY = 8u;
	using Parameters = LinearUpscalePassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

	explicit LinearUpscalePass(const ComputePassPipelineRuntime& runtime) noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	void Execute(PassCommandContext& context, ParameterInstance& parameters, std::uint32_t outputWidth, std::uint32_t outputHeight) const;

private:
	const ComputePassPipelineRuntime& m_runtime;
};

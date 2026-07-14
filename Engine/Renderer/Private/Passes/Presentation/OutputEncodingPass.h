#pragma once

#include "Frame/Presentation/OutputEncodingUniformData.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"

#include <cstdint>

struct PassExecutionContext;
struct ComputePassPipelineRuntime;
struct RenderPassDefinition;

struct OutputEncodingPassParameters
{
	ShaderRWTexture2D<void> EncodedColor;
	ShaderTexture2D<void> DisplayLinearColor;
	ShaderUniform<OutputEncodingUniformData> OutputEncodingConstants;

	static void Describe(ShaderParameterStructBuilder<OutputEncodingPassParameters>& builder)
	{
		builder.RWTexture("EncodedColor", &OutputEncodingPassParameters::EncodedColor, ShaderStageVisibility::Compute);
		builder.ReadTexture("DisplayLinearColor", &OutputEncodingPassParameters::DisplayLinearColor, ShaderStageVisibility::Compute);
		builder.Uniform(
		    "OutputEncodingConstants",
		    &OutputEncodingPassParameters::OutputEncodingConstants,
		    ShaderStageVisibility::Compute);
	}
};

class OutputEncodingPass final
{
  public:
	static constexpr const char* PassName = "OutputEncoding";
	static constexpr std::uint32_t ThreadGroupSizeX = 8u;
	static constexpr std::uint32_t ThreadGroupSizeY = 8u;
	using Parameters = OutputEncodingPassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

	explicit OutputEncodingPass(const ComputePassPipelineRuntime& runtime) noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	void Execute(
	    PassExecutionContext& context,
	    ParameterInstance& parameters,
	    std::uint32_t outputWidth,
	    std::uint32_t outputHeight) const;

  private:
	const ComputePassPipelineRuntime& m_runtime;
};

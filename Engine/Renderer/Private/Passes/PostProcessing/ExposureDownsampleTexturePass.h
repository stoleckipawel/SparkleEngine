#pragma once

#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"

#include <cstdint>

struct ComputePassPipelineRuntime;
struct PassExecutionContext;
struct RenderPassDefinition;

struct ExposureDownsampleTexturePassParameters
{
	ShaderTexture2D<void> LuminanceMomentsInput;
	ShaderRWTexture2D<void> LuminanceMomentsOutput;

	static void Describe(ShaderParameterStructBuilder<ExposureDownsampleTexturePassParameters>& builder)
	{
		builder.ReadTexture(
		    "LuminanceMomentsInput",
		    &ExposureDownsampleTexturePassParameters::LuminanceMomentsInput,
		    ShaderStageVisibility::Compute);
		builder.RWTexture(
		    "LuminanceMomentsOutput",
		    &ExposureDownsampleTexturePassParameters::LuminanceMomentsOutput,
		    ShaderStageVisibility::Compute);
	}
};

class ExposureDownsampleTexturePass final
{
  public:
	static constexpr const char* PassName = "ExposureDownsampleTexture";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	using Parameters = ExposureDownsampleTexturePassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

	explicit ExposureDownsampleTexturePass(const ComputePassPipelineRuntime& runtime) noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	void Execute(PassExecutionContext& context, ParameterInstance& parameters, std::uint32_t outputWidth, std::uint32_t outputHeight) const;

  private:
	const ComputePassPipelineRuntime& m_runtime;
};

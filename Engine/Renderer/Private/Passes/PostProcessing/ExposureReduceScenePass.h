#pragma once

#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"

#include <cstdint>

struct ComputePassPipelineRuntime;
struct PassExecutionContext;
struct RenderPassDefinition;

struct ExposureReduceScenePassParameters
{
	ShaderTexture2D<void> SceneColor;
	ShaderRWTexture2D<void> LuminanceMomentsOutput;

	static void Describe(ShaderParameterStructBuilder<ExposureReduceScenePassParameters>& builder)
	{
		builder.ReadTexture("SceneColor", &ExposureReduceScenePassParameters::SceneColor, ShaderStageVisibility::Compute);
		builder.RWTexture(
		    "LuminanceMomentsOutput",
		    &ExposureReduceScenePassParameters::LuminanceMomentsOutput,
		    ShaderStageVisibility::Compute);
	}
};

class ExposureReduceScenePass final
{
  public:
	static constexpr const char* PassName = "ExposureReduceScene";
	static constexpr std::uint32_t ThreadGroupSizeX = 16;
	static constexpr std::uint32_t ThreadGroupSizeY = 16;
	using Parameters = ExposureReduceScenePassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

	explicit ExposureReduceScenePass(const ComputePassPipelineRuntime& runtime) noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	void Execute(PassExecutionContext& context, ParameterInstance& parameters, std::uint32_t outputWidth, std::uint32_t outputHeight) const;

  private:
	const ComputePassPipelineRuntime& m_runtime;
};

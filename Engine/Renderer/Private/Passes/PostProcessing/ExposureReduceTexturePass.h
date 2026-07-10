#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"

#include <cstdint>

class FrameGraphBuilder;
struct ComputePassPipelineRuntime;
struct PassExecutionContext;
struct RenderPassDefinition;

struct ExposureReduceTexturePassParameters
{
	ShaderTexture2D<void> LuminanceMomentsInput;
	ShaderRWTexture2D<void> LuminanceMomentsOutput;

	static void Describe(ShaderParameterStructBuilder<ExposureReduceTexturePassParameters>& builder)
	{
		builder.ReadTexture(
		    "LuminanceMomentsInput",
		    &ExposureReduceTexturePassParameters::LuminanceMomentsInput,
		    ShaderStageVisibility::Compute);
		builder.RWTexture(
		    "LuminanceMomentsOutput",
		    &ExposureReduceTexturePassParameters::LuminanceMomentsOutput,
		    ShaderStageVisibility::Compute);
	}
};

class ExposureReduceTexturePass final
{
  public:
	static constexpr const char* PassName = "ExposureReduceTexture";
	static constexpr std::uint32_t ThreadGroupSizeX = 16;
	static constexpr std::uint32_t ThreadGroupSizeY = 16;
	using Parameters = ExposureReduceTexturePassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

	explicit ExposureReduceTexturePass(const ComputePassPipelineRuntime& runtime) noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	static void DeclareResources(
	    FrameGraphBuilder& builder,
	    FrameGraphTextureHandle inputMoments,
	    FrameGraphTextureHandle outputMoments,
	    ParameterInstance& parameters);
	void Execute(PassExecutionContext& context, ParameterInstance& parameters, std::uint32_t outputWidth, std::uint32_t outputHeight) const;

  private:
	const ComputePassPipelineRuntime& m_runtime;
};

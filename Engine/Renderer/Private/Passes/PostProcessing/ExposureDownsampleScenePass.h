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

struct ExposureDownsampleScenePassParameters
{
	ShaderTexture2D<void> SceneColor;
	ShaderRWTexture2D<void> LuminanceMomentsOutput;

	static void Describe(ShaderParameterStructBuilder<ExposureDownsampleScenePassParameters>& builder)
	{
		builder.ReadTexture("SceneColor", &ExposureDownsampleScenePassParameters::SceneColor, ShaderStageVisibility::Compute);
		builder.RWTexture(
		    "LuminanceMomentsOutput",
		    &ExposureDownsampleScenePassParameters::LuminanceMomentsOutput,
		    ShaderStageVisibility::Compute);
	}
};

class ExposureDownsampleScenePass final
{
  public:
	static constexpr const char* PassName = "ExposureDownsampleScene";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	using Parameters = ExposureDownsampleScenePassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

	explicit ExposureDownsampleScenePass(const ComputePassPipelineRuntime& runtime) noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	static void DeclareResources(
	    FrameGraphBuilder& builder,
	    FrameGraphTextureHandle finalSceneColor,
	    FrameGraphTextureHandle luminanceMoments,
	    ParameterInstance& parameters);
	void Execute(PassExecutionContext& context, ParameterInstance& parameters, std::uint32_t outputWidth, std::uint32_t outputHeight) const;

  private:
	const ComputePassPipelineRuntime& m_runtime;
};

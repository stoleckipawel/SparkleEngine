#pragma once

#include "Frame/PostProcessing/ExposureUniformData.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"

#include <cstdint>

class FrameGraphBuilder;
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

struct ExposurePassParameters
{
	ShaderRWTexture2D<void> ExposureTexture;
	ShaderRWTexture2D<void> ExposureHistoryTexture;
	ShaderTexture2D<void> PreviousExposureTexture;
	ShaderTexture2D<void> LuminanceMoments;
	ShaderUniform<ExposureUniformData> ExposureConstants;

	static void Describe(ShaderParameterStructBuilder<ExposurePassParameters>& builder)
	{
		builder.RWTexture("ExposureTexture", &ExposurePassParameters::ExposureTexture, ShaderStageVisibility::Compute);
		builder.RWTexture("ExposureHistoryTexture", &ExposurePassParameters::ExposureHistoryTexture, ShaderStageVisibility::Compute);
		builder.ReadTexture("PreviousExposureTexture", &ExposurePassParameters::PreviousExposureTexture, ShaderStageVisibility::Compute);
		builder.ReadTexture("LuminanceMoments", &ExposurePassParameters::LuminanceMoments, ShaderStageVisibility::Compute);
		builder.Uniform("ExposureConstants", &ExposurePassParameters::ExposureConstants, ShaderStageVisibility::Compute);
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
	static void DeclareResources(
	    FrameGraphBuilder& builder,
	    FrameGraphTextureHandle finalSceneColor,
	    FrameGraphTextureHandle luminanceMoments,
	    ParameterInstance& parameters);
	void Execute(PassExecutionContext& context, ParameterInstance& parameters, std::uint32_t outputWidth, std::uint32_t outputHeight) const;

  private:
	const ComputePassPipelineRuntime& m_runtime;
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
	static void DeclareResources(
	    FrameGraphBuilder& builder,
	    FrameGraphTextureHandle inputMoments,
	    FrameGraphTextureHandle outputMoments,
	    ParameterInstance& parameters);
	void Execute(PassExecutionContext& context, ParameterInstance& parameters, std::uint32_t outputWidth, std::uint32_t outputHeight) const;

  private:
	const ComputePassPipelineRuntime& m_runtime;
};

class ExposurePass final
{
  public:
	static constexpr const char* PassName = "Exposure";
	using Parameters = ExposurePassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

	explicit ExposurePass(const ComputePassPipelineRuntime& runtime) noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	static void DeclareResources(
	    FrameGraphBuilder& builder,
	    FrameGraphTextureHandle luminanceMoments,
	    FrameGraphTextureHandle previousExposure,
	    FrameGraphTextureHandle currentExposure,
	    FrameGraphTextureHandle exposure,
	    ParameterInstance& parameters);
	void Execute(PassExecutionContext& context, ParameterInstance& parameters) const;

  private:
	const ComputePassPipelineRuntime& m_runtime;
};

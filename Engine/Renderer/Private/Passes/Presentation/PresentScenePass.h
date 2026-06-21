#pragma once

#include "Frame/Targets/FrameRenderTargets.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"

class FrameGraphBuilder;
struct PassExecutionContext;
struct RasterPassPipelineRuntime;
struct RenderPassDefinition;

struct PresentScenePassParameters
{
	ShaderRenderTarget BackBuffer;
	ShaderTexture2D<void> SceneColor;
	ShaderSamplerSet SamplerLinearClamp;

	static void Describe(ShaderParameterStructBuilder<PresentScenePassParameters>& builder)
	{
		builder.RenderTarget("BackBuffer", &PresentScenePassParameters::BackBuffer, ShaderStageVisibility::AllGraphics);
		builder.ReadTexture("SceneColor", &PresentScenePassParameters::SceneColor, ShaderStageVisibility::Pixel);
		builder.Sampler("SamplerLinearClamp", &PresentScenePassParameters::SamplerLinearClamp, ShaderStageVisibility::Pixel);
	}
};

class PresentScenePass final
{
  public:
	static constexpr const char* PassName = "PresentScene";
	using Parameters = PresentScenePassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = RasterPassPipelineRuntime;

	explicit PresentScenePass(const RasterPassPipelineRuntime& runtime) noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	static void DeclareResources(FrameGraphBuilder& builder, const SceneRenderTargets& sceneTargets, ParameterInstance& parameters);
	void Execute(PassExecutionContext& context, ParameterInstance& parameters) const;

  private:
	const RasterPassPipelineRuntime& m_runtime;
};

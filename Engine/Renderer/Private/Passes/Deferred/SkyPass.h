#pragma once

#include "Frame/Targets/FrameRenderTargets.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"

#include "Passes/Bindings/EnvironmentMapPassBinding.h"
#include "ShaderData/RenderConstantBufferData.h"

#include <cstdint>

class FrameGraphBuilder;
struct RenderPassDefinition;
struct ComputePassPipelineRuntime;
struct PassExecutionContext;
struct PassRuntimeServices;
struct RenderViewData;

struct SkyPassParameters
{
	ShaderRWTexture2D<void> SceneColor;
	ShaderTexture2D<void> SceneDepth;
	ShaderTexture2DSRV SkyTexture;
	ShaderSamplerSet SamplerLinearClamp;
	ShaderUniform<PerFrameConstantBufferData> PerFrame;
	ShaderUniform<PerViewConstantBufferData> PerView;

	static void Describe(ShaderParameterStructBuilder<SkyPassParameters>& builder)
	{
		builder.RWTexture("SceneColor", &SkyPassParameters::SceneColor, ShaderStageVisibility::Compute);
		builder.ReadTexture("SceneDepth", &SkyPassParameters::SceneDepth, ShaderStageVisibility::Compute);
		builder.ReadTexture("SkyTexture", &SkyPassParameters::SkyTexture, ShaderStageVisibility::Compute);
		builder.Sampler("SamplerLinearClamp", &SkyPassParameters::SamplerLinearClamp, ShaderStageVisibility::Compute);
		builder.Uniform("PerFrame", &SkyPassParameters::PerFrame, ShaderStageVisibility::Compute);
		builder.Uniform("PerView", &SkyPassParameters::PerView, ShaderStageVisibility::Compute);
	}
};

class SkyPass final
{
  public:
	static constexpr const char* PassName = "Sky";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	using Parameters = SkyPassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

	explicit SkyPass(const ComputePassPipelineRuntime& runtime) noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	static void DeclareResources(
	    FrameGraphBuilder& builder,
	    const SceneRenderTargets& sceneTargets,
	    ParameterInstance& parameters);
	void Execute(PassExecutionContext& context, ParameterInstance& parameters) const;

  private:
	void SetParameters(
	    ParameterInstance& parameters,
	    const RenderViewData& viewData,
	    const PassRuntimeServices& passRuntimeServices) const;

	const ComputePassPipelineRuntime& m_runtime;
	mutable EnvironmentMapPassBinding m_environmentMapBinding;
};

#pragma once

#include "Frame/Presentation/ToneMappingUniformData.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"

#include <cstdint>

class FrameGraphBuilder;
struct PassExecutionContext;
struct ComputePassPipelineRuntime;
struct RenderPassDefinition;

struct ToneMappingPassParameters
{
	ShaderRWTexture2D<void> ToneMappedColor;
	ShaderTexture2D<void> SceneColor;
	ShaderTexture2D<void> ExposureTexture;
	ShaderSamplerSet SamplerLinearClamp;
	ShaderUniform<ToneMappingUniformData> ToneMappingConstants;

	static void Describe(ShaderParameterStructBuilder<ToneMappingPassParameters>& builder)
	{
		builder.RWTexture("ToneMappedColor", &ToneMappingPassParameters::ToneMappedColor, ShaderStageVisibility::Compute);
		builder.ReadTexture("SceneColor", &ToneMappingPassParameters::SceneColor, ShaderStageVisibility::Compute);
		builder.ReadTexture("ExposureTexture", &ToneMappingPassParameters::ExposureTexture, ShaderStageVisibility::Compute);
		builder.Sampler("SamplerLinearClamp", &ToneMappingPassParameters::SamplerLinearClamp, ShaderStageVisibility::Compute);
		builder.Uniform("ToneMappingConstants", &ToneMappingPassParameters::ToneMappingConstants, ShaderStageVisibility::Compute);
	}
};

class ToneMappingPass final
{
  public:
	static constexpr const char* PassName = "ToneMapping";
	static constexpr std::uint32_t ThreadGroupSizeX = 8u;
	static constexpr std::uint32_t ThreadGroupSizeY = 8u;
	using Parameters = ToneMappingPassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

	explicit ToneMappingPass(const ComputePassPipelineRuntime& runtime) noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	static void DeclareResources(
	    FrameGraphBuilder& builder,
	    FrameGraphTextureHandle sceneColor,
	    FrameGraphTextureHandle exposure,
	    FrameGraphTextureHandle toneMappedColor,
	    ParameterInstance& parameters);
	void Execute(
	    PassExecutionContext& context,
	    ParameterInstance& parameters,
	    std::uint32_t outputWidth,
	    std::uint32_t outputHeight) const;

  private:
	const ComputePassPipelineRuntime& m_runtime;
};

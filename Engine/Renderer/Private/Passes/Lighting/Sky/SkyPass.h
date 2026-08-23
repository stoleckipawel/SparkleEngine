#pragma once

#include "Frame/Graph/RenderFrameGraphTargets.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"

#include "ShaderData/ViewUniformData.h"
#include "ShaderData/ViewCameraUniformData.h"
#include "ShaderData/ViewTemporalUniformData.h"
#include "ShaderData/SkyUniformData.h"

#include <cstdint>

struct RenderPassDefinition;
struct ComputePassPipelineRuntime;
struct PassCommandContext;

struct SkyPassParameters
{
	ShaderRWTexture2D<void> SceneColor;
	ShaderTexture2D<void> SceneDepth;
	ShaderTexture2D<void> SkyTexture;
	ShaderSamplerSet SamplerLinearClamp;
	ShaderUniform<ViewUniformData> View;
	ShaderUniform<ViewCameraUniformData> ViewCamera;
	ShaderUniform<ViewTemporalUniformData> ViewTemporal;
	ShaderUniform<SkyUniformData> Sky;

	static void Describe(ShaderParameterStructBuilder<SkyPassParameters>& builder)
	{
		builder.RWTexture("SceneColor", &SkyPassParameters::SceneColor, ShaderStageVisibility::Compute);
		builder.ReadTexture("SceneDepth", &SkyPassParameters::SceneDepth, ShaderStageVisibility::Compute);
		builder.ReadTexture("SkyTexture", &SkyPassParameters::SkyTexture, ShaderStageVisibility::Compute);
		builder.Sampler("SamplerLinearClamp", &SkyPassParameters::SamplerLinearClamp, ShaderStageVisibility::Compute);
		builder.Uniform("View", &SkyPassParameters::View, ShaderStageVisibility::Compute);
		builder.Uniform("ViewCamera", &SkyPassParameters::ViewCamera, ShaderStageVisibility::Compute);
		builder.Uniform("ViewTemporal", &SkyPassParameters::ViewTemporal, ShaderStageVisibility::Compute);
		builder.Uniform("Sky", &SkyPassParameters::Sky, ShaderStageVisibility::Compute);
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
	void Execute(PassCommandContext& context, ParameterInstance& parameters, std::uint32_t outputWidth, std::uint32_t outputHeight) const;

private:
	const ComputePassPipelineRuntime& m_runtime;
};

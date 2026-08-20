#pragma once

#include "Frame/PostProcessing/ExposureUniformData.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"

struct ComputePassPipelineRuntime;
struct PassCommandContext;
struct RenderPassDefinition;

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
	void Execute(PassCommandContext& context, ParameterInstance& parameters) const;

private:
	const ComputePassPipelineRuntime& m_runtime;
};

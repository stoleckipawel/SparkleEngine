#pragma once

#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"

#include "ShaderData/RenderConstantBufferData.h"

#include <cstdint>

struct ComputePassPipelineRuntime;
struct PassExecutionContext;
struct RenderPassDefinition;

struct SkyMotionVectorPassParameters
{
	ShaderTexture2D<void> GBufferDeviceZ;
	ShaderRWTexture2D<void> GBufferMotionVector;
	ShaderUniform<PerFrameConstantBufferData> PerFrame;
	ShaderUniform<PerViewConstantBufferData> PerView;
	ShaderUniform<PerTemporalConstantBufferData> PerTemporal;

	static void Describe(ShaderParameterStructBuilder<SkyMotionVectorPassParameters>& builder)
	{
		builder.ReadTexture("GBufferDeviceZ", &SkyMotionVectorPassParameters::GBufferDeviceZ, ShaderStageVisibility::Compute);
		builder.RWTexture("GBufferMotionVector", &SkyMotionVectorPassParameters::GBufferMotionVector, ShaderStageVisibility::Compute);
		builder.Uniform("PerFrame", &SkyMotionVectorPassParameters::PerFrame, ShaderStageVisibility::Compute);
		builder.Uniform("PerView", &SkyMotionVectorPassParameters::PerView, ShaderStageVisibility::Compute);
		builder.Uniform("PerTemporal", &SkyMotionVectorPassParameters::PerTemporal, ShaderStageVisibility::Compute);
	}
};

class SkyMotionVectorPass final
{
  public:
	static constexpr const char* PassName = "SkyMotionVector";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	using Parameters = SkyMotionVectorPassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

	explicit SkyMotionVectorPass(const ComputePassPipelineRuntime& runtime) noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	void Execute(PassExecutionContext& context, ParameterInstance& parameters) const;

  private:
	const ComputePassPipelineRuntime& m_runtime;
};

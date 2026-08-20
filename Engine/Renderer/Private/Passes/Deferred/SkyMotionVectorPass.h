#pragma once

#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"

#include "ShaderData/ViewUniformData.h"
#include "ShaderData/ViewCameraUniformData.h"
#include "ShaderData/ViewTemporalUniformData.h"

#include <cstdint>

struct ComputePassPipelineRuntime;
struct PassCommandContext;
struct RenderPassDefinition;

struct SkyMotionVectorPassParameters
{
	ShaderTexture2D<void> GBufferDeviceZ;
	ShaderRWTexture2D<void> GBufferMotionVector;
	ShaderUniform<ViewUniformData> View;
	ShaderUniform<ViewCameraUniformData> ViewCamera;
	ShaderUniform<ViewTemporalUniformData> ViewTemporal;

	static void Describe(ShaderParameterStructBuilder<SkyMotionVectorPassParameters>& builder)
	{
		builder.ReadTexture("GBufferDeviceZ", &SkyMotionVectorPassParameters::GBufferDeviceZ, ShaderStageVisibility::Compute);
		builder.RWTexture("GBufferMotionVector", &SkyMotionVectorPassParameters::GBufferMotionVector, ShaderStageVisibility::Compute);
		builder.Uniform("View", &SkyMotionVectorPassParameters::View, ShaderStageVisibility::Compute);
		builder.Uniform("ViewCamera", &SkyMotionVectorPassParameters::ViewCamera, ShaderStageVisibility::Compute);
		builder.Uniform("ViewTemporal", &SkyMotionVectorPassParameters::ViewTemporal, ShaderStageVisibility::Compute);
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
	void Execute(PassCommandContext& context, ParameterInstance& parameters, std::uint32_t outputWidth, std::uint32_t outputHeight) const;

private:
	const ComputePassPipelineRuntime& m_runtime;
};

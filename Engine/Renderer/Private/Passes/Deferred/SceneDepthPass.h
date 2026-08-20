#pragma once

#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"

#include "ShaderData/ViewCameraUniformData.h"

#include <cstdint>

struct ComputePassPipelineRuntime;
struct PassExecutionContext;
struct RenderPassDefinition;

struct SceneDepthPassParameters
{
	ShaderTexture2D<void> GBufferDeviceZ;
	ShaderRWTexture2D<void> SceneDepth;
	ShaderUniform<ViewCameraUniformData> ViewCamera;

	static void Describe(ShaderParameterStructBuilder<SceneDepthPassParameters>& builder)
	{
		builder.ReadTexture("GBufferDeviceZ", &SceneDepthPassParameters::GBufferDeviceZ, ShaderStageVisibility::Compute);
		builder.RWTexture("SceneDepth", &SceneDepthPassParameters::SceneDepth, ShaderStageVisibility::Compute);
		builder.Uniform("ViewCamera", &SceneDepthPassParameters::ViewCamera, ShaderStageVisibility::Compute);
	}
};

class SceneDepthPass final
{
public:
	static constexpr const char* PassName = "SceneDepth";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	using Parameters = SceneDepthPassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

	explicit SceneDepthPass(const ComputePassPipelineRuntime& runtime) noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	void Execute(PassExecutionContext& context, ParameterInstance& parameters) const;

private:
	const ComputePassPipelineRuntime& m_runtime;
};

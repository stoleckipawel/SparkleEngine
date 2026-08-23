#pragma once

#include "Passes/Lighting/Shadows/DirectShadowSignalPassCommon.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"

#include <cstdint>

struct ComputePassPipelineRuntime;
struct PassCommandContext;
struct RenderPassDefinition;

struct DirectShadowSignalPassParameters : DirectShadowSignalRayQueryPassParameters
{
	ShaderAccelerationStructure SceneTlas;

	static void Describe(ShaderParameterStructBuilder<DirectShadowSignalPassParameters>& builder)
	{
		DirectShadowSignalRayQueryPassParameters::Describe(builder);
		builder.AccelerationStructure("SceneTlas", &DirectShadowSignalPassParameters::SceneTlas, ShaderStageVisibility::Compute);
	}
};

class DirectShadowSignalPass final
{
public:
	static constexpr const char* PassName = "DirectShadowSignal";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	using Parameters = DirectShadowSignalPassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

	explicit DirectShadowSignalPass(const ComputePassPipelineRuntime& runtime) noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	void Execute(PassCommandContext& context, ParameterInstance& parameters, std::uint32_t outputWidth, std::uint32_t outputHeight) const;

private:
	const ComputePassPipelineRuntime& m_runtime;
};

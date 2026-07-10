#pragma once

#include "Passes/Deferred/DirectShadowSignalPassCommon.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"

#include <cstdint>

class FrameGraphBuilder;
struct ComputePassPipelineRuntime;
struct PassExecutionContext;
struct RenderPassDefinition;

struct DirectShadowSignalNoRayQueryPassParameters : DirectShadowSignalCommonPassParameters
{
	static void Describe(ShaderParameterStructBuilder<DirectShadowSignalNoRayQueryPassParameters>& builder)
	{
		DirectShadowSignalCommonPassParameters::Describe(builder);
	}
};

class DirectShadowSignalNoRayQueryPass final
{
  public:
	static constexpr const char* PassName = "DirectShadowSignalNoRayQuery";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	using Parameters = DirectShadowSignalNoRayQueryPassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

	explicit DirectShadowSignalNoRayQueryPass(const ComputePassPipelineRuntime& runtime) noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	static void DeclareResources(
	    FrameGraphBuilder& builder,
	    FrameGraphTextureHandle sceneDepth,
	    const DirectShadowSignalResources& shadowSignals,
	    ParameterInstance& parameters);
	void Execute(PassExecutionContext& context, ParameterInstance& parameters) const;

  private:
	const ComputePassPipelineRuntime& m_runtime;
};

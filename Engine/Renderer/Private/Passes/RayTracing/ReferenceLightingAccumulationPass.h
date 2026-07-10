#pragma once

#include "RayTracing/Effects/ReferenceLighting/ReferenceLightingAccumulationUniformData.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"
#include <cstdint>

class FrameGraphBuilder;
struct ComputePassPipelineRuntime;
struct PassExecutionContext;
struct RenderPassDefinition;

struct ReferenceLightingAccumulationPassParameters
{
	ShaderTexture2D<void> ReferenceLightingSample;
	ShaderRWTexture2D<void> SceneColorTexture;
	ShaderTexture2D<void> PreviousReferenceLighting;
	ShaderRWTexture2D<void> CurrentReferenceLighting;
	ShaderTexture2D<void> ReferenceSampleValidity;
	ShaderTexture2D<void> GBufferMotionVector;
	ShaderUniform<ReferenceLightingAccumulationUniformData> ReferenceLightingAccumulationConstants;

	static void Describe(ShaderParameterStructBuilder<ReferenceLightingAccumulationPassParameters>& builder);
};

class ReferenceLightingAccumulationPass final
{
  public:
	static constexpr const char* PassName = "ReferenceLightingAccumulation";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	using Parameters = ReferenceLightingAccumulationPassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

	explicit ReferenceLightingAccumulationPass(const ComputePassPipelineRuntime& runtime) noexcept;
	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	static void DeclareResources(
	    FrameGraphBuilder& builder,
	    FrameGraphTextureHandle referenceLightingSample,
	    FrameGraphTextureHandle sceneColor,
	    FrameGraphTextureHandle previousReferenceLighting,
	    FrameGraphTextureHandle currentReferenceLighting,
	    FrameGraphTextureHandle referenceSampleValidity,
	    FrameGraphTextureHandle gBufferMotionVector,
	    ParameterInstance& parameters);
	void Execute(PassExecutionContext& context, ParameterInstance& parameters) const;

  private:
	const ComputePassPipelineRuntime& m_runtime;
};

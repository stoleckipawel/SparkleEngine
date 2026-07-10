#pragma once

#include "Passes/RayTracing/RestirIndirectPassCommon.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"

#include <cstdint>

struct RenderPassDefinition;

struct RestirIndirectTemporalPassParameters : RestirIndirectScenePassParameters
{
	ShaderRWTexture2D<void> TemporalReservoirSampleTexture;
	ShaderRWTexture2D<void> TemporalReservoirWeightTexture;
	ShaderTexture2D<void> PreviousReservoirSampleTexture;
	ShaderTexture2D<void> PreviousReservoirWeightTexture;
	ShaderTexture2D<void> PreviousReservoirSurfaceTexture;
	ShaderTexture2D<void> GBufferMotionVector;
	ShaderUniform<PerTemporalConstantBufferData> PerTemporal;
	static void Describe(ShaderParameterStructBuilder<RestirIndirectTemporalPassParameters>& builder);
};

class RestirIndirectTemporalPass final : public RestirIndirectPassBase<RestirIndirectTemporalPassParameters>
{
  public:
	explicit RestirIndirectTemporalPass(const ComputePassPipelineRuntime& runtime) noexcept : RestirIndirectPassBase(runtime) {}
	static constexpr const char* PassName = "RestirIndirectTemporal";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	static void DeclareResources(
	    FrameGraphBuilder& builder,
	    const SceneRenderTargets& scene,
	    const GBufferRenderTargets& gbuffer,
	    FrameGraphTextureHandle temporalSample,
	    FrameGraphTextureHandle temporalWeight,
	    FrameGraphTextureHandle previousSample,
	    FrameGraphTextureHandle previousWeight,
	    FrameGraphTextureHandle previousSurface,
	    FrameGraphAccelerationStructureHandle sceneTlas,
	    ParameterInstance& parameters);
	void Execute(PassExecutionContext& context, ParameterInstance& parameters) const;
};

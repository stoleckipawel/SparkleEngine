#pragma once

#include "Passes/RayTracing/RestirIndirectPassCommon.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"

#include <cstdint>

struct RenderPassDefinition;

struct RestirIndirectSpatialPassParameters : RestirIndirectScenePassParameters
{
	ShaderTexture2D<void> TemporalReservoirSampleTexture;
	ShaderTexture2D<void> TemporalReservoirWeightTexture;
	ShaderRWTexture2D<void> CurrentReservoirSampleTexture;
	ShaderRWTexture2D<void> CurrentReservoirWeightTexture;
	ShaderRWTexture2D<void> CurrentReservoirSurfaceTexture;
	static void Describe(ShaderParameterStructBuilder<RestirIndirectSpatialPassParameters>& builder);
};

class RestirIndirectSpatialPass final : public RestirIndirectPassBase<RestirIndirectSpatialPassParameters>
{
  public:
	explicit RestirIndirectSpatialPass(const ComputePassPipelineRuntime& runtime) noexcept : RestirIndirectPassBase(runtime) {}
	static constexpr const char* PassName = "RestirIndirectSpatial";
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
	    FrameGraphTextureHandle currentSample,
	    FrameGraphTextureHandle currentWeight,
	    FrameGraphTextureHandle currentSurface,
	    FrameGraphAccelerationStructureHandle sceneTlas,
	    ParameterInstance& parameters);
	void Execute(PassExecutionContext& context, ParameterInstance& parameters) const;
};

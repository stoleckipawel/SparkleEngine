#pragma once

#include "Passes/RayTracing/RestirIndirectPassCommon.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"

#include <cstdint>

struct RenderPassDefinition;

struct RestirIndirectResolvePassParameters : RestirIndirectScenePassParameters
{
	ShaderTexture2D<void> CurrentReservoirSampleTexture;
	ShaderTexture2D<void> CurrentReservoirWeightTexture;
	ShaderRWTexture2D<void> IndirectDiffuse;
	ShaderRWTexture2D<void> IndirectSpecular;
	ShaderRWTexture2D<void> IndirectDiffuseAlbedo;
	ShaderRWTexture2D<void> IndirectSpecularAlbedo;
	ShaderRWTexture2D<void> IndirectMaterialGuide;
	ShaderRWTexture2D<void> IndirectSpecularSampleGuide;
	static void Describe(ShaderParameterStructBuilder<RestirIndirectResolvePassParameters>& builder);
};

class RestirIndirectResolvePass final : public RestirIndirectPassBase<RestirIndirectResolvePassParameters>
{
  public:
	explicit RestirIndirectResolvePass(const ComputePassPipelineRuntime& runtime) noexcept : RestirIndirectPassBase(runtime) {}
	static constexpr const char* PassName = "RestirIndirectResolve";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	static void DeclareResources(
	    FrameGraphBuilder& builder,
	    const LightingRenderTargets& lighting,
	    const SceneRenderTargets& scene,
	    const GBufferRenderTargets& gbuffer,
	    FrameGraphTextureHandle currentSample,
	    FrameGraphTextureHandle currentWeight,
	    FrameGraphAccelerationStructureHandle sceneTlas,
	    ParameterInstance& parameters);
	void Execute(PassExecutionContext& context, ParameterInstance& parameters) const;
};

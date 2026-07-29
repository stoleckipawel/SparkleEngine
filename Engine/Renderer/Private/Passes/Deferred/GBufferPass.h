#pragma once

#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"
#include "Frame/Targets/FrameRenderTargets.h"

#include "ShaderData/RenderConstantBufferData.h"
#include "ShaderData/MorphTargetShaderData.h"

#include <cstddef>

class RenderCommandContext;
struct RasterPassPipelineRuntime;
struct RenderPassDefinition;
struct PassExecutionContext;
struct RenderSceneData;
struct MeshInstanceBatch;
struct PassRuntimeContext;
struct RenderViewData;
struct FrameContext;
class FrameGraphResourceCommands;

struct GBufferPassParameters
{
	ShaderRenderTarget BaseColor;
	ShaderRenderTarget Normal;
	ShaderRenderTarget Material;
	ShaderRenderTarget Emissive;
	ShaderRenderTarget Subsurface;
	ShaderRenderTarget MotionVector;
	ShaderDepthTarget DeviceZ;
	ShaderUniform<PerFrameConstantBufferData> PerFrame;
	ShaderUniform<PerViewConstantBufferData> PerView;
	ShaderUniform<PerTemporalConstantBufferData> PerTemporal;
	ShaderSamplerSet SamplerAniso16xWrap;
	ShaderBuffer<MeshInstanceData> MeshInstances;
	ShaderBuffer<std::uint32_t> MeshInstanceSlots;
	ShaderBuffer<JointMatrixData> JointMatrices;
	ShaderBuffer<JointMatrixData> PreviousJointMatrices;
	ShaderBuffer<float> MorphWeights;
	ShaderBuffer<float> PreviousMorphWeights;

	static void Describe(
	    ShaderParameterStructBuilder<GBufferPassParameters>& builder);
};

struct GBufferDrawParameters
{
	ShaderUniform<MeshInstanceDrawConstantBufferData> MeshInstanceDraw;
	ShaderBuffer<MeshInstanceData> MeshInstances;
	ShaderBuffer<std::uint32_t> MeshInstanceSlots;
	ShaderBuffer<VertexSkinInfluenceData> SkinInfluences;
	ShaderBuffer<MorphTargetDeltaData> MorphTargetDeltas;
	ShaderBuffer<JointMatrixData> JointMatrices;
	ShaderBuffer<JointMatrixData> PreviousJointMatrices;
	ShaderBuffer<float> MorphWeights;
	ShaderBuffer<float> PreviousMorphWeights;
	ShaderUniform<PerObjectPSConstantBufferData> PerObjectPS;
	ShaderTexture2DSRV TextureBaseColor;
	ShaderTexture2DSRV TextureNormal;
	ShaderTexture2DSRV TextureRoughness;
	ShaderTexture2DSRV TextureMetallic;
	ShaderTexture2DSRV TextureOcclusion;
	ShaderTexture2DSRV TextureEmissive;
	ShaderTexture2DSRV TextureSubsurfaceColor;
	ShaderTexture2DSRV TextureSubsurfaceStrength;

	static void Describe(
	    ShaderParameterStructBuilder<GBufferDrawParameters>& builder);
};

class GBufferPass final
{
  public:
	static constexpr const char* PassName = "GBuffer";
	using Parameters = GBufferPassParameters;
	using DrawParameters = GBufferDrawParameters;

	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using DrawParameterMetadata = ShaderParameterStructMetadata<DrawParameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using DrawParameterInstance = TypedPassParameterInstance<DrawParameters>;
	using PipelineRuntime = RasterPassPipelineRuntime;

	explicit GBufferPass(const RasterPassPipelineRuntime& runtime) noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const DrawParameterMetadata& GetDrawParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	void Execute(PassExecutionContext& context, ParameterInstance& parameters) const;


  private:
	void SetParameters(ParameterInstance& parameters, const RenderViewData& viewData, const PassRuntimeContext& passRuntimeContext) const;
	void PrepareTargets(PassExecutionContext& context, const Parameters& parameters) const;
	void ConfigurePipeline(RenderCommandContext& commandContext, const RenderViewData& viewData) const;
	void BindPassResources(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& commandContext,
	    const ParameterInstance& parameters,
	    const PassRuntimeContext& passRuntimeContext) const;
	void DrawOpaqueMeshes(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& commandContext,
	    const FrameContext& frame,
	    const Parameters& parameters,
	    const PassRuntimeContext& passRuntimeContext) const;

	const RasterPassPipelineRuntime& m_runtime;
};

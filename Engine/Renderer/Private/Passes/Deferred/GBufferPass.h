#pragma once

#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"
#include "Frame/Targets/FrameRenderTargets.h"

#include "ShaderData/ViewUniformData.h"
#include "ShaderData/ViewCameraUniformData.h"
#include "ShaderData/ViewTemporalUniformData.h"
#include "ShaderData/MeshInstanceShaderData.h"
#include "ShaderData/PerObjectConstantBufferData.h"
#include "ShaderData/MorphTargetShaderData.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>

class RenderCommandContext;
class GBufferMeshBatchDrawer;
class GpuMeshCache;
struct RasterPassPipelineRuntime;
struct RenderPassDefinition;
struct PassCommandContext;
struct PreparedRenderScene;
struct MeshInstanceBatch;
struct RenderView;
class FrameGraphResourceCommands;

struct GBufferPassFrameInput final
{
	// These required values are borrowed only between frame-graph setup and the synchronous recording call.
	std::optional<std::reference_wrapper<const PreparedRenderScene>> PreparedScene;
	std::optional<std::reference_wrapper<const RenderView>> View;
};

struct GBufferPassParameters
{
	ShaderRenderTarget BaseColor;
	ShaderRenderTarget Normal;
	ShaderRenderTarget Material;
	ShaderRenderTarget Emissive;
	ShaderRenderTarget Subsurface;
	ShaderRenderTarget MotionVector;
	ShaderDepthTarget DeviceZ;
	ShaderUniform<ViewUniformData> View;
	ShaderUniform<ViewCameraUniformData> ViewCamera;
	ShaderUniform<ViewTemporalUniformData> ViewTemporal;
	ShaderSamplerSet SamplerAniso16xWrap;
	ShaderBuffer<MeshInstanceData> MeshInstances;
	ShaderBuffer<std::uint32_t> MeshInstanceSlots;
	ShaderBuffer<JointMatrixData> JointMatrices;
	ShaderBuffer<JointMatrixData> PreviousJointMatrices;
	ShaderBuffer<float> MorphWeights;
	ShaderBuffer<float> PreviousMorphWeights;

	static void Describe(ShaderParameterStructBuilder<GBufferPassParameters>& builder);
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

	static void Describe(ShaderParameterStructBuilder<GBufferDrawParameters>& builder);
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

	GBufferPass(
	    const RasterPassPipelineRuntime& runtime,
	    GpuMeshCache& gpuMeshCache,
	    const std::shared_ptr<GBufferPassFrameInput>& frameInput) noexcept;
	~GBufferPass() noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const DrawParameterMetadata& GetDrawParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	void Execute(PassCommandContext& context, ParameterInstance& parameters) const;

private:
	void PrepareTargets(PassCommandContext& context, const Parameters& parameters) const;
	void ConfigurePipeline(RenderCommandContext& commandContext, const RenderView& view) const;
	void BindPassResources(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& commandContext,
	    const ParameterInstance& parameters,
	    const RenderView& view) const;
	void DrawOpaqueMeshes(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& commandContext,
	    const PreparedRenderScene& preparedScene,
	    const RenderView& view,
	    const Parameters& parameters) const;

	const RasterPassPipelineRuntime& m_runtime;
	std::shared_ptr<const GBufferMeshBatchDrawer> m_meshBatchDrawer;
	std::shared_ptr<GBufferPassFrameInput> m_frameInput;
};

#pragma once

#include "Passes/GBuffer/GBufferMeshPass.h"
#include "RHI/Public/Pipeline/RhiPipelineDesc.h"

#include <cstdint>

class FrameGraphResourceCommands;
class GpuMesh;
class GpuMeshCache;
class RenderCommandContext;
struct MeshInstanceBatch;
class RasterPassRenderState;
class RenderPassRuntimeCache;
struct GraphicsAttachmentSignature;
struct PreparedRenderScene;
struct RenderView;

class GBufferMeshBatchDrawer final
{
public:
	explicit GBufferMeshBatchDrawer(const GpuMeshCache& gpuMeshCache) noexcept :
	    m_gpuMeshCache(gpuMeshCache)
	{
	}

	void DrawOpaqueMeshes(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& commandContext,
	    const PreparedRenderScene& preparedScene,
	    const RenderView& view,
	    const GBufferMeshPass::Parameters& parameters,
	    const RenderPassRuntimeCache& runtimeCache,
	    const RasterPassRenderState& renderState,
	    const GraphicsAttachmentSignature& attachments,
	    bool wireframe,
	    const GBufferMeshPass::DrawParameterMetadata& drawParameterMetadata) const;
	void MaterializePipelines(
	    const RenderPassRuntimeCache& runtimeCache,
	    const PreparedRenderScene& preparedScene,
	    const RenderView& view,
	    const RasterPassRenderState& renderState,
	    const GraphicsAttachmentSignature& attachments,
	    bool wireframe) const;

private:
	static bool BindMaterial(
	    const PreparedRenderScene& preparedScene,
	    GBufferMeshPass::DrawParameterInstance& drawParameters,
	    std::uint32_t materialSlot);
	static const GpuMesh* ResolveBatch(const RenderView& view, const MeshInstanceBatch& batch, const GpuMeshCache& meshes) noexcept;
	static bool HasValidSkinning(const PreparedRenderScene& preparedScene, const RenderView& view, const MeshInstanceBatch& batch) noexcept;
	static void ConfigureDrawParameters(
	    const GBufferMeshPass::Parameters& passParameters,
	    const MeshInstanceBatch& batch,
	    GBufferMeshPass::DrawParameterInstance& drawParameters);
	static RhiRasterizerState ResolveRasterizerState(
	    const PreparedRenderScene& preparedScene,
	    const MeshInstanceBatch& batch,
	    const GpuMesh& gpuMesh,
	    bool wireframe) noexcept;
	static bool BindBatchPipeline(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& commandContext,
	    const RenderPassRuntimeCache& runtimeCache,
	    const RasterPassRenderState& renderState,
	    const GraphicsAttachmentSignature& attachments,
	    const PreparedRenderScene& preparedScene,
	    const MeshInstanceBatch& batch,
	    GBufferMeshPass::DrawParameterInstance& drawParameters,
	    const GpuMesh& gpuMesh,
	    bool wireframe);
	static void DrawBatch(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& commandContext,
	    const PreparedRenderScene& preparedScene,
	    const RenderView& view,
	    const GBufferMeshPass::Parameters& passParameters,
	    const RenderPassRuntimeCache& runtimeCache,
	    const RasterPassRenderState& renderState,
	    const GraphicsAttachmentSignature& attachments,
	    const GBufferMeshPass::DrawParameterMetadata& drawParameterMetadata,
	    const GpuMesh& gpuMesh,
	    const MeshInstanceBatch& batch,
	    bool wireframe);

	const GpuMeshCache& m_gpuMeshCache;
};

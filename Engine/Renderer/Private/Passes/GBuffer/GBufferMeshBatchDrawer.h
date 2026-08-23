#pragma once

#include "Passes/GBuffer/GBufferMeshPass.h"

#include <cstdint>

class FrameGraphResourceCommands;
class GpuMesh;
class GpuMeshCache;
class RenderCommandContext;
struct MeshInstanceBatch;
struct RasterPassPipelineRuntime;
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
	    const RasterPassPipelineRuntime& runtime,
	    const GBufferMeshPass::DrawParameterMetadata& drawParameterMetadata) const;

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
	static RasterPassPipelineRuntime ResolveBatchRuntime(
	    const PreparedRenderScene& preparedScene,
	    const MeshInstanceBatch& batch,
	    const RasterPassPipelineRuntime& runtime);
	static bool BindBatchPipeline(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& commandContext,
	    const RasterPassPipelineRuntime& runtime,
	    GBufferMeshPass::DrawParameterInstance& drawParameters,
	    const GpuMesh& gpuMesh,
	    std::uint32_t viewModeIndex);
	static void DrawBatch(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& commandContext,
	    const PreparedRenderScene& preparedScene,
	    const RenderView& view,
	    const GBufferMeshPass::Parameters& passParameters,
	    const RasterPassPipelineRuntime& runtime,
	    const GBufferMeshPass::DrawParameterMetadata& drawParameterMetadata,
	    const GpuMesh& gpuMesh,
	    const MeshInstanceBatch& batch,
	    std::uint32_t viewModeIndex);

	const GpuMeshCache& m_gpuMeshCache;
};

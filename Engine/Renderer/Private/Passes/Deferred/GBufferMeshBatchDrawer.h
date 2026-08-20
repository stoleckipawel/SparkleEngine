#pragma once

#include "Passes/Deferred/GBufferPass.h"

#include <cstdint>

class FrameGraphResourceCommands;
class GpuMesh;
class GpuMeshCache;
class RenderCommandContext;
class RenderHardwareInterface;
struct FrameContext;
struct MeshInstanceBatch;
struct PassRuntimeContext;
struct RasterPassPipelineRuntime;
struct PreparedRenderScene;
struct RenderView;

class GBufferMeshBatchDrawer final
{
public:
	static void DrawOpaqueMeshes(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& commandContext,
	    const FrameContext& frame,
	    const GBufferPass::Parameters& parameters,
	    const PassRuntimeContext& passRuntimeContext,
	    const RasterPassPipelineRuntime& runtime,
	    const GBufferPass::DrawParameterMetadata& drawParameterMetadata);

private:
	static bool BindMaterial(
	    const PreparedRenderScene& preparedScene,
	    GBufferPass::DrawParameterInstance& drawParameters,
	    std::uint32_t materialSlot);
	static const GpuMesh* ResolveBatch(const RenderView& view, const MeshInstanceBatch& batch, const GpuMeshCache& meshes) noexcept;
	static bool HasValidSkinning(const PreparedRenderScene& preparedScene, const RenderView& view, const MeshInstanceBatch& batch) noexcept;
	static void ConfigureDrawParameters(
	    const GBufferPass::Parameters& passParameters,
	    const MeshInstanceBatch& batch,
	    GBufferPass::DrawParameterInstance& drawParameters);
	static RasterPassPipelineRuntime ResolveBatchRuntime(
	    const PreparedRenderScene& preparedScene,
	    const MeshInstanceBatch& batch,
	    const RasterPassPipelineRuntime& runtime);
	static bool BindBatchPipeline(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& commandContext,
	    RenderHardwareInterface& renderHardwareInterface,
	    const RasterPassPipelineRuntime& runtime,
	    GBufferPass::DrawParameterInstance& drawParameters,
	    const GpuMesh& gpuMesh,
	    std::uint32_t viewModeIndex);
	static void DrawBatch(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& commandContext,
	    const FrameContext& frame,
	    const GBufferPass::Parameters& passParameters,
	    RenderHardwareInterface& renderHardwareInterface,
	    const RasterPassPipelineRuntime& runtime,
	    const GBufferPass::DrawParameterMetadata& drawParameterMetadata,
	    const GpuMesh& gpuMesh,
	    const MeshInstanceBatch& batch,
	    std::uint32_t viewModeIndex);
};

#pragma once

#include "Passes/Deferred/GBufferPass.h"

#include <cstdint>

class FrameGraphResourceCommands;
class GPUMesh;
class GPUMeshCache;
class RenderCommandContext;
class RenderHardwareInterface;
struct FrameContext;
struct MeshInstanceBatch;
struct PassRuntimeServices;
struct RasterPassPipelineRuntime;
struct RenderSceneData;

class GBufferMeshBatchDrawer final
{
  public:
	static void DrawOpaqueMeshes(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& cmd,
	    const FrameContext& frame,
	    const GBufferPass::Parameters& parameters,
	    const PassRuntimeServices& passRuntimeServices,
	    const RasterPassPipelineRuntime& runtime,
	    const GBufferPass::DrawParameterMetadata& drawParameterMetadata);

  private:
	static bool BindMaterial(
	    const RenderSceneData& sceneData,
	    GBufferPass::DrawParameterInstance& drawParameters,
	    std::uint32_t materialSlot);
	static const GPUMesh* ResolveBatch(
	    const RenderSceneData& sceneData,
	    const MeshInstanceBatch& batch,
	    const GPUMeshCache& meshes) noexcept;
	static bool HasValidSkinning(
	    const RenderSceneData& sceneData,
	    const MeshInstanceBatch& batch) noexcept;
	static void ConfigureDrawParameters(
	    const GBufferPass::Parameters& passParameters,
	    const MeshInstanceBatch& batch,
	    GBufferPass::DrawParameterInstance& drawParameters);
	static RasterPassPipelineRuntime ResolveBatchRuntime(
	    const RenderSceneData& sceneData,
	    const MeshInstanceBatch& batch,
	    const RasterPassPipelineRuntime& runtime);
	static bool BindBatchPipeline(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& cmd,
	    RenderHardwareInterface& renderHardwareInterface,
	    const RasterPassPipelineRuntime& runtime,
	    GBufferPass::DrawParameterInstance& drawParameters,
	    const GPUMesh& gpuMesh,
	    std::uint32_t viewModeIndex);
	static void DrawBatch(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& cmd,
	    const FrameContext& frame,
	    const GBufferPass::Parameters& passParameters,
	    RenderHardwareInterface& renderHardwareInterface,
	    const RasterPassPipelineRuntime& runtime,
	    const GBufferPass::DrawParameterMetadata& drawParameterMetadata,
	    const GPUMesh& gpuMesh,
	    const MeshInstanceBatch& batch,
	    std::uint32_t viewModeIndex);
};

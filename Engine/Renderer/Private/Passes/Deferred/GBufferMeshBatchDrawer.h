#pragma once

#include "Passes/Deferred/GBufferPass.h"

class FrameGraphResourceCommands;
class RenderCommandContext;
struct FrameContext;
struct PassRuntimeServices;
struct RasterPassPipelineRuntime;

class GBufferMeshBatchDrawer final
{
  public:
	static void DrawOpaqueMeshes(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& cmd,
	    const FrameContext& frame,
	    const PassRuntimeServices& passRuntimeServices,
	    const RasterPassPipelineRuntime& runtime,
	    const GBufferPass::DrawParameterMetadata& drawParameterMetadata);
};

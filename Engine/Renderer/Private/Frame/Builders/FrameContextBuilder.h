#pragma once

#include "Renderer/Public/Viewport/ViewportContracts.h"

#include <cstdint>

class PerViewDataBuilder;
class RenderCamera;
class PersistentRenderGpuScene;
class TemporalDataBuilder;
class RenderPreparationGraph;
class RenderRayTracingScene;
struct FrameContext;
class RenderWorld;
struct RenderFrameDynamicData;
struct RhiRect;
struct RhiViewport;

class FrameContextBuilder final
{
  public:
	static FrameContext Build(
	    const RenderWorld& world,
	    const RenderFrameDynamicData& dynamic,
	    PersistentRenderGpuScene& gpuScene,
	    std::uint32_t frameIndex,
	    const RenderCamera& renderCamera,
	    RenderViewportExtent sceneExtent,
	    RenderPreparationGraph& renderPreparationGraph,
	    RenderRayTracingScene* renderRayTracingScene,
	    PerViewDataBuilder& perViewDataBuilder,
	    TemporalDataBuilder& temporalDataBuilder);

  private:
	static RhiViewport BuildSceneViewport(
	    RenderViewportExtent sceneExtent) noexcept;
	static RhiRect BuildSceneScissorRect(
	    RenderViewportExtent sceneExtent) noexcept;
};

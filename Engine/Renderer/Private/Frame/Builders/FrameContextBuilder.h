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
class RenderScene;
struct RhiRect;
struct RhiViewport;

struct FrameContextBuildRequest final
{
	PersistentRenderGpuScene& GpuScene;
	std::uint64_t FrameId = 0;
	std::uint32_t FrameIndex = 0;
	RenderViewportExtent SceneExtent;
	RenderRayTracingScene* RayTracingScene = nullptr;
};

class FrameContextBuilder final
{
public:
	FrameContextBuilder(
	    RenderScene& scene,
	    const RenderCamera& renderCamera,
	    RenderPreparationGraph& renderPreparationGraph,
	    PerViewDataBuilder& perViewDataBuilder,
	    TemporalDataBuilder& temporalDataBuilder) noexcept;

	void Build(FrameContext& output, const FrameContextBuildRequest& request) const;

private:
	static RhiViewport BuildSceneViewport(RenderViewportExtent sceneExtent) noexcept;
	static RhiRect BuildSceneScissorRect(RenderViewportExtent sceneExtent) noexcept;

	RenderScene& m_scene;
	const RenderCamera& m_renderCamera;
	RenderPreparationGraph& m_renderPreparationGraph;
	PerViewDataBuilder& m_perViewDataBuilder;
	TemporalDataBuilder& m_temporalDataBuilder;
};

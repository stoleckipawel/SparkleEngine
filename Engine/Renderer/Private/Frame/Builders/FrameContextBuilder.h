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
struct RenderSceneDynamicData;
struct RhiRect;
struct RhiViewport;

struct FrameContextBuildRequest final
{
	const RenderSceneDynamicData& Dynamic;
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
	    const RenderWorld& world,
	    const RenderCamera& renderCamera,
	    RenderPreparationGraph& renderPreparationGraph,
	    PerViewDataBuilder& perViewDataBuilder,
	    TemporalDataBuilder& temporalDataBuilder) noexcept;

	void Build(FrameContext& output, const FrameContextBuildRequest& request) const;

private:
	static RhiViewport BuildSceneViewport(RenderViewportExtent sceneExtent) noexcept;
	static RhiRect BuildSceneScissorRect(RenderViewportExtent sceneExtent) noexcept;

	const RenderWorld& m_world;
	const RenderCamera& m_renderCamera;
	RenderPreparationGraph& m_renderPreparationGraph;
	PerViewDataBuilder& m_perViewDataBuilder;
	TemporalDataBuilder& m_temporalDataBuilder;
};

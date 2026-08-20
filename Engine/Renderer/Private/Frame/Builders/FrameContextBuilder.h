#pragma once

#include "Renderer/Public/Viewport/ViewportContracts.h"
#include "Renderer/Public/Debug/RenderViewMode.h"

#include <cstdint>

class PersistentRenderGpuScene;
class RenderScenePreparation;
class RenderRayTracingScene;
class RenderViewBuilder;
class RenderViewPreparation;
class RenderViewState;
struct FrameContext;
class RenderScene;
struct RenderViewInput;

struct FrameContextBuildRequest final
{
	PersistentRenderGpuScene& GpuScene;
	RenderViewState& ViewState;
	const RenderViewInput& ViewInput;
	const ViewportRenderRequest& ViewportRequest;
	std::uint64_t FrameId = 0;
	std::uint64_t ShaderGeneration = 0;
	std::uint64_t ImageProviderGeneration = 0;
	std::uint64_t GraphTopologyGeneration = 0;
	std::uint32_t FrameIndex = 0;
	RenderViewportExtent RenderExtent;
	RenderViewportExtent OutputExtent;
	RenderViewMode ViewMode = RenderViewMode::Lit;
	RenderRayTracingScene* RayTracingScene = nullptr;
};

class FrameContextBuilder final
{
public:
	FrameContextBuilder(
	    RenderScene& scene,
	    RenderScenePreparation& renderScenePreparation,
	    RenderViewBuilder& renderViewBuilder,
	    RenderViewPreparation& renderViewPreparation) noexcept;

	void Build(FrameContext& output, const FrameContextBuildRequest& request) const;

private:
	RenderScene& m_scene;
	RenderScenePreparation& m_renderScenePreparation;
	RenderViewBuilder& m_renderViewBuilder;
	RenderViewPreparation& m_renderViewPreparation;
};

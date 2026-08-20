#pragma once

#include "GameFramework/Public/Rendering/RenderViewInput.h"
#include "Renderer/Public/Debug/RenderViewMode.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"
#include "RHI/Public/Resources/RhiResourceDesc.h"

#include <cstdint>

class RenderViewState;
struct RenderView;

struct RenderViewBuildRequest final
{
	const RenderViewInput& Input;
	const ViewportRenderRequest& ViewportRequest;
	RenderViewportExtent RenderExtent = {};
	RenderViewportExtent OutputExtent = {};
	RenderViewMode ViewMode = RenderViewMode::Lit;
	std::uint64_t FrameId = 0u;
	std::uint64_t SceneGeneration = 0u;
	std::uint64_t ShaderGeneration = 0u;
	std::uint64_t ImageProviderGeneration = 0u;
	std::uint64_t GraphTopologyGeneration = 0u;
};

class RenderViewBuilder final
{
public:
	void Build(RenderView& output, RenderViewState& state, const RenderViewBuildRequest& request) const noexcept;

private:
	static RhiViewport BuildViewport(RenderViewportExtent extent) noexcept;
	static RhiRect BuildScissorRect(RenderViewportExtent extent) noexcept;
};

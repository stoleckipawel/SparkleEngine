#pragma once

#include "GameFramework/Public/Rendering/RenderViewInput.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

#include <cstdint>

class RenderViewState;
struct RenderView;

struct RenderViewBuildRequest final
{
	const RenderViewInput& Input;
	const ViewportRenderRequest& ViewportRequest;
	RenderViewportExtent RenderExtent = {};
	RenderViewportExtent OutputExtent = {};
	std::uint64_t FrameId = 0u;
	std::uint64_t SceneGeneration = 0u;
	std::uint64_t ShaderGeneration = 0u;
	std::uint64_t ImageProviderGeneration = 0u;
	std::uint64_t GraphTopologyGeneration = 0u;
};

void BuildRenderView(RenderView& output, RenderViewState& state, const RenderViewBuildRequest& request) noexcept;

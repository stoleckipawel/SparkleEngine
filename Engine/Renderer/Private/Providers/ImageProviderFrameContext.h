#pragma once

#include "Frame/Temporal/TemporalFrameState.h"
#include "ShaderData/RenderViewCameraData.h"
#include "Viewport/ViewportContracts.h"

#include <cstdint>

// Provider-neutral temporal data shared by image reconstruction features.
// Frame-graph resources stay in their declaring passes and are resolved only
// at evaluation time.
struct ImageProviderFrameContext final
{
	RenderViewportExtent RenderExtent = {};
	RenderViewportExtent OutputExtent = {};
	std::uint64_t FrameId = 0;
	std::uint64_t ProviderGeneration = 0;
	PerViewCameraConstantBufferData Camera = {};
	PerTemporalConstantBufferData TemporalData = {};
	RenderTemporalFrameState TemporalState = {};
	bool ResetHistory = true;
};

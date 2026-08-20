#pragma once

#include "ShaderData/ViewCameraUniformData.h"
#include "ShaderData/ViewTemporalUniformData.h"
#include "Viewport/ViewportContracts.h"

#include <cstdint>

// Provider-neutral temporal data shared by image reconstruction features.
// Frame-graph resources stay in their declaring passes and are resolved only
// at evaluation time.
struct ImageProviderFrameInput final
{
	RenderViewportExtent RenderExtent = {};
	RenderViewportExtent OutputExtent = {};
	std::uint64_t FrameId = 0;
	std::uint64_t ProviderGeneration = 0;
	ViewCameraUniformData Camera = {};
	ViewTemporalUniformData Temporal = {};
	bool ResetHistory = true;
};

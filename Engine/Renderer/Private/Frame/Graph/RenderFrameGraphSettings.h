#pragma once

#include "Renderer/Public/Settings/EngineRenderingDisplayTypes.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"
#include "RHI/Public/Formats/PixelFormat.h"

#include <cstdint>

enum class FramePresentationTarget : std::uint8_t
{
	ViewportProduct,
	BackBuffer,
};

struct RenderFrameGraphSettings final
{
	RenderViewportExtent RenderExtent;
	RenderViewportExtent OutputExtent;
	bool UseRayReconstruction = false;
	PixelFormat OutputFormat = PixelFormat::Unknown;
	EngineExposureMeteringMethod ExposureMeteringMethod = EngineExposureMeteringMethod::ParallelReduction;
	FramePresentationTarget PresentationTarget = FramePresentationTarget::ViewportProduct;
	RenderOutputFlags RequestedOutputs = RenderOutputFlags::None;

	bool operator==(const RenderFrameGraphSettings&) const noexcept = default;
};

bool ShouldUseRayReconstruction(RenderViewMode viewMode) noexcept;

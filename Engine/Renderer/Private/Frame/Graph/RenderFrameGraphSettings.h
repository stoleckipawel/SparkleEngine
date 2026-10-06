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
	PixelFormat OutputFormat = PixelFormat::Unknown;
	EngineExposureMeteringMethod ExposureMeteringMethod = EngineExposureMeteringMethod::Histogram;
	FramePresentationTarget PresentationTarget = FramePresentationTarget::ViewportProduct;
	RenderOutputFlags RequestedOutputs = RenderOutputFlags::None;

	bool operator==(const RenderFrameGraphSettings&) const noexcept = default;
};

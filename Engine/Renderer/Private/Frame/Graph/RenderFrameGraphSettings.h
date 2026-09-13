#pragma once

#include "Providers/ImageProviderPipeline.h"
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
	ImageProviderPipeline ImagePipeline = ImageProviderPipeline::RayReconstruction;
	PixelFormat OutputFormat = PixelFormat::Unknown;
	EngineExposureMeteringMethod ExposureMeteringMethod = EngineExposureMeteringMethod::ParallelReduction;
	FramePresentationTarget PresentationTarget = FramePresentationTarget::BackBuffer;
	RenderOutputFlags RequestedOutputs = RenderOutputFlags::None;

	bool operator==(const RenderFrameGraphSettings&) const noexcept = default;
};

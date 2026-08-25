#pragma once

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Renderer/Public/Settings/EngineRenderingDisplayTypes.h"
#include "RHI/Public/Formats/PixelFormat.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

#include <cstdint>

class FrameGraphBuilder;
class GpuMeshCache;
class IRayReconstructionProvider;
class IUpscalerProvider;
class RenderRayTracingScene;

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
	EngineExposureMeteringMethod ExposureMeteringMethod = EngineExposureMeteringMethod::ParallelReduction;
	FramePresentationTarget PresentationTarget = FramePresentationTarget::BackBuffer;
	RenderOutputFlags RequestedOutputs = RenderOutputFlags::None;

	bool operator==(const RenderFrameGraphSettings&) const noexcept = default;
};

RenderFrameGraphResources BuildRenderFrameGraph(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    GpuMeshCache& gpuMeshCache,
    RenderRayTracingScene& rayTracingScene,
    bool hasMaskedRayTracingGeometry,
    IUpscalerProvider* upscalerProvider,
    IRayReconstructionProvider* rayReconstructionProvider);

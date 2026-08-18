#pragma once

#include "Frame/Core/FrameAssembly.h"
#include "Renderer/Public/Settings/EngineRenderingDisplayTypes.h"
#include "RHI/Public/Formats/PixelFormat.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

#include <cstdint>

class FrameGraphBuilder;

struct FrameBuildResult
{
	FrameAssemblyResourceLayout Resources = {};
};

enum class FramePresentationTarget : std::uint8_t
{
	ViewportProduct,
	BackBuffer,
};

struct FrameBuildSettings final
{
	RenderViewportExtent RenderExtent;
	RenderViewportExtent OutputExtent;
	PixelFormat OutputFormat = PixelFormat::Unknown;
	EngineExposureMeteringMethod ExposureMeteringMethod = EngineExposureMeteringMethod::ParallelReduction;
	FramePresentationTarget PresentationTarget = FramePresentationTarget::BackBuffer;
};

FrameBuildResult BuildFrame(FrameGraphBuilder& builder, const FrameBuildSettings& settings);

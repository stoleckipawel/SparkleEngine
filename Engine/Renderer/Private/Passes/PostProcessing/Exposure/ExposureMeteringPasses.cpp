#include "../../../PCH.h"
#include "Passes/PostProcessing/Exposure/ExposureMeteringPasses.h"

#include "Core/Public/Math/MathUtils.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/FrameGraphBufferDesc.h"
#include "Passes/PostProcessing/Exposure/ExposureHistogramPassDefinitions.h"
#include "Passes/PostProcessing/Exposure/ExposureMomentPassDefinitions.h"

#include <algorithm>

ExposureMomentTexture AddExposureHistogramPasses(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, const RenderFrameGraphResources& resources)
{
	// Bin count matches the fixed layout in ExposureHistogram.hlsli.
	constexpr std::uint32_t histogramBinCount = 512u;
	const auto histogram = builder.CreateBuffer(FrameGraphBufferDesc::Create("ExposureHistogram", histogramBinCount * sizeof(std::uint32_t), sizeof(std::uint32_t)));
	const auto moments = CreateExposureMomentTexture(builder, "ExposureHistogramMoments", 0u, 1u, 1u);
	AddExposureHistogramClearPass(builder, histogram);
	AddExposureHistogramBuildPass(builder, resources.Transient.Scene.SceneColor, sceneExtent, histogram);
	AddExposureHistogramResolvePass(builder, histogram, moments);
	return moments;
}

ExposureMomentTexture AddExposureDownsamplePasses(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, const RenderFrameGraphResources& resources)
{
	std::uint32_t width = (std::max) (MathUtils::DivideRoundUp(sceneExtent.Width, 2u), 1u);
	std::uint32_t height = (std::max) (MathUtils::DivideRoundUp(sceneExtent.Height, 2u), 1u);
	ExposureMomentTexture current = CreateExposureMomentTexture(builder, "ExposureDownsampleMoments", 0u, width, height);
	AddExposureSceneDownsamplePass(builder, resources.Transient.Scene.SceneColor, current);

	std::uint32_t level = 1u;
	while (current.Width > 1u || current.Height > 1u)
	{
		width = (std::max) (MathUtils::DivideRoundUp(current.Width, 2u), 1u);
		height = (std::max) (MathUtils::DivideRoundUp(current.Height, 2u), 1u);
		ExposureMomentTexture next = CreateExposureMomentTexture(builder, "ExposureDownsampleMoments", level, width, height);
		AddExposureTextureDownsamplePass(builder, current, next);
		current = next;
		++level;
	}

	return current;
}

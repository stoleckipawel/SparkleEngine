#include "../../PCH.h"
#include "Passes/PostProcessing/ExposureMeteringPasses.h"

#include "Core/Public/Math/MathUtils.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Passes/PostProcessing/ExposureMomentPassDefinitions.h"

#include <algorithm>

ExposureMomentTexture AddExposureReductionPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const RenderFrameGraphResources& resources)
{
	std::uint32_t width = (std::max) (MathUtils::DivideRoundUp(sceneExtent.Width, 16u), 1u);
	std::uint32_t height = (std::max) (MathUtils::DivideRoundUp(sceneExtent.Height, 16u), 1u);
	ExposureMomentTexture current = CreateExposureMomentTexture(builder, "ExposureReductionMoments", 0u, width, height);
	AddExposureSceneReductionPass(builder, resources.Transient.Scene.SceneColor, current);

	std::uint32_t level = 1u;
	while (current.Width > 1u || current.Height > 1u)
	{
		width = (std::max) (MathUtils::DivideRoundUp(current.Width, 16u), 1u);
		height = (std::max) (MathUtils::DivideRoundUp(current.Height, 16u), 1u);
		ExposureMomentTexture next = CreateExposureMomentTexture(builder, "ExposureReductionMoments", level, width, height);
		AddExposureTextureReductionPass(builder, current, next);
		current = next;
		++level;
	}

	return current;
}

ExposureMomentTexture AddExposureDownsamplePasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const RenderFrameGraphResources& resources)
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

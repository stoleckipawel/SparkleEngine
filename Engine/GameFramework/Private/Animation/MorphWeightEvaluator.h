#pragma once

#include "GameFramework/Public/Scene/Animations/AnimationClipResource.h"

#include <span>

namespace MorphWeightEvaluator
{
	bool Evaluate(
	    const AnimationClipResource& clip,
	    std::uint32_t channelIndex,
	    float playbackTimeSeconds,
	    std::span<float> outputWeights) noexcept;
}

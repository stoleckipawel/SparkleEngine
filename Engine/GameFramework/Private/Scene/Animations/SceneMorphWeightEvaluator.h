#pragma once

#include "GameFramework/Public/Scene/Animations/SceneAnimation.h"

namespace SceneMorphWeightEvaluator
{
	void AppendSnapshots(
	    const SceneAnimationClipDesc& clip,
	    float playbackTimeSeconds,
	    std::vector<SceneMorphWeightSnapshot>& outMorphWeights);
}

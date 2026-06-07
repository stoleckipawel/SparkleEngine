#pragma once

#include "GameFramework/Public/Scene/Animations/SceneAnimation.h"
#include "GameFramework/Public/Scene/Skeletons/SceneSkeleton.h"

class SceneSkeletons;

namespace SceneAnimationPoseEvaluator
{
	SceneAnimationPoseSnapshot EvaluateClip(
	    const SceneAnimationClipDesc& clip,
	    const SceneSkeletonDesc& skeleton,
	    float playbackTimeSeconds);

	void AppendMatchingPose(
	    const SceneAnimationClipDesc& clip,
	    float playbackTimeSeconds,
	    const SceneSkeletons& skeletons,
	    std::vector<SceneAnimationPoseSnapshot>& outActivePoses);
}

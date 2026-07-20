#pragma once

#include "Animation/AnimationEvaluationTypes.h"
#include "Animation/AnimationClipResource.h"

#include <DirectXMath.h>

#include <span>

namespace AnimationPoseEvaluator
{
	bool Evaluate(
	    const AnimationClipResource& clip,
	    const ECS::SkeletonEvaluationData& skeleton,
	    float playbackTimeSeconds,
	    std::span<ECS::AnimationJointTransform> localTransforms,
	    std::span<DirectX::XMFLOAT4X4> modelSpaceTransforms) noexcept;
}

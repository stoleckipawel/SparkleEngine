#pragma once

#include "GameFramework/Public/Scene/Animations/SceneAnimation.h"

#include <DirectXMath.h>

namespace SceneAnimationSampler
{
	DirectX::XMVECTOR SampleVectorChannel(
	    const SceneAnimationClipDesc& clip,
	    const SceneAnimationChannel& channel,
	    float timeSeconds) noexcept;

	DirectX::XMVECTOR SampleRotationChannel(
	    const SceneAnimationClipDesc& clip,
	    const SceneAnimationChannel& channel,
	    float timeSeconds) noexcept;
}

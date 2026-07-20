#pragma once

#include "GameFramework/Public/Scene/Animations/AnimationClipResource.h"

#include <DirectXMath.h>

namespace AnimationSampler
{
	DirectX::XMVECTOR SampleVectorChannel(
	    const AnimationClipResource& clip,
	    const AnimationChannel& channel,
	    float timeSeconds) noexcept;

	DirectX::XMVECTOR SampleRotationChannel(
	    const AnimationClipResource& clip,
	    const AnimationChannel& channel,
	    float timeSeconds) noexcept;
}

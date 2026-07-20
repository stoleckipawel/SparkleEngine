#include "PCH.h"

#include "Animation/AnimationSampler.h"

#include <algorithm>
#include <limits>

namespace
{
	DirectX::XMVECTOR LoadValue(const DirectX::XMFLOAT4& value) noexcept { return DirectX::XMLoadFloat4(&value); }

	std::uint32_t FindKeyframeSegment(
	    const AnimationClipResource& clip,
	    const AnimationChannel& channel,
	    float timeSeconds) noexcept
	{
		if (channel.keyframeCount <= 1u)
			return 0;
		const std::uint32_t first = channel.firstKeyframe;
		const std::uint32_t lastSegment = channel.keyframeCount - 2u;
		for (std::uint32_t segment = 0; segment <= lastSegment; ++segment)
		{
			if (timeSeconds <= clip.keyframes[first + segment + 1u].timeSeconds)
				return segment;
		}
		return lastSegment;
	}

	float ComputeSegmentAlpha(const AnimationKeyframe& lhs, const AnimationKeyframe& rhs, float timeSeconds) noexcept
	{
		const float duration = rhs.timeSeconds - lhs.timeSeconds;
		return duration <= (std::numeric_limits<float>::epsilon)()
		           ? 0.0f
		           : std::clamp((timeSeconds - lhs.timeSeconds) / duration, 0.0f, 1.0f);
	}
}

namespace AnimationSampler
{
	DirectX::XMVECTOR SampleVectorChannel(
	    const AnimationClipResource& clip,
	    const AnimationChannel& channel,
	    float timeSeconds) noexcept
	{
		const std::uint32_t first = channel.firstKeyframe;
		if (channel.keyframeCount == 0u || first >= clip.keyframes.size())
			return DirectX::XMVectorZero();
		if (channel.keyframeCount == 1u || channel.interpolation == Assets::CookedAnimationInterpolation::Step)
			return LoadValue(clip.keyframes[first + FindKeyframeSegment(clip, channel, timeSeconds)].value);
		const std::uint32_t segment = FindKeyframeSegment(clip, channel, timeSeconds);
		const AnimationKeyframe& lhs = clip.keyframes[first + segment];
		const AnimationKeyframe& rhs = clip.keyframes[first + segment + 1u];
		return DirectX::XMVectorLerp(LoadValue(lhs.value), LoadValue(rhs.value), ComputeSegmentAlpha(lhs, rhs, timeSeconds));
	}

	DirectX::XMVECTOR SampleRotationChannel(
	    const AnimationClipResource& clip,
	    const AnimationChannel& channel,
	    float timeSeconds) noexcept
	{
		const std::uint32_t first = channel.firstKeyframe;
		if (channel.keyframeCount == 0u || first >= clip.keyframes.size())
			return DirectX::XMQuaternionIdentity();
		if (channel.keyframeCount == 1u || channel.interpolation == Assets::CookedAnimationInterpolation::Step)
			return DirectX::XMQuaternionNormalize(
			    LoadValue(clip.keyframes[first + FindKeyframeSegment(clip, channel, timeSeconds)].value));
		const std::uint32_t segment = FindKeyframeSegment(clip, channel, timeSeconds);
		const AnimationKeyframe& lhs = clip.keyframes[first + segment];
		const AnimationKeyframe& rhs = clip.keyframes[first + segment + 1u];
		return DirectX::XMQuaternionNormalize(DirectX::XMQuaternionSlerp(
		    LoadValue(lhs.value), LoadValue(rhs.value), ComputeSegmentAlpha(lhs, rhs, timeSeconds)));
	}
}

#include "PCH.h"

#include "Animation/AnimationSampler.h"

#include <algorithm>
#include <limits>

static inline std::uint32_t FindKeyframeSegment(
    const AnimationClipResource& clip,
    const AnimationChannel& channel,
    float timeSeconds) noexcept
{
	if (channel.keyframeCount <= 1u)
	{
		return 0;
	}

	const std::uint32_t first = channel.firstKeyframe;
	const std::uint32_t lastSegment = channel.keyframeCount - 2u;
	for (std::uint32_t segment = 0; segment <= lastSegment; ++segment)
	{
		if (timeSeconds <= clip.keyframes[first + segment + 1u].timeSeconds)
		{
			return segment;
		}
	}

	return lastSegment;
}

static inline float ComputeSegmentAlpha(const AnimationKeyframe& lhs, const AnimationKeyframe& rhs, float timeSeconds) noexcept
{
	const float duration = rhs.timeSeconds - lhs.timeSeconds;
	return duration <= (std::numeric_limits<float>::epsilon)() ? 0.0f : std::clamp((timeSeconds - lhs.timeSeconds) / duration, 0.0f, 1.0f);
}

static inline std::uint32_t FindStepKeyframe(const AnimationClipResource& clip, const AnimationChannel& channel, float timeSeconds) noexcept
{
	const std::uint32_t first = channel.firstKeyframe;
	std::uint32_t selected = 0u;
	for (std::uint32_t keyframe = 1u; keyframe < channel.keyframeCount; ++keyframe)
	{
		if (timeSeconds < clip.keyframes[first + keyframe].timeSeconds)
		{
			break;
		}
		selected = keyframe;
	}
	return selected;
}

static inline DirectX::XMVECTOR CubicSpline(const AnimationKeyframe& lhs, const AnimationKeyframe& rhs, float alpha) noexcept
{
	const float alphaSquared = alpha * alpha;
	const float alphaCubed = alphaSquared * alpha;
	const float duration = rhs.timeSeconds - lhs.timeSeconds;
	const float h00 = 2.0f * alphaCubed - 3.0f * alphaSquared + 1.0f;
	const float h10 = alphaCubed - 2.0f * alphaSquared + alpha;
	const float h01 = -2.0f * alphaCubed + 3.0f * alphaSquared;
	const float h11 = alphaCubed - alphaSquared;
	const DirectX::XMVECTOR valueTerms = DirectX::XMVectorAdd(
	    DirectX::XMVectorScale(DirectX::XMLoadFloat4(&lhs.value), h00),
	    DirectX::XMVectorScale(DirectX::XMLoadFloat4(&rhs.value), h01));
	const DirectX::XMVECTOR tangentTerms = DirectX::XMVectorAdd(
	    DirectX::XMVectorScale(DirectX::XMLoadFloat4(&lhs.outTangent), h10 * duration),
	    DirectX::XMVectorScale(DirectX::XMLoadFloat4(&rhs.inTangent), h11 * duration));
	return DirectX::XMVectorAdd(valueTerms, tangentTerms);
}

namespace AnimationSampler
{
	DirectX::XMVECTOR SampleVectorChannel(const AnimationClipResource& clip, const AnimationChannel& channel, float timeSeconds) noexcept
	{
		const std::uint32_t first = channel.firstKeyframe;
		if (channel.keyframeCount == 0u || first >= clip.keyframes.size())
		{
			return DirectX::XMVectorZero();
		}

		if (channel.keyframeCount == 1u)
		{
			return DirectX::XMLoadFloat4(&clip.keyframes[first].value);
		}
		if (channel.interpolation == Assets::CookedAnimationInterpolation::Step)
		{
			return DirectX::XMLoadFloat4(&clip.keyframes[first + FindStepKeyframe(clip, channel, timeSeconds)].value);
		}

		const std::uint32_t segment = FindKeyframeSegment(clip, channel, timeSeconds);
		const AnimationKeyframe& lhs = clip.keyframes[first + segment];
		const AnimationKeyframe& rhs = clip.keyframes[first + segment + 1u];
		const float alpha = ComputeSegmentAlpha(lhs, rhs, timeSeconds);
		return channel.interpolation == Assets::CookedAnimationInterpolation::CubicSpline
		    ? CubicSpline(lhs, rhs, alpha)
		    : DirectX::XMVectorLerp(DirectX::XMLoadFloat4(&lhs.value), DirectX::XMLoadFloat4(&rhs.value), alpha);
	}

	DirectX::XMVECTOR SampleRotationChannel(const AnimationClipResource& clip, const AnimationChannel& channel, float timeSeconds) noexcept
	{
		const std::uint32_t first = channel.firstKeyframe;
		if (channel.keyframeCount == 0u || first >= clip.keyframes.size())
		{
			return DirectX::XMQuaternionIdentity();
		}

		if (channel.keyframeCount == 1u)
		{
			return DirectX::XMQuaternionNormalize(DirectX::XMLoadFloat4(&clip.keyframes[first].value));
		}
		if (channel.interpolation == Assets::CookedAnimationInterpolation::Step)
		{
			return DirectX::XMQuaternionNormalize(
			    DirectX::XMLoadFloat4(&clip.keyframes[first + FindStepKeyframe(clip, channel, timeSeconds)].value));
		}

		const std::uint32_t segment = FindKeyframeSegment(clip, channel, timeSeconds);
		const AnimationKeyframe& lhs = clip.keyframes[first + segment];
		const AnimationKeyframe& rhs = clip.keyframes[first + segment + 1u];
		const float alpha = ComputeSegmentAlpha(lhs, rhs, timeSeconds);
		const DirectX::XMVECTOR sampled = channel.interpolation == Assets::CookedAnimationInterpolation::CubicSpline
		    ? CubicSpline(lhs, rhs, alpha)
		    : DirectX::XMQuaternionSlerp(DirectX::XMLoadFloat4(&lhs.value), DirectX::XMLoadFloat4(&rhs.value), alpha);
		return DirectX::XMQuaternionNormalize(sampled);
	}
}

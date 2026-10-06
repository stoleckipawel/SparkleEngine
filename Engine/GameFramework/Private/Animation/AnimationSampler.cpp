#include "PCH.h"

#include "Animation/AnimationSampler.h"

#include <algorithm>
#include <limits>
#include <span>

static std::span<const AnimationKeyframe> SelectKeyframes(
    const AnimationClipResource& clip,
    const AnimationChannel& channel,
    float timeSeconds) noexcept
{
	const std::uint32_t first = channel.firstKeyframe;
	if (channel.keyframeCount == 0u || first >= clip.keyframes.size())
	{
		return {};
	}
	const std::span<const AnimationKeyframe> keyframes{clip.keyframes};
	if (channel.keyframeCount == 1u)
	{
		return keyframes.subspan(first, 1u);
	}
	if (channel.interpolation == Assets::CookedAnimationInterpolation::Step)
	{
		std::uint32_t selected = 0u;
		for (std::uint32_t keyframe = 1u; keyframe < channel.keyframeCount; ++keyframe)
		{
			if (timeSeconds < keyframes[first + keyframe].timeSeconds)
			{
				break;
			}
			selected = keyframe;
		}
		return keyframes.subspan(first + selected, 1u);
	}

	const std::uint32_t lastSegment = channel.keyframeCount - 2u;
	for (std::uint32_t segment = 0u; segment <= lastSegment; ++segment)
	{
		if (timeSeconds <= keyframes[first + segment + 1u].timeSeconds)
		{
			return keyframes.subspan(first + segment, 2u);
		}
	}
	return keyframes.subspan(first + lastSegment, 2u);
}

static float ComputeSegmentAlpha(const AnimationKeyframe& lhs, const AnimationKeyframe& rhs, float timeSeconds) noexcept
{
	const float duration = rhs.timeSeconds - lhs.timeSeconds;
	return duration <= (std::numeric_limits<float>::epsilon)() ? 0.0f : std::clamp((timeSeconds - lhs.timeSeconds) / duration, 0.0f, 1.0f);
}

static DirectX::XMVECTOR CubicSpline(const AnimationKeyframe& lhs, const AnimationKeyframe& rhs, float alpha) noexcept
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
		const std::span<const AnimationKeyframe> keyframes = SelectKeyframes(clip, channel, timeSeconds);
		if (keyframes.empty())
		{
			return DirectX::XMVectorZero();
		}
		if (keyframes.size() == 1u)
		{
			return DirectX::XMLoadFloat4(&keyframes.front().value);
		}

		const AnimationKeyframe& lhs = keyframes[0];
		const AnimationKeyframe& rhs = keyframes[1];
		const float alpha = ComputeSegmentAlpha(lhs, rhs, timeSeconds);
		return channel.interpolation == Assets::CookedAnimationInterpolation::CubicSpline
		    ? CubicSpline(lhs, rhs, alpha)
		    : DirectX::XMVectorLerp(DirectX::XMLoadFloat4(&lhs.value), DirectX::XMLoadFloat4(&rhs.value), alpha);
	}

	DirectX::XMVECTOR SampleRotationChannel(const AnimationClipResource& clip, const AnimationChannel& channel, float timeSeconds) noexcept
	{
		const std::span<const AnimationKeyframe> keyframes = SelectKeyframes(clip, channel, timeSeconds);
		if (keyframes.empty())
		{
			return DirectX::XMQuaternionIdentity();
		}
		if (keyframes.size() == 1u)
		{
			return DirectX::XMQuaternionNormalize(DirectX::XMLoadFloat4(&keyframes.front().value));
		}

		const AnimationKeyframe& lhs = keyframes[0];
		const AnimationKeyframe& rhs = keyframes[1];
		const float alpha = ComputeSegmentAlpha(lhs, rhs, timeSeconds);
		const DirectX::XMVECTOR sampled = channel.interpolation == Assets::CookedAnimationInterpolation::CubicSpline
		    ? CubicSpline(lhs, rhs, alpha)
		    : DirectX::XMQuaternionSlerp(DirectX::XMLoadFloat4(&lhs.value), DirectX::XMLoadFloat4(&rhs.value), alpha);
		return DirectX::XMQuaternionNormalize(sampled);
	}
}

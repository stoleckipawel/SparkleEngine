#include "PCH.h"

#include "Scene/Animations/SceneAnimationSampler.h"

#include <algorithm>
#include <limits>

namespace
{
	DirectX::XMVECTOR LoadValue(const DirectX::XMFLOAT4& value) noexcept
	{
		return DirectX::XMLoadFloat4(&value);
	}

	std::uint32_t FindKeyframeSegment(const SceneAnimationClipDesc& clip, const SceneAnimationChannel& channel, float timeSeconds) noexcept
	{
		if (channel.keyframeCount <= 1u)
		{
			return 0;
		}

		const std::uint32_t first = channel.firstKeyframe;
		const std::uint32_t lastSegment = channel.keyframeCount - 2u;
		for (std::uint32_t segment = 0; segment <= lastSegment; ++segment)
		{
			const SceneAnimationKeyframe& nextKey = clip.keyframes[first + segment + 1u];
			if (timeSeconds <= nextKey.timeSeconds)
			{
				return segment;
			}
		}

		return lastSegment;
	}

	float ComputeSegmentAlpha(const SceneAnimationKeyframe& lhs, const SceneAnimationKeyframe& rhs, float timeSeconds) noexcept
	{
		const float duration = rhs.timeSeconds - lhs.timeSeconds;
		if (duration <= (std::numeric_limits<float>::epsilon)())
		{
			return 0.0f;
		}

		return std::clamp((timeSeconds - lhs.timeSeconds) / duration, 0.0f, 1.0f);
	}
}

namespace SceneAnimationSampler
{
	DirectX::XMVECTOR SampleVectorChannel(const SceneAnimationClipDesc& clip, const SceneAnimationChannel& channel, float timeSeconds) noexcept
	{
		const std::uint32_t first = channel.firstKeyframe;
		if (channel.keyframeCount == 0u || first >= clip.keyframes.size())
		{
			return DirectX::XMVectorZero();
		}

		if (channel.keyframeCount == 1u || channel.interpolation == Assets::CookedAnimationInterpolation::Step)
		{
			return LoadValue(clip.keyframes[first + FindKeyframeSegment(clip, channel, timeSeconds)].value);
		}

		const std::uint32_t segment = FindKeyframeSegment(clip, channel, timeSeconds);
		const SceneAnimationKeyframe& lhs = clip.keyframes[first + segment];
		const SceneAnimationKeyframe& rhs = clip.keyframes[first + segment + 1u];
		const float alpha = ComputeSegmentAlpha(lhs, rhs, timeSeconds);
		return DirectX::XMVectorLerp(LoadValue(lhs.value), LoadValue(rhs.value), alpha);
	}

	DirectX::XMVECTOR SampleRotationChannel(const SceneAnimationClipDesc& clip, const SceneAnimationChannel& channel, float timeSeconds) noexcept
	{
		const std::uint32_t first = channel.firstKeyframe;
		if (channel.keyframeCount == 0u || first >= clip.keyframes.size())
		{
			return DirectX::XMQuaternionIdentity();
		}

		if (channel.keyframeCount == 1u || channel.interpolation == Assets::CookedAnimationInterpolation::Step)
		{
			return DirectX::XMQuaternionNormalize(LoadValue(clip.keyframes[first + FindKeyframeSegment(clip, channel, timeSeconds)].value));
		}

		const std::uint32_t segment = FindKeyframeSegment(clip, channel, timeSeconds);
		const SceneAnimationKeyframe& lhs = clip.keyframes[first + segment];
		const SceneAnimationKeyframe& rhs = clip.keyframes[first + segment + 1u];
		const float alpha = ComputeSegmentAlpha(lhs, rhs, timeSeconds);
		return DirectX::XMQuaternionNormalize(DirectX::XMQuaternionSlerp(LoadValue(lhs.value), LoadValue(rhs.value), alpha));
	}
}

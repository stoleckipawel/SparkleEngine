#include "PCH.h"

#include "Animation/AnimationPoseEvaluator.h"

#include "Animation/AnimationSampler.h"

#include <algorithm>

namespace
{
	DirectX::XMMATRIX Compose(const ECS::AnimationJointTransform& transform) noexcept
	{
		return DirectX::XMMatrixScalingFromVector(DirectX::XMLoadFloat3(&transform.Scale)) *
		       DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&transform.Rotation)) *
		       DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat3(&transform.Translation));
	}
}

namespace AnimationPoseEvaluator
{
	bool Evaluate(
	    const AnimationClipResource& clip,
	    const ECS::SkeletonEvaluationData& skeleton,
	    float playbackTimeSeconds,
	    std::span<ECS::AnimationJointTransform> localTransforms,
	    std::span<DirectX::XMFLOAT4X4> modelSpaceTransforms) noexcept
	{
		if (!skeleton.IsValid() || localTransforms.size() != skeleton.Resource->joints.size() ||
		    modelSpaceTransforms.size() != skeleton.Resource->joints.size())
			return false;
		std::copy(skeleton.BindLocalTransforms.begin(), skeleton.BindLocalTransforms.end(), localTransforms.begin());
		for (const AnimationChannel& channel : clip.channels)
		{
			if (channel.targetJointIndex >= localTransforms.size())
				continue;
			ECS::AnimationJointTransform& transform = localTransforms[channel.targetJointIndex];
			switch (channel.targetPath)
			{
				case Assets::CookedAnimationTargetPath::Translation:
					DirectX::XMStoreFloat3(&transform.Translation, AnimationSampler::SampleVectorChannel(clip, channel, playbackTimeSeconds));
					break;
				case Assets::CookedAnimationTargetPath::Rotation:
					DirectX::XMStoreFloat4(&transform.Rotation, AnimationSampler::SampleRotationChannel(clip, channel, playbackTimeSeconds));
					break;
				case Assets::CookedAnimationTargetPath::Scale:
					DirectX::XMStoreFloat3(&transform.Scale, AnimationSampler::SampleVectorChannel(clip, channel, playbackTimeSeconds));
					break;
				case Assets::CookedAnimationTargetPath::Weights:
				case Assets::CookedAnimationTargetPath::Unknown:
				default:
					break;
			}
		}
		for (std::size_t jointIndex = 0; jointIndex < localTransforms.size(); ++jointIndex)
		{
			DirectX::XMMATRIX model = Compose(localTransforms[jointIndex]);
			const std::uint32_t parent = skeleton.Resource->joints[jointIndex].parentJointIndex;
			if (parent < modelSpaceTransforms.size())
				model *= DirectX::XMLoadFloat4x4(&modelSpaceTransforms[parent]);
			DirectX::XMStoreFloat4x4(&modelSpaceTransforms[jointIndex], model);
		}
		return true;
	}
}

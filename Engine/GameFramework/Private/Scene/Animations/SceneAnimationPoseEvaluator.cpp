#include "PCH.h"

#include "Scene/Animations/SceneAnimationPoseEvaluator.h"

#include "Scene/Animations/SceneAnimationSampler.h"
#include "World/Resources/SkeletonResourceStore.h"

#include <vector>

namespace
{
	struct JointLocalTransform final
	{
		DirectX::XMVECTOR translation = DirectX::XMVectorZero();
		DirectX::XMVECTOR rotation = DirectX::XMQuaternionIdentity();
		DirectX::XMVECTOR scale = DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
	};

	DirectX::XMMATRIX LoadMatrix(const DirectX::XMFLOAT4X4& matrix) noexcept
	{
		return DirectX::XMLoadFloat4x4(&matrix);
	}

	JointLocalTransform DecomposeTransform(DirectX::FXMMATRIX matrix) noexcept
	{
		JointLocalTransform transform;
		if (!DirectX::XMMatrixDecompose(&transform.scale, &transform.rotation, &transform.translation, matrix))
		{
			transform = {};
		}
		transform.rotation = DirectX::XMQuaternionNormalize(transform.rotation);
		return transform;
	}

	DirectX::XMMATRIX ComposeTransform(const JointLocalTransform& transform) noexcept
	{
		return DirectX::XMMatrixScalingFromVector(transform.scale) *
		       DirectX::XMMatrixRotationQuaternion(transform.rotation) *
		       DirectX::XMMatrixTranslationFromVector(transform.translation);
	}

	std::vector<JointLocalTransform> BuildBindLocalTransforms(const SceneSkeletonDesc& skeleton)
	{
		std::vector<JointLocalTransform> localTransforms;
		localTransforms.reserve(skeleton.joints.size());
		for (std::size_t jointIndex = 0; jointIndex < skeleton.joints.size(); ++jointIndex)
		{
			const SceneJointDesc& joint = skeleton.joints[jointIndex];
			DirectX::XMMATRIX localMatrix = LoadMatrix(joint.bindPoseWorldTransform);
			if (joint.parentJointIndex < skeleton.joints.size())
			{
				const DirectX::XMMATRIX parentWorld = LoadMatrix(skeleton.joints[joint.parentJointIndex].bindPoseWorldTransform);
				localMatrix = localMatrix * DirectX::XMMatrixInverse(nullptr, parentWorld);
			}
			localTransforms.push_back(DecomposeTransform(localMatrix));
		}
		return localTransforms;
	}

	void ApplyChannel(
	    const SceneAnimationClipDesc& clip,
	    const SceneAnimationChannel& channel,
	    float timeSeconds,
	    std::vector<JointLocalTransform>& localTransforms)
	{
		if (channel.targetJointIndex >= localTransforms.size())
		{
			return;
		}

		JointLocalTransform& transform = localTransforms[channel.targetJointIndex];
		switch (channel.targetPath)
		{
			case Assets::CookedAnimationTargetPath::Translation:
				transform.translation = SceneAnimationSampler::SampleVectorChannel(clip, channel, timeSeconds);
				break;
			case Assets::CookedAnimationTargetPath::Rotation:
				transform.rotation = SceneAnimationSampler::SampleRotationChannel(clip, channel, timeSeconds);
				break;
			case Assets::CookedAnimationTargetPath::Scale:
				transform.scale = SceneAnimationSampler::SampleVectorChannel(clip, channel, timeSeconds);
				break;
			case Assets::CookedAnimationTargetPath::Weights:
			case Assets::CookedAnimationTargetPath::Unknown:
			default:
				break;
		}
	}

	void ComposeModelSpaceTransforms(
	    const SceneSkeletonDesc& skeleton,
	    const std::vector<JointLocalTransform>& localTransforms,
	    std::vector<DirectX::XMMATRIX>& outModelSpaceTransforms)
	{
		outModelSpaceTransforms.resize(localTransforms.size(), DirectX::XMMatrixIdentity());
		for (std::size_t jointIndex = 0; jointIndex < localTransforms.size(); ++jointIndex)
		{
			DirectX::XMMATRIX modelMatrix = ComposeTransform(localTransforms[jointIndex]);
			const std::uint32_t parentJointIndex = skeleton.joints[jointIndex].parentJointIndex;
			if (parentJointIndex < outModelSpaceTransforms.size())
			{
				modelMatrix = modelMatrix * outModelSpaceTransforms[parentJointIndex];
			}
			outModelSpaceTransforms[jointIndex] = modelMatrix;
		}
	}
}

namespace SceneAnimationPoseEvaluator
{
	SceneAnimationPoseSnapshot EvaluateClip(
	    const SceneAnimationClipDesc& clip,
	    const SceneSkeletonDesc& skeleton,
	    float playbackTimeSeconds)
	{
		std::vector<JointLocalTransform> localTransforms = BuildBindLocalTransforms(skeleton);
		for (const SceneAnimationChannel& channel : clip.channels)
		{
			ApplyChannel(clip, channel, playbackTimeSeconds, localTransforms);
		}

		std::vector<DirectX::XMMATRIX> modelSpaceTransforms;
		ComposeModelSpaceTransforms(skeleton, localTransforms, modelSpaceTransforms);

		SceneAnimationPoseSnapshot pose;
		pose.skeletonAssetId = skeleton.assetId;
		pose.animationAssetId = clip.animationAssetId;
		pose.clipName = clip.name;
		pose.playbackTimeSeconds = playbackTimeSeconds;
		pose.jointCount = static_cast<std::uint32_t>(skeleton.joints.size());
		pose.skinningMatrices.reserve(skeleton.joints.size());
		for (std::size_t jointIndex = 0; jointIndex < skeleton.joints.size(); ++jointIndex)
		{
			const DirectX::XMMATRIX skinningMatrix = LoadMatrix(skeleton.joints[jointIndex].inverseBindMatrix) * modelSpaceTransforms[jointIndex];
			DirectX::XMFLOAT4X4 storedMatrix{};
			DirectX::XMStoreFloat4x4(&storedMatrix, skinningMatrix);
			pose.skinningMatrices.push_back(storedMatrix);
		}

		return pose;
	}

	void AppendMatchingPose(
	    const SceneAnimationClipDesc& clip,
	    float playbackTimeSeconds,
	    const SkeletonResourceStore& skeletons,
	    std::vector<SceneAnimationPoseSnapshot>& outActivePoses)
	{
		for (const SceneSkeletonDesc& skeleton : skeletons.GetAll())
		{
			if (skeleton.assetId != clip.targetSkeletonAssetId)
			{
				continue;
			}

			outActivePoses.push_back(EvaluateClip(clip, skeleton, playbackTimeSeconds));
			break;
		}
	}
}

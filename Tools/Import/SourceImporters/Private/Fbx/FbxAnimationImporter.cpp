#include "PCH.h"

#include "Fbx/FbxAnimationImporter.h"

#include "Fbx/FbxNodeTransformConverter.h"
#include "Core/Public/Diagnostics/Error.h"

#include <DirectXMath.h>

#include <algorithm>
#include <format>
#include <limits>
#include <utility>

class FbxAnimationTranslation final
{
  public:
	static std::pair<std::uint32_t, std::uint32_t> FindSkeletonJointForNode(
	    const SourceImportOutput& output,
	    std::uint32_t sourceNodeIndex) noexcept
	{
		for (std::size_t skeletonIndex = 0; skeletonIndex < output.scene.skeletons.size(); ++skeletonIndex)
		{
			const ImportedSkeleton& skeleton = output.scene.skeletons[skeletonIndex];
			for (std::size_t jointIndex = 0; jointIndex < skeleton.joints.size(); ++jointIndex)
			{
				if (skeleton.joints[jointIndex].sourceNodeIndex == sourceNodeIndex)
				{
					return {static_cast<std::uint32_t>(skeletonIndex), static_cast<std::uint32_t>(jointIndex)};
				}
			}
		}
		return {(std::numeric_limits<std::uint32_t>::max)(), (std::numeric_limits<std::uint32_t>::max)()};
	}

	static void AppendVectorSampler(
	    const aiVectorKey* keys,
	    unsigned int keyCount,
	    double ticksPerSecond,
	    ImportedAnimationTargetPath targetPath,
	    std::uint32_t targetNodeIndex,
	    std::uint32_t targetJointIndex,
	    ImportedAnimationClip& clip)
	{
		if (keyCount == 0)
		{
			return;
		}
		if (keys == nullptr)
		{
			throw Diagnostics::Error("FBX animation vector sampler has no key data.");
		}

		ImportedAnimationSampler sampler;
		sampler.keyframes.reserve(keyCount);
		float previousTime = -1.0f;
		for (unsigned int keyIndex = 0; keyIndex < keyCount; ++keyIndex)
		{
			const float timeSeconds = static_cast<float>(keys[keyIndex].mTime / ticksPerSecond);
			const aiVector3D& value = keys[keyIndex].mValue;
			if (timeSeconds < 0.0f || timeSeconds <= previousTime)
			{
				throw Diagnostics::Error(std::format("FBX animation vector key {} is not strictly time ordered.", keyIndex));
			}

			sampler.keyframes.push_back(ImportedAnimationKeyframe{.timeSeconds = timeSeconds, .value = {value.x, value.y, value.z, 0.0f}});
			clip.durationSeconds = (std::max) (clip.durationSeconds, timeSeconds);
			previousTime = timeSeconds;
		}

		const std::uint32_t samplerIndex = static_cast<std::uint32_t>(clip.samplers.size());
		clip.samplers.push_back(std::move(sampler));
		clip.channels.push_back(
		    ImportedAnimationChannel{
		        .targetPath = targetPath,
		        .targetNodeIndex = targetNodeIndex,
		        .targetJointIndex = targetJointIndex,
		        .samplerIndex = samplerIndex});
	}

	static void AppendRotationSampler(
	    const aiQuatKey* keys,
	    unsigned int keyCount,
	    double ticksPerSecond,
	    std::uint32_t targetNodeIndex,
	    std::uint32_t targetJointIndex,
	    ImportedAnimationClip& clip)
	{
		if (keyCount == 0)
		{
			return;
		}
		if (keys == nullptr)
		{
			throw Diagnostics::Error("FBX animation rotation sampler has no key data.");
		}

		ImportedAnimationSampler sampler;
		sampler.keyframes.reserve(keyCount);
		float previousTime = -1.0f;
		for (unsigned int keyIndex = 0; keyIndex < keyCount; ++keyIndex)
		{
			const float timeSeconds = static_cast<float>(keys[keyIndex].mTime / ticksPerSecond);
			const aiQuaternion& value = keys[keyIndex].mValue;
			const DirectX::XMVECTOR quaternion = DirectX::XMVectorSet(value.x, value.y, value.z, value.w);
			const float lengthSquared = DirectX::XMVectorGetX(DirectX::XMVector4LengthSq(quaternion));
			if (timeSeconds < 0.0f || timeSeconds <= previousTime || lengthSquared <= 1.0e-8f)
			{
				throw Diagnostics::Error(std::format("FBX animation rotation key {} is invalid or not strictly time ordered.", keyIndex));
			}

			ImportedAnimationKeyframe keyframe;
			keyframe.timeSeconds = timeSeconds;
			DirectX::XMStoreFloat4(&keyframe.value, DirectX::XMQuaternionNormalize(quaternion));
			sampler.keyframes.push_back(keyframe);
			clip.durationSeconds = (std::max) (clip.durationSeconds, timeSeconds);
			previousTime = timeSeconds;
		}

		const std::uint32_t samplerIndex = static_cast<std::uint32_t>(clip.samplers.size());
		clip.samplers.push_back(std::move(sampler));
		clip.channels.push_back(
		    ImportedAnimationChannel{
		        .targetPath = ImportedAnimationTargetPath::Rotation,
		        .targetNodeIndex = targetNodeIndex,
		        .targetJointIndex = targetJointIndex,
		        .samplerIndex = samplerIndex});
	}

	static void AppendNodeChannel(
	    const aiScene& scene,
	    const aiNodeAnim& sourceChannel,
	    double ticksPerSecond,
	    ImportedAnimationClip& clip,
	    const SourceImportOutput& output)
	{
		if ((sourceChannel.mNumPositionKeys == 0 && sourceChannel.mNumRotationKeys == 0 && sourceChannel.mNumScalingKeys == 0) ||
		    (sourceChannel.mPreState != aiAnimBehaviour_DEFAULT && sourceChannel.mPreState != aiAnimBehaviour_CONSTANT) ||
		    (sourceChannel.mPostState != aiAnimBehaviour_DEFAULT && sourceChannel.mPostState != aiAnimBehaviour_CONSTANT))
		{
			throw Diagnostics::Error("FBX animation channel has no keys or uses unsupported boundary behavior.");
		}

		const aiNode* targetNode = FbxNodeTransformConverter::FindNode(scene, sourceChannel.mNodeName);
		if (targetNode == nullptr)
		{
			throw Diagnostics::Error("FBX animation channel targets an unknown node.");
		}

		const std::uint32_t targetNodeIndex = FbxNodeTransformConverter::FindNodeIndex(scene, *targetNode);
		if (targetNodeIndex == (std::numeric_limits<std::uint32_t>::max)())
		{
			throw Diagnostics::Error("FBX animation channel target has no source-node index.");
		}

		const auto [targetSkeletonIndex, targetJointIndex] = FindSkeletonJointForNode(output, targetNodeIndex);
		if (targetSkeletonIndex == (std::numeric_limits<std::uint32_t>::max)() ||
		    targetJointIndex == (std::numeric_limits<std::uint32_t>::max)())
		{
			throw Diagnostics::Error("FBX animation channel target is not owned by an imported skeleton.");
		}

		if (clip.targetSkeletonIndex != (std::numeric_limits<std::uint32_t>::max)() && clip.targetSkeletonIndex != targetSkeletonIndex)
		{
			throw Diagnostics::Error("FBX animation channels span multiple skeletons.");
		}
		clip.targetSkeletonIndex = targetSkeletonIndex;

		AppendVectorSampler(
		    sourceChannel.mPositionKeys,
		    sourceChannel.mNumPositionKeys,
		    ticksPerSecond,
		    ImportedAnimationTargetPath::Translation,
		    targetNodeIndex,
		    targetJointIndex,
		    clip);
		AppendRotationSampler(
		    sourceChannel.mRotationKeys,
		    sourceChannel.mNumRotationKeys,
		    ticksPerSecond,
		    targetNodeIndex,
		    targetJointIndex,
		    clip);
		AppendVectorSampler(
		    sourceChannel.mScalingKeys,
		    sourceChannel.mNumScalingKeys,
		    ticksPerSecond,
		    ImportedAnimationTargetPath::Scale,
		    targetNodeIndex,
		    targetJointIndex,
		    clip);
	}
};

void FbxAnimationImporter::ImportAnimations(const aiScene& scene, SourceImportOutput& output)
{
	output.scene.animations.reserve(scene.mNumAnimations);
	for (unsigned int animationIndex = 0; animationIndex < scene.mNumAnimations; ++animationIndex)
	{
		const aiAnimation* sourceAnimation = scene.mAnimations[animationIndex];
		if (sourceAnimation == nullptr || sourceAnimation->mDuration < 0.0 || sourceAnimation->mTicksPerSecond <= 0.0 ||
		    sourceAnimation->mNumChannels == 0 ||
		    sourceAnimation->mChannels == nullptr || sourceAnimation->mNumMeshChannels != 0 || sourceAnimation->mNumMorphMeshChannels != 0)
		{
			throw Diagnostics::Error(std::format("FBX animation {} has incomplete or unsupported channel data.", animationIndex));
		}

		ImportedAnimationClip clip;
		clip.name = sourceAnimation->mName.C_Str();
		clip.sourceAnimationIndex = animationIndex;
		clip.durationSeconds = static_cast<float>(sourceAnimation->mDuration / sourceAnimation->mTicksPerSecond);
		clip.samplers.reserve(sourceAnimation->mNumChannels * 3u);
		clip.channels.reserve(sourceAnimation->mNumChannels * 3u);

		for (unsigned int channelIndex = 0; channelIndex < sourceAnimation->mNumChannels; ++channelIndex)
		{
			if (sourceAnimation->mChannels[channelIndex] == nullptr)
			{
				throw Diagnostics::Error(std::format("FBX animation {} has a null channel {}.", animationIndex, channelIndex));
			}
			FbxAnimationTranslation::AppendNodeChannel(
			    scene,
			    *sourceAnimation->mChannels[channelIndex],
			    sourceAnimation->mTicksPerSecond,
			    clip,
			    output);
		}

		if (!clip.IsValid())
		{
			throw Diagnostics::Error(std::format("FBX animation {} cannot be represented without losing channel data.", animationIndex));
		}

		output.scene.animations.push_back(std::move(clip));
	}
}

#include "PCH.h"

#include "Assets/Translators/SceneAssetAnimationTranslator.h"

namespace Assets
{
	AnimationClipResource BuildSceneAssetAnimation(const LoadedAnimationAsset& animationAsset, CookedAssetId animationAssetId)
	{
		AnimationClipResource clip{
		    .animationAssetId = animationAssetId,
		    .targetSkeletonAssetId = animationAsset.header.targetSkeletonAssetId,
		    .name = animationAsset.header.name,
		    .sourceAnimationIndex = animationAsset.header.sourceAnimationIndex,
		    .durationSeconds = animationAsset.header.durationSeconds,
		    .channelCount = animationAsset.header.channelCount,
		    .keyframeCount = animationAsset.header.keyframeCount};

		clip.channels.reserve(animationAsset.channels.size());

		for (const CookedAnimationChannelRecord& channel : animationAsset.channels)
		{
			clip.channels.push_back(
			    AnimationChannel{
			        .targetPath = channel.targetPath,
			        .interpolation = channel.interpolation,
			        .targetNodeIndex = channel.targetNodeIndex,
			        .targetJointIndex = channel.targetJointIndex,
			        .firstKeyframe = channel.firstKeyframe,
			        .keyframeCount = channel.keyframeCount});
		}

		clip.keyframes.reserve(animationAsset.keyframes.size());

		for (const CookedAnimationKeyframeRecord& keyframe : animationAsset.keyframes)
		{
			clip.keyframes.push_back(
			    AnimationKeyframe{
			        .timeSeconds = keyframe.timeSeconds,
			        .value = keyframe.value,
			        .inTangent = keyframe.inTangent,
			        .outTangent = keyframe.outTangent});
		}

		return clip;
	}
}

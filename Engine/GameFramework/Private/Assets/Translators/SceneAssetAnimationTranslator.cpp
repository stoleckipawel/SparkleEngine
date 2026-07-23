#include "PCH.h"

#include "Assets/Translators/SceneAssetAnimationTranslator.h"

#include <string_view>

namespace Assets
{

		std::string AnimationNameToString(const CookedAnimationAssetHeader& header)
		{
			std::size_t length = 0;
			while (length < sizeof(header.name) && header.name[length] != '\0')
			{
				++length;
			}
			const std::string_view nameView(header.name, length);
			return std::string(nameView);
		}


	AnimationClipResource BuildSceneAssetAnimation(
	    const LoadedAnimationAsset& animationAsset,
	    CookedAssetId animationAssetId)
	{
		AnimationClipResource clip{
		    .animationAssetId = animationAssetId,
		    .targetSkeletonAssetId = animationAsset.header.targetSkeletonAssetId,
		    .name = AnimationNameToString(animationAsset.header),
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

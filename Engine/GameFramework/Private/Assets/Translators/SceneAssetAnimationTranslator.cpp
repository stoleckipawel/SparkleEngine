#include "PCH.h"

#include "Assets/Translators/SceneAssetAnimationTranslator.h"

#include <string_view>

namespace Assets
{
	namespace
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
	}

	SceneAnimationClipDesc BuildSceneAssetAnimation(
	    const LoadedAnimationAsset& animationAsset,
	    CookedAssetId animationAssetId)
	{
		return SceneAnimationClipDesc{
		    .animationAssetId = animationAssetId,
		    .targetSkeletonAssetId = animationAsset.header.targetSkeletonAssetId,
		    .name = AnimationNameToString(animationAsset.header),
		    .sourceAnimationIndex = animationAsset.header.sourceAnimationIndex,
		    .durationSeconds = animationAsset.header.durationSeconds,
		    .channelCount = animationAsset.header.channelCount,
		    .keyframeCount = animationAsset.header.keyframeCount};
	}
}

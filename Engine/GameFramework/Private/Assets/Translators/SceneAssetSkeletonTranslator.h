#pragma once

#include "Assets/Cooked/LoadedSkeletonAsset.h"
#include "GameFramework/Public/Scene/Skeletons/SceneSkeleton.h"

#include <cstdint>

namespace Assets
{
	SceneSkeletonDesc BuildSceneAssetSkeleton(
	    const LoadedSkeletonAsset& skeletonAsset,
	    CookedAssetId skeletonAssetId,
	    std::uint32_t sourceSkinIndex);
}

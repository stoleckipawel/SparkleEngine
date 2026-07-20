#pragma once

#include "Assets/Cooked/LoadedSkeletonAsset.h"
#include "GameFramework/Public/Scene/Skeletons/SkeletonResource.h"

#include <cstdint>

namespace Assets
{
	SkeletonResource BuildSceneAssetSkeleton(
	    const LoadedSkeletonAsset& skeletonAsset,
	    CookedAssetId skeletonAssetId,
	    std::uint32_t sourceSkinIndex);
}

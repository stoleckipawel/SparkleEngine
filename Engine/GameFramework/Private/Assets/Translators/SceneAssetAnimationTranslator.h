#pragma once

#include "Assets/Cooked/LoadedAnimationAsset.h"
#include "GameFramework/Public/Scene/Animations/AnimationClipResource.h"

namespace Assets
{
	AnimationClipResource BuildSceneAssetAnimation(
	    const LoadedAnimationAsset& animationAsset,
	    CookedAssetId animationAssetId);
}

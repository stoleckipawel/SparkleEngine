#pragma once

#include "Assets/Cooked/LoadedAnimationAsset.h"
#include "Animation/AnimationClipResource.h"

namespace Assets
{
	AnimationClipResource BuildSceneAssetAnimation(const LoadedAnimationAsset& animationAsset, CookedAssetId animationAssetId);
}

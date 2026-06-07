#pragma once

#include "Assets/Cooked/LoadedAnimationAsset.h"
#include "GameFramework/Public/Scene/Animations/SceneAnimation.h"

namespace Assets
{
	SceneAnimationClipDesc BuildSceneAssetAnimation(
	    const LoadedAnimationAsset& animationAsset,
	    CookedAssetId animationAssetId);
}

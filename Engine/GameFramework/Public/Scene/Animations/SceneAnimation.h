#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAnimationAsset.h"
#include "GameFramework/Public/GameFrameworkAPI.h"

#include <cstdint>
#include <string>

struct SPARKLE_ENGINE_API SceneAnimationClipDesc
{
	Assets::CookedAssetId animationAssetId = Assets::InvalidCookedAssetId;
	Assets::CookedAssetId targetSkeletonAssetId = Assets::InvalidCookedAssetId;
	std::string name;
	std::uint32_t sourceAnimationIndex = 0;
	float durationSeconds = 0.0f;
	std::uint32_t channelCount = 0;
	std::uint32_t keyframeCount = 0;
};

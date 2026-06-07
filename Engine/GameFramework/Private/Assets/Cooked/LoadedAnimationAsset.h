#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAnimationAsset.h"

#include <vector>

namespace Assets
{
	struct LoadedAnimationAsset final
	{
		CookedAnimationAssetHeader header;
		std::vector<CookedAnimationChannelRecord> channels;
		std::vector<CookedAnimationKeyframeRecord> keyframes;
	};
}

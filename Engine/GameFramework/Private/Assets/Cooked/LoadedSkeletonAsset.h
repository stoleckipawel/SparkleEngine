#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedSkeletonAsset.h"

#include <vector>

namespace Assets
{
	struct LoadedSkeletonAsset final
	{
		CookedSkeletonAssetHeader header;
		std::vector<CookedSkeletonJointRecord> joints;
	};
}

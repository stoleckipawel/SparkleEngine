#pragma once

#include "CookedSceneBuild.h"

#include <string>
#include <vector>

class CookedSkeletonAssetWriter final
{
  public:
	static bool WriteSkeletonAssets(const std::vector<CookedSkeletonAssetBuild>& skeletonAssets, std::string& outErrorMessage);
};

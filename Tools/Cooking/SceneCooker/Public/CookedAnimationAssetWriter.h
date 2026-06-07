#pragma once

#include "CookedSceneBuild.h"

#include <string>
#include <vector>

class CookedAnimationAssetWriter final
{
  public:
	static bool WriteAnimationAssets(const std::vector<CookedAnimationAssetBuild>& animationAssets, std::string& outErrorMessage);
};

#pragma once

#include "CookedSceneBuild.h"
#include "Core/Public/Files/FileUtils.h"

#include <string>
#include <vector>

class CookedAnimationAssetWriter final
{
  public:
	static bool StageAnimationAssets(
	    const std::vector<CookedAnimationAssetBuild>& animationAssets,
	    std::vector<Files::FilePublication>& outPublication,
	    std::string& outErrorMessage);
};

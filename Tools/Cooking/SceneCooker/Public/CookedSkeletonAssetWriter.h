#pragma once

#include "CookedSceneBuild.h"
#include "Core/Public/Files/FileUtils.h"

#include <string>
#include <vector>

class CookedSkeletonAssetWriter final
{
  public:
	static bool StageSkeletonAssets(
	    const std::vector<CookedSkeletonAssetBuild>& skeletonAssets,
	    std::vector<Files::FilePublication>& outPublication,
	    std::string& outErrorMessage);
};

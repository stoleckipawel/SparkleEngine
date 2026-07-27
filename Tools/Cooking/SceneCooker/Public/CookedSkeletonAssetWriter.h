#pragma once

#include <string>
#include <vector>

namespace Files
{
	struct FilePublication;
}

struct CookedSkeletonAssetBuild;

class CookedSkeletonAssetWriter final
{
  public:
	static bool StageSkeletonAssets(
	    const std::vector<CookedSkeletonAssetBuild>& skeletonAssets,
	    std::vector<Files::FilePublication>& outPublication,
	    std::string& outErrorMessage);
};

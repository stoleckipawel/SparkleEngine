#pragma once

#include <vector>

namespace Files
{
	struct FilePublication;
}

struct CookedSkeletonAssetBuild;

class CookedSkeletonAssetWriter final
{
public:
	static void StageSkeletonAssets(
	    const std::vector<CookedSkeletonAssetBuild>& skeletonAssets,
	    std::vector<Files::FilePublication>& outPublication);
};

#pragma once

#include <vector>

namespace Files
{
	struct FilePublication;
}

struct CookedAnimationAssetBuild;

class CookedAnimationAssetWriter final
{
public:
	static void StageAnimationAssets(
	    const std::vector<CookedAnimationAssetBuild>& animationAssets,
	    std::vector<Files::FilePublication>& outPublication);
};

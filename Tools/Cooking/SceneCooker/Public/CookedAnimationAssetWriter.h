#pragma once

#include <string>
#include <vector>

namespace Files
{
	struct FilePublication;
}

struct CookedAnimationAssetBuild;

class CookedAnimationAssetWriter final
{
  public:
	static bool StageAnimationAssets(
	    const std::vector<CookedAnimationAssetBuild>& animationAssets,
	    std::vector<Files::FilePublication>& outPublication,
	    std::string& outErrorMessage);
};

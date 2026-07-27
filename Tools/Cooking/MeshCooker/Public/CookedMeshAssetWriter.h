#pragma once

#include <string>
#include <vector>

namespace Files
{
	struct FilePublication;
}

struct CookedMeshAssetBuild;

class CookedMeshAssetWriter final
{
  public:
	static bool StageMeshAssets(
	    const std::vector<CookedMeshAssetBuild>& meshAssets,
	    std::vector<Files::FilePublication>& outPublication,
	    std::string& outErrorMessage);
};

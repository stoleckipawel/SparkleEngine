#pragma once

#include "CookedMeshAssetBuild.h"
#include "Core/Public/Files/FileUtils.h"

#include <string>
#include <vector>

class CookedMeshAssetWriter final
{
  public:
	static bool StageMeshAssets(
	    const std::vector<CookedMeshAssetBuild>& meshAssets,
	    std::vector<Files::FilePublication>& outPublication,
	    std::string& outErrorMessage);
};

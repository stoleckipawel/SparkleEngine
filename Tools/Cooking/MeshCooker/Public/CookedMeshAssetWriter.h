#pragma once

#include "CookedMeshAssetBuild.h"

#include <string>
#include <vector>

class CookedMeshAssetWriter final
{
  public:
	static bool WriteMeshAssets(const std::vector<CookedMeshAssetBuild>& meshAssets, std::string& outErrorMessage);
};

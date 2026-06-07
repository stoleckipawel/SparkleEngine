#pragma once

#include "CookedMeshAssetBuild.h"
#include "SourceImportResult.h"

#include <string_view>

class CookedMeshAssetBuilder final
{
  public:
	static MeshCookOutput BuildMeshAssets(const SourceImportResult& importResult, std::string_view sceneAssetId);
};

#pragma once

#include "CookedMeshAssetBuild.h"
#include "SourceImportOutput.h"

#include <string_view>

class CookedMeshAssetBuilder final
{
public:
	static MeshCookOutput BuildMeshAssets(const SourceImportOutput& importOutput, std::string_view sceneAssetId);
};

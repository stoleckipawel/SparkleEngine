#pragma once

#include "CookedMeshAssetBuild.h"
#include "SourceImportResult.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

class MeshCooker final
{
public:
	static MeshCookOutput BuildMeshAssets(const SourceImportResult& importResult, std::string_view sceneAssetId);
	static bool WriteMeshAssets(const std::vector<CookedMeshAssetBuild>& meshAssets, std::string& outErrorMessage);

private:
	static Assets::CookedAssetId BuildMeshAssetId(std::string_view sceneAssetId, std::size_t meshIndex) noexcept;
};

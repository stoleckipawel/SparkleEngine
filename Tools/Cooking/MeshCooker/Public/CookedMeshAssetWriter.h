#pragma once

#include <vector>

namespace Files
{
	struct FilePublication;
}

struct CookedMeshAssetBuild;

class CookedMeshAssetWriter final
{
public:
	static void StageMeshAssets(const std::vector<CookedMeshAssetBuild>& meshAssets, std::vector<Files::FilePublication>& outPublication);
};

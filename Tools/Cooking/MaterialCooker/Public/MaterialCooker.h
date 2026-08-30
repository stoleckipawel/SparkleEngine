#pragma once

#include <string_view>
#include <vector>

namespace Files
{
	struct FilePublication;
}

struct CookedMaterialAssetBuild;
struct MaterialCookOutput;
struct SourceImportOutput;
struct TextureCookRequest;

class MaterialCooker final
{
public:
	static MaterialCookOutput BuildMaterialAssets(const SourceImportOutput& importOutput, std::string_view sceneAssetId);
	static std::vector<TextureCookRequest> CollectTextureCookRequests(const SourceImportOutput& importOutput);
	static void StageMaterialAssets(
	    const std::vector<CookedMaterialAssetBuild>& materialAssets,
	    std::vector<Files::FilePublication>& outPublication);
};

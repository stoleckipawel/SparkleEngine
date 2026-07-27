#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace Files
{
	struct FilePublication;
}

struct CookedMaterialAssetBuild;
struct MaterialCookOutput;
struct SourceImportResult;
struct TextureCookRequest;

class MaterialCooker final
{
  public:
	static bool BuildMaterialAssets(
	    const SourceImportResult& importResult,
	    std::string_view sceneAssetId,
	    MaterialCookOutput& outOutput,
	    std::string& outErrorMessage);
	static bool CollectTextureCookRequests(
	    const SourceImportResult& importResult,
	    std::vector<TextureCookRequest>& outRequests,
	    std::string& outErrorMessage);
	static bool StageMaterialAssets(
	    const std::vector<CookedMaterialAssetBuild>& materialAssets,
	    std::vector<Files::FilePublication>& outPublication,
	    std::string& outErrorMessage);
};

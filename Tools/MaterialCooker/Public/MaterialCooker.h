#pragma once

#include "CookedMaterialAssetBuild.h"
#include "GameFramework/Public/Assets/Cooked/CookedSceneManifest.h"
#include "SourceImportResult.h"
#include "TextureCookRequestList.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

class MaterialCooker final
{
public:
	static bool BuildMaterialAssets(
	    const SourceImportResult& importResult,
	    std::string_view sceneAssetId,
	    std::vector<CookedMaterialAssetBuild>& outMaterialAssets,
	    std::vector<Assets::CookedSceneMaterialAssetRef>& outMaterialAssetReferences,
	    std::string& outErrorMessage);
	static bool CollectTextureCookRequests(
	    const SourceImportResult& importResult,
	    std::vector<TextureCookRequest>& outRequests,
	    std::string& outErrorMessage);
	static bool WriteMaterialAssets(
	    const std::vector<CookedMaterialAssetBuild>& materialAssets,
	    std::string& outErrorMessage);

private:
	static Assets::CookedAssetId BuildMaterialAssetId(std::string_view sceneAssetId, std::size_t materialIndex) noexcept;
	static Assets::CookedAlphaMode TranslateAlphaMode(AlphaMode alphaMode) noexcept;
};

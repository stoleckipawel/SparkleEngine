#include "PCH.h"

#include "Assets/Payload/SceneAssetPayloadMaterialAppender.h"

#include "Assets/Cooked/LoadedMaterialAsset.h"
#include "Assets/CookedAssembly/CookedMaterialTranslator.h"
#include "Assets/Loaders/MaterialAssetLoader.h"
#include "Assets/Loaders/CookedAssetFileSet.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Paths/DirectoryPaths.h"

#include <filesystem>
#include <format>
#include <utility>

namespace Assets
{
	bool SceneAssetPayloadMaterialAppender::AppendMaterials(
	    const LoadedSceneManifest& sceneManifest,
	    const CookedAssetFileSet& files,
	    SceneAssetPayload& sceneAssetPayload,
	    std::string& errorMessage)
	{
		MaterialAssetLoader materialAssetLoader;
		CookedMaterialTranslator materialTranslator;
		sceneAssetPayload.materials.reserve(sceneAssetPayload.materials.size() + sceneManifest.materialAssetReferences.size());

		for (const CookedSceneMaterialAssetRef& materialReference : sceneManifest.materialAssetReferences)
		{
			LoadedMaterialAsset materialAsset;
			const std::filesystem::path materialAssetPath = Paths::CookedMaterialAsset(materialReference.materialAssetId);
			if (!materialAssetLoader.Decode(materialAssetPath, files.Find(materialAssetPath), materialAsset, errorMessage))
			{
				errorMessage = std::format(
				    "Failed to load cooked material asset {} from '{}' - {}",
				    Formatting::FormatHexUInt64(materialReference.materialAssetId),
				    materialAssetPath.string(),
				    errorMessage);
				return false;
			}

			MaterialDesc runtimeMaterial;
			if (!materialTranslator.Translate(materialAsset, runtimeMaterial, errorMessage))
			{
				errorMessage = std::format(
				    "Failed to translate cooked material asset {} from '{}' - {}",
				    Formatting::FormatHexUInt64(materialReference.materialAssetId),
				    materialAssetPath.string(),
				    errorMessage);
				return false;
			}

			sceneAssetPayload.materials.push_back(std::move(runtimeMaterial));
		}

		return true;
	}
}

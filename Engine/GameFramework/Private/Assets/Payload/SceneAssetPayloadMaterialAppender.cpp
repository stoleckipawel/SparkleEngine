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
	void SceneAssetPayloadMaterialAppender::AppendMaterials(
	    const LoadedSceneManifest& sceneManifest,
	    CookedAssetFileSet& files,
	    SceneAssetPayload& sceneAssetPayload)
	{
		MaterialAssetLoader materialAssetLoader;
		CookedMaterialTranslator materialTranslator;
		sceneAssetPayload.materials.reserve(sceneAssetPayload.materials.size() + sceneManifest.materialAssetReferences.size());

		for (const CookedSceneMaterialAssetRef& materialReference : sceneManifest.materialAssetReferences)
		{
			const std::filesystem::path materialAssetPath = Paths::CookedMaterialAsset(materialReference.materialAssetId);
			const LoadedMaterialAsset materialAsset = materialAssetLoader.Decode(materialAssetPath, files.Get(materialAssetPath));
			sceneAssetPayload.materials.push_back(materialTranslator.Translate(materialAsset));
			files.Release(materialAssetPath);
		}
	}
}

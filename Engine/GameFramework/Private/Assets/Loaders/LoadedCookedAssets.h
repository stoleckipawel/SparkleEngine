#pragma once

#include "Assets/Cooked/CookedMaterialAsset.h"
#include "Assets/Cooked/CookedSceneManifest.h"

#include <string>
#include <vector>

namespace Engine::Assets
{
	struct LoadedSceneManifest
	{
		CookedSceneManifestHeader header;
		std::vector<CookedSceneMeshAssetRef> meshAssetReferences;
		std::vector<CookedSceneMaterialAssetRef> materialAssetReferences;
		std::vector<CookedSceneInstanceRecord> instances;
	};

	struct LoadedMaterialAsset
	{
		CookedMaterialAssetHeader header;
		std::string name;
		std::vector<CookedTextureReference> textureReferences;
	};
}
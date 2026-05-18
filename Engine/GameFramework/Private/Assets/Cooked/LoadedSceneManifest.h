#pragma once

#include "Assets/Cooked/CookedSceneManifest.h"

#include <vector>

namespace Assets
{
	struct LoadedSceneManifest
	{
		CookedSceneManifestHeader header;
		std::vector<CookedSceneMeshAssetRef> meshAssetReferences;
		std::vector<CookedSceneMaterialAssetRef> materialAssetReferences;
		std::vector<CookedSceneInstanceRecord> instances;
		std::vector<CookedSceneInstanceGroupRecord> instanceGroups;
	};
}

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
		std::vector<CookedSceneCameraRecord> cameras;
		std::vector<CookedSceneLightRecord> lights;
		std::vector<CookedSceneSkeletonRef> skeletonRefs;
		std::vector<CookedSceneAnimationRef> animationRefs;
		std::vector<float> morphWeights;
		std::vector<CookedSceneMaterialVariantRecord> materialVariants;
		std::vector<CookedSceneMaterialVariantMappingRecord> materialVariantMappings;
	};
}

#pragma once

#include "Assets/Cooked/LoadedSceneManifest.h"
#include "Assets/SceneAssetPayload.h"

#include <vector>

namespace Assets
{
	struct SceneAssetPayloadMeshBinding final
	{
		CookedMeshAssetKind kind = CookedMeshAssetKind::Static;
		SceneMeshAssetIndex payloadMeshAssetIndex = kInvalidSceneMeshAssetIndex;
	};

	std::vector<SceneAssetPayloadMeshBinding> BuildSceneAssetPayloadMeshBindings(const LoadedSceneManifest& sceneManifest);
}

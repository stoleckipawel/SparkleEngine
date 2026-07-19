#pragma once

#include "Assets/Cooked/LoadedSceneManifest.h"
#include "Assets/SceneAssetPayload.h"
#include "Level/LevelDesc.h"

#include <cstdint>
#include <string>

namespace Assets
{
	class CookedAssetFileSet;
	class SceneAssetPayloadMeshAppender final
	{
	  public:
		static bool AppendMeshAssets(
		    const SceneAssetId& sceneAssetId,
		    const LoadedSceneManifest& sceneManifest,
		    const CookedAssetFileSet& files,
		    SceneAssetPayload& sceneAssetPayload,
		    SceneMeshAssetIndex& outMeshAssetBaseIndex,
		    std::string& errorMessage);

		static bool AppendMeshInstances(
		    const SceneAssetId& sceneAssetId,
		    const LoadedSceneManifest& sceneManifest,
		    SceneAssetPayload& sceneAssetPayload,
		    SceneMeshAssetIndex meshAssetBaseIndex,
		    SceneMeshInstanceGroupIndex groupBaseIndex,
		    std::uint32_t materialBaseIndex,
		    std::string& errorMessage);

		static bool AppendMeshInstanceGroups(
		    const SceneAssetId& sceneAssetId,
		    const LoadedSceneManifest& sceneManifest,
		    SceneAssetPayload& sceneAssetPayload,
		    SceneMeshAssetIndex meshAssetBaseIndex,
		    SceneMeshInstanceIndex instanceBaseIndex,
		    std::uint32_t materialBaseIndex,
		    std::string& errorMessage);
	};
}

#pragma once

#include "Assets/Payload/SceneAssetPayloadMeshBindings.h"
#include "Level/LevelDesc.h"

#include <cstdint>
#include <span>
namespace Assets
{
	class CookedAssetFileSet;
	class SceneAssetPayloadMeshAppender final
	{
	public:
		static void AppendMeshAssets(
		    const LoadedSceneManifest& sceneManifest,
		    CookedAssetFileSet& files,
		    SceneAssetPayload& sceneAssetPayload);

		static void AppendMeshInstances(
		    const LoadedSceneManifest& sceneManifest,
		    std::span<const SceneAssetPayloadMeshBinding> meshAssetBindings,
		    SceneAssetPayload& sceneAssetPayload);

		static void AppendMeshInstanceGroups(
		    const LoadedSceneManifest& sceneManifest,
		    std::span<const SceneAssetPayloadMeshBinding> meshAssetBindings,
		    SceneAssetPayload& sceneAssetPayload);
	};
}

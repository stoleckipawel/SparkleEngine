#pragma once

#include "Assets/Cooked/LoadedSceneManifest.h"
#include "Assets/SceneAssetPayload.h"

namespace Assets
{
	class SceneAssetPayloadMetadataAppender final
	{
	  public:
		static void AppendSceneMetadata(const LoadedSceneManifest& sceneManifest, SceneAssetPayload& sceneAssetPayload);
		static void RecordDiagnostics(const LoadedSceneManifest& sceneManifest, SceneAssetPayload& sceneAssetPayload);
	};
}

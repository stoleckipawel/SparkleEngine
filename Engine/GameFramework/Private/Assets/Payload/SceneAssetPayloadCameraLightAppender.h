#pragma once

#include "Assets/Cooked/LoadedSceneManifest.h"
#include "Assets/SceneAssetPayload.h"

namespace Assets
{
	class SceneAssetPayloadCameraLightAppender final
	{
	public:
		static void Append(const LoadedSceneManifest& sceneManifest, SceneAssetPayload& sceneAssetPayload);
	};
}

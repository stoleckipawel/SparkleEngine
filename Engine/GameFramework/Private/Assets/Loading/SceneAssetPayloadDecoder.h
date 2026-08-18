#pragma once

#include "Assets/SceneAssetPayload.h"

namespace Assets
{
	struct LoadedSceneManifest;
	class CookedAssetFileSet;

	class SceneAssetPayloadDecoder final
	{
	public:
		static SceneAssetPayload Decode(const LoadedSceneManifest& manifest, CookedAssetFileSet& files);
	};
}

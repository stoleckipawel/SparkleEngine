#pragma once

#include "Assets/SceneAssetPayload.h"
#include "Assets/Cooked/LoadedSceneManifest.h"

namespace Assets
{
	SceneAssetPayload::Camera BuildSceneAssetCamera(const CookedSceneCameraRecord& cameraRecord);
}

#pragma once

#include "Assets/SceneAssetPayload.h"
#include "Assets/Cooked/LoadedSceneManifest.h"

#include <cstddef>

namespace Assets
{
	SceneAssetPayload::Camera BuildSceneAssetCamera(const CookedSceneCameraRecord& cameraRecord, std::size_t cameraIndex);
}

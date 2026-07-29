#include "PCH.h"

#include "Assets/Payload/SceneAssetPayloadCameraLightAppender.h"

#include "Assets/Translators/SceneAssetCameraTranslator.h"
#include "Assets/Translators/SceneAssetLightTranslator.h"

#include <cstddef>

namespace Assets
{
	void SceneAssetPayloadCameraLightAppender::Append(const LoadedSceneManifest& sceneManifest, SceneAssetPayload& sceneAssetPayload)
	{
		sceneAssetPayload.cameras.reserve(sceneAssetPayload.cameras.size() + sceneManifest.cameras.size());
		for (std::size_t cameraIndex = 0; cameraIndex < sceneManifest.cameras.size(); ++cameraIndex)
			sceneAssetPayload.cameras.push_back(BuildSceneAssetCamera(sceneManifest.cameras[cameraIndex]));

		sceneAssetPayload.lights.reserve(sceneAssetPayload.lights.size() + sceneManifest.lights.size());
		for (std::size_t lightIndex = 0; lightIndex < sceneManifest.lights.size(); ++lightIndex)
			sceneAssetPayload.lights.push_back(BuildSceneAssetLight(sceneManifest.lights[lightIndex]));
	}
}

#include "PCH.h"

#include "Assets/Payload/SceneAssetPayloadMetadataAppender.h"

#include "Assets/Translators/SceneAssetCameraTranslator.h"
#include "Assets/Translators/SceneAssetLightTranslator.h"

#include <cstddef>

namespace Assets
{
	void SceneAssetPayloadMetadataAppender::AppendSceneMetadata(
	    const LoadedSceneManifest& sceneManifest,
	    SceneAssetPayload& sceneAssetPayload)
	{
		sceneAssetPayload.cameras.reserve(sceneAssetPayload.cameras.size() + sceneManifest.cameras.size());
		for (std::size_t cameraIndex = 0; cameraIndex < sceneManifest.cameras.size(); ++cameraIndex)
		{
			sceneAssetPayload.cameras.push_back(BuildSceneAssetCamera(sceneManifest.cameras[cameraIndex], cameraIndex));
		}

		sceneAssetPayload.lights.reserve(sceneAssetPayload.lights.size() + sceneManifest.lights.size());
		for (std::size_t lightIndex = 0; lightIndex < sceneManifest.lights.size(); ++lightIndex)
		{
			sceneAssetPayload.lights.push_back(BuildSceneAssetLight(sceneManifest.lights[lightIndex], lightIndex));
		}
	}

	void SceneAssetPayloadMetadataAppender::RecordDiagnostics(
	    const LoadedSceneManifest& sceneManifest,
	    SceneAssetPayload& sceneAssetPayload)
	{
		sceneAssetPayload.diagnostics.loadedSceneAssetCount += 1u;
		sceneAssetPayload.diagnostics.meshAssetReferenceCount += sceneManifest.meshAssetReferences.size();
		sceneAssetPayload.diagnostics.meshInstanceCount += sceneManifest.instances.size();
		sceneAssetPayload.diagnostics.meshInstanceGroupCount += sceneManifest.instanceGroups.size();
		sceneAssetPayload.diagnostics.cameraCount += sceneManifest.cameras.size();
		sceneAssetPayload.diagnostics.lightCount += sceneManifest.lights.size();
		sceneAssetPayload.diagnostics.skeletonRefCount += sceneManifest.skeletonRefs.size();
		sceneAssetPayload.diagnostics.animationRefCount += sceneManifest.animationRefs.size();
		sceneAssetPayload.diagnostics.sceneFeatureFlags |= sceneManifest.header.featureFlags;
	}
}

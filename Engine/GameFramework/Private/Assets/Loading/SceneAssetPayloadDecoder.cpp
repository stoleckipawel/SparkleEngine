#include "PCH.h"

#include "Assets/Loading/SceneAssetPayloadDecoder.h"

#include "Assets/Cooked/LoadedSceneManifest.h"
#include "Assets/Loaders/CookedAssetFileSet.h"
#include "Assets/Payload/SceneAssetPayloadAnimationAppender.h"
#include "Assets/Payload/SceneAssetPayloadCameraLightAppender.h"
#include "Assets/Payload/SceneAssetPayloadMaterialAppender.h"
#include "Assets/Payload/SceneAssetPayloadMaterialVariantAppender.h"
#include "Assets/Payload/SceneAssetPayloadMeshAppender.h"
#include "Assets/Payload/SceneAssetPayloadMeshBindings.h"
#include "Assets/Payload/SceneAssetPayloadSkeletonAppender.h"

namespace Assets
{
	bool SceneAssetPayloadDecoder::Decode(
	    const SceneAssetId& sceneAssetId,
	    LoadedSceneManifest& manifest,
	    const CookedAssetFileSet& files,
	    SceneAssetPayload& payload,
	    std::string& errorMessage)
	{
		constexpr std::uint32_t materialBaseIndex = 0;
		SceneMeshAssetIndex meshAssetBaseIndex = kInvalidSceneMeshAssetIndex;
		const auto instanceBaseIndex = static_cast<SceneMeshInstanceIndex>(payload.staticMeshInstances.size());
		const auto groupBaseIndex = static_cast<SceneMeshInstanceGroupIndex>(payload.meshInstanceGroups.size());
		const std::vector<SceneAssetPayloadMeshBinding> meshAssetBindings = BuildSceneAssetPayloadMeshBindings(manifest);
		if (!SceneAssetPayloadMeshAppender::AppendMeshAssets(sceneAssetId, manifest, files, payload, meshAssetBaseIndex, errorMessage) ||
		    !SceneAssetPayloadMaterialAppender::AppendMaterials(manifest, files, payload, errorMessage) ||
		    !SceneAssetPayloadMeshAppender::AppendMeshInstances(
		        sceneAssetId,
		        manifest,
		        meshAssetBindings,
		        payload,
		        meshAssetBaseIndex,
		        groupBaseIndex,
		        materialBaseIndex,
		        errorMessage) ||
		    !SceneAssetPayloadMeshAppender::AppendMeshInstanceGroups(
		        sceneAssetId,
		        manifest,
		        meshAssetBindings,
		        payload,
		        meshAssetBaseIndex,
		        instanceBaseIndex,
		        materialBaseIndex,
		        errorMessage) ||
		    !SceneAssetPayloadMaterialVariantAppender::AppendMaterialVariants(
		        manifest,
		        meshAssetBindings,
		        payload,
		        materialBaseIndex,
		        errorMessage) ||
		    !SceneAssetPayloadSkeletonAppender::AppendSkeletons(manifest, files, payload, errorMessage) ||
		    !SceneAssetPayloadAnimationAppender::AppendAnimations(manifest, files, payload, errorMessage))
			return false;

		SceneAssetPayloadCameraLightAppender::Append(manifest, payload);
		errorMessage.clear();
		return true;
	}
}

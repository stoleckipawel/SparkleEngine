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
	SceneAssetPayload SceneAssetPayloadDecoder::Decode(
	    const LoadedSceneManifest& manifest,
	    const CookedAssetFileSet& files)
	{
		SceneAssetPayload payload;
		const std::vector<SceneAssetPayloadMeshBinding> meshAssetBindings = BuildSceneAssetPayloadMeshBindings(manifest);
		SceneAssetPayloadMeshAppender::AppendMeshAssets(manifest, files, payload);
		SceneAssetPayloadMaterialAppender::AppendMaterials(manifest, files, payload);
		SceneAssetPayloadMeshAppender::AppendMeshInstances(manifest, meshAssetBindings, payload);
		SceneAssetPayloadMeshAppender::AppendMeshInstanceGroups(manifest, meshAssetBindings, payload);
		SceneAssetPayloadMaterialVariantAppender::AppendMaterialVariants(manifest, meshAssetBindings, payload);
		SceneAssetPayloadSkeletonAppender::AppendSkeletons(manifest, files, payload);
		SceneAssetPayloadAnimationAppender::AppendAnimations(manifest, files, payload);
		SceneAssetPayloadCameraLightAppender::Append(manifest, payload);
		return payload;
	}
}

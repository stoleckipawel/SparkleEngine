#include "PCH.h"

#include "Assets/SceneAssetPayloadLoader.h"

#include "Assets/Cooked/LoadedSceneManifest.h"
#include "Assets/Loaders/SceneManifestLoader.h"
#include "Assets/Payload/SceneAssetPayloadMaterialAppender.h"
#include "Assets/Payload/SceneAssetPayloadMaterialVariantAppender.h"
#include "Assets/Payload/SceneAssetPayloadAnimationAppender.h"
#include "Assets/Payload/SceneAssetPayloadMeshAppender.h"
#include "Assets/Payload/SceneAssetPayloadMetadataAppender.h"
#include "Assets/Payload/SceneAssetPayloadSkeletonAppender.h"
#include "Core/Public/Paths/DirectoryPaths.h"

#include <format>

namespace Assets
{
	bool SceneAssetPayloadLoader::AppendSceneAsset(
	    const SceneAssetId& sceneAssetId,
	    const std::filesystem::path& manifestRelativePath,
	    SceneAssetPayload& sceneAssetPayload,
	    std::uint32_t& materialBaseIndex,
	    std::string& errorMessage)
	{
		SceneManifestLoader sceneManifestLoader;
		LoadedSceneManifest sceneManifest;
		const std::filesystem::path sceneManifestPath = Paths::CookedSceneManifestRelative(manifestRelativePath);

		if (!sceneManifestLoader.Load(sceneManifestPath, sceneManifest, errorMessage))
		{
			errorMessage = std::format(
			    "Failed to load cooked scene manifest for '{}' from '{}' - {}",
			    sceneAssetId.value,
			    sceneManifestPath.string(),
			    errorMessage);
			return false;
		}

		SceneMeshAssetIndex meshAssetBaseIndex = kInvalidSceneMeshAssetIndex;
		const auto instanceBaseIndex = static_cast<SceneMeshInstanceIndex>(sceneAssetPayload.staticMeshInstances.size());
		const auto groupBaseIndex = static_cast<SceneMeshInstanceGroupIndex>(sceneAssetPayload.meshInstanceGroups.size());
		if (!SceneAssetPayloadMeshAppender::AppendMeshAssets(
		        sceneAssetId,
		        sceneManifest,
		        sceneAssetPayload,
		        meshAssetBaseIndex,
		        errorMessage) ||
		    !SceneAssetPayloadMaterialAppender::AppendMaterials(sceneManifest, sceneAssetPayload, errorMessage) ||
		    !SceneAssetPayloadMeshAppender::AppendMeshInstances(
		        sceneAssetId,
		        sceneManifest,
		        sceneAssetPayload,
		        meshAssetBaseIndex,
		        groupBaseIndex,
		        materialBaseIndex,
		        errorMessage) ||
		    !SceneAssetPayloadMeshAppender::AppendMeshInstanceGroups(
		        sceneAssetId,
		        sceneManifest,
		        sceneAssetPayload,
		        meshAssetBaseIndex,
		        instanceBaseIndex,
		        materialBaseIndex,
		        errorMessage) ||
		    !SceneAssetPayloadMaterialVariantAppender::AppendMaterialVariants(
		        sceneManifest,
		        sceneAssetPayload,
		        materialBaseIndex,
		        errorMessage) ||
		    !SceneAssetPayloadSkeletonAppender::AppendSkeletons(sceneManifest, sceneAssetPayload, errorMessage) ||
		    !SceneAssetPayloadAnimationAppender::AppendAnimations(sceneManifest, sceneAssetPayload, errorMessage))
		{
			return false;
		}

		SceneAssetPayloadMetadataAppender::AppendSceneMetadata(sceneManifest, sceneAssetPayload);
		SceneAssetPayloadMetadataAppender::RecordDiagnostics(sceneManifest, sceneAssetPayload);

		materialBaseIndex += static_cast<std::uint32_t>(sceneManifest.materialAssetReferences.size());
		errorMessage.clear();
		return true;
	}
}

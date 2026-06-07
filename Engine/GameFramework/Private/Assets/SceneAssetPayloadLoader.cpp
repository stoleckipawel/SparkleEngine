#include "PCH.h"

#include "Assets/SceneAssetPayloadLoader.h"

#include "Assets/Cooked/LoadedSceneManifest.h"
#include "Assets/Loaders/SceneManifestLoader.h"
#include "Assets/Payload/SceneAssetPayloadMaterialAppender.h"
#include "Assets/Payload/SceneAssetPayloadAnimationAppender.h"
#include "Assets/Payload/SceneAssetPayloadMeshAppender.h"
#include "Assets/Payload/SceneAssetPayloadMetadataAppender.h"
#include "Assets/Payload/SceneAssetPayloadSkeletonAppender.h"
#include "Core/Public/Paths/DirectoryPaths.h"

#include <format>

static const auto g_sceneAssetPayloadLoaderLogger = Logging::GetOrCreateLogger("GameFramework.SceneAssets");

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
		    !SceneAssetPayloadSkeletonAppender::AppendSkeletons(sceneManifest, sceneAssetPayload, errorMessage) ||
		    !SceneAssetPayloadAnimationAppender::AppendAnimations(sceneManifest, sceneAssetPayload, errorMessage))
		{
			return false;
		}

		SceneAssetPayloadMetadataAppender::AppendSceneMetadata(sceneManifest, sceneAssetPayload);
		SceneAssetPayloadMetadataAppender::RecordDiagnostics(sceneManifest, sceneAssetPayload);

		SPDLOG_LOGGER_INFO(
		    g_sceneAssetPayloadLoaderLogger,
		    "SceneAssetManager: Loaded scene asset '{}' - meshAssetRefs={}, meshInstances={}, instanceGroups={}, materials={}, cameras={}, lights={}, skeletonRefs={}, loadedSkeletons={}, animationRefs={}, featureFlags=0x{:08X}",
		    sceneAssetId.value,
		    sceneManifest.meshAssetReferences.size(),
		    sceneManifest.instances.size(),
		    sceneManifest.instanceGroups.size(),
		    sceneManifest.materialAssetReferences.size(),
		    sceneManifest.cameras.size(),
		    sceneManifest.lights.size(),
		    sceneManifest.skeletonRefs.size(),
		    sceneManifest.skeletonRefs.size(),
		    sceneManifest.animationRefs.size(),
		    sceneManifest.header.featureFlags);

		materialBaseIndex += static_cast<std::uint32_t>(sceneManifest.materialAssetReferences.size());
		errorMessage.clear();
		return true;
	}
}

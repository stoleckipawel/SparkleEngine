#include "PCH.h"

#include "Assets/Loading/SceneAssetFileReader.h"

#include "Assets/Cooked/LoadedSceneManifest.h"
#include "Assets/Loaders/CookedAssetFileSet.h"
#include "Assets/Loaders/SceneManifestLoader.h"
#include "Core/Public/Paths/DirectoryPaths.h"

#include <format>

namespace Assets
{
	bool SceneAssetFileReader::Read(
	    const SceneAssetId& sceneAssetId,
	    const std::filesystem::path& manifestRelativePath,
	    LoadedSceneManifest& manifest,
	    CookedAssetFileSet& files,
	    std::atomic<std::size_t>& retainedBytes,
	    std::size_t maximumBytes,
	    std::string& errorMessage)
	{
		const std::filesystem::path manifestPath = Paths::CookedSceneManifestRelative(manifestRelativePath);
		if (!files.Read(manifestPath, retainedBytes, maximumBytes, errorMessage) ||
		    !SceneManifestLoader{}.Decode(manifestPath, files.Find(manifestPath), manifest, errorMessage))
		{
			errorMessage = std::format(
			    "Failed to load cooked scene manifest for '{}' from '{}' - {}", sceneAssetId.value, manifestPath.string(), errorMessage);
			return false;
		}
		for (const CookedSceneMeshAssetRef& reference : manifest.meshAssetReferences)
			if (!files.Read(Paths::CookedMeshAsset(reference.meshAssetId), retainedBytes, maximumBytes, errorMessage))
				return false;
		for (const CookedSceneMaterialAssetRef& reference : manifest.materialAssetReferences)
			if (!files.Read(Paths::CookedMaterialAsset(reference.materialAssetId), retainedBytes, maximumBytes, errorMessage))
				return false;
		for (const CookedSceneSkeletonRef& reference : manifest.skeletonRefs)
			if (!files.Read(Paths::CookedSkeletonAsset(reference.skeletonAssetId), retainedBytes, maximumBytes, errorMessage))
				return false;
		for (const CookedAnimationReference& reference : manifest.animationReferences)
			if (!files.Read(Paths::CookedAnimationAsset(reference.animationAssetId), retainedBytes, maximumBytes, errorMessage))
				return false;
		errorMessage.clear();
		return true;
	}
}

#include "PCH.h"

#include "Assets/Loading/SceneAssetFileReader.h"

#include "Assets/Cooked/LoadedSceneManifest.h"
#include "Assets/Loaders/CookedAssetFileSet.h"
#include "Assets/Loaders/SceneManifestLoader.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Paths/DirectoryPaths.h"

#include <format>

namespace Assets
{
	void SceneAssetFileReader::Read(
	    const SceneAssetId& sceneAssetId,
	    const std::filesystem::path& manifestRelativePath,
	    LoadedSceneManifest& manifest,
	    CookedAssetFileSet& files)
	{
		const std::filesystem::path manifestPath = Paths::CookedSceneManifestRelative(manifestRelativePath);
		try
		{
			files.Read(manifestPath);
			manifest = SceneManifestLoader{}.Decode(manifestPath, files.Get(manifestPath));
			files.Release(manifestPath);

			for (const CookedSceneMeshAssetRef& reference : manifest.meshAssetReferences)
				files.Read(Paths::CookedMeshAsset(reference.meshAssetId));
			for (const CookedSceneMaterialAssetRef& reference : manifest.materialAssetReferences)
				files.Read(Paths::CookedMaterialAsset(reference.materialAssetId));
			for (const CookedSceneSkeletonRef& reference : manifest.skeletonRefs)
				files.Read(Paths::CookedSkeletonAsset(reference.skeletonAssetId));
			for (const CookedAnimationReference& reference : manifest.animationReferences)
				files.Read(Paths::CookedAnimationAsset(reference.animationAssetId));
		}
		catch (const Diagnostics::Error& error)
		{
			throw Diagnostics::Error(
			    std::format(
			        "Could not load scene asset '{}' from '{}': {}",
			        sceneAssetId.value,
			        manifestPath.generic_string(),
			        error.what()));
		}
	}
}

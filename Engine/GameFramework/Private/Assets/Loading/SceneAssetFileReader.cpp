#include "PCH.h"

#include "Assets/Loading/SceneAssetFileReader.h"

#include "Assets/Cooked/LoadedSceneManifest.h"
#include "Assets/Loaders/CookedAssetFileSet.h"
#include "Assets/Loaders/SceneManifestLoader.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "Level/Loading/SceneLoadBudget.h"

#include <format>

namespace Assets
{
	namespace SceneAssetFileReaderDetail
	{
		template <typename T> std::size_t VectorBytes(const std::vector<T>& values) noexcept
		{
			return values.capacity() * sizeof(T);
		}

		std::size_t GetRetainedManifestBytes(const LoadedSceneManifest& manifest) noexcept
		{
			return VectorBytes(manifest.meshAssetReferences) + VectorBytes(manifest.materialAssetReferences)
			    + VectorBytes(manifest.instances) + VectorBytes(manifest.instanceGroups) + VectorBytes(manifest.cameras)
			    + VectorBytes(manifest.lights) + VectorBytes(manifest.skeletonRefs) + VectorBytes(manifest.animationReferences)
			    + VectorBytes(manifest.morphWeights) + VectorBytes(manifest.materialVariants)
			    + VectorBytes(manifest.materialVariantMappings);
		}
	}

	std::size_t SceneAssetFileReader::Read(
	    const SceneAssetId& sceneAssetId,
	    const std::filesystem::path& manifestRelativePath,
	    LoadedSceneManifest& manifest,
	    CookedAssetFileSet& files,
	    SceneLoadBudget& budget)
	{
		const std::filesystem::path manifestPath = Paths::CookedSceneManifestRelative(manifestRelativePath);
		try
		{
			files.Read(manifestPath, budget);
			const std::size_t manifestRawBytes = files.Get(manifestPath).size_bytes();
			if (!budget.TryReserve(manifestRawBytes))
			{
				throw Diagnostics::Error("Scene load exceeded the aggregate retained-data byte budget.");
			}

			std::size_t retainedManifestBytes = manifestRawBytes;
			manifest = SceneManifestLoader{}.Decode(manifestPath, files.Get(manifestPath));
			const std::size_t measuredManifestBytes = SceneAssetFileReaderDetail::GetRetainedManifestBytes(manifest);
			if (measuredManifestBytes > retainedManifestBytes)
			{
				if (!budget.TryReserve(measuredManifestBytes - retainedManifestBytes))
				{
					throw Diagnostics::Error("Scene load exceeded the aggregate retained-data byte budget.");
				}
			}
			else
			{
				budget.Release(retainedManifestBytes - measuredManifestBytes);
			}
			retainedManifestBytes = measuredManifestBytes;

			for (const CookedSceneMeshAssetRef& reference : manifest.meshAssetReferences)
				files.Read(Paths::CookedMeshAsset(reference.meshAssetId), budget);
			for (const CookedSceneMaterialAssetRef& reference : manifest.materialAssetReferences)
				files.Read(Paths::CookedMaterialAsset(reference.materialAssetId), budget);
			for (const CookedSceneSkeletonRef& reference : manifest.skeletonRefs)
				files.Read(Paths::CookedSkeletonAsset(reference.skeletonAssetId), budget);
			for (const CookedAnimationReference& reference : manifest.animationReferences)
				files.Read(Paths::CookedAnimationAsset(reference.animationAssetId), budget);
			return retainedManifestBytes;
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

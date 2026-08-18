#include "PCH.h"

#include "Assets/Loaders/SceneManifestLoader.h"

#include "Assets/Cooked/LoadedSceneManifest.h"
#include "Assets/Loaders/CookedAssetByteReader.h"
#include "Assets/Loaders/CookedAssetLoaderDiagnostics.h"
#include "Assets/Loaders/SceneManifestValidator.h"
#include <cstdint>

namespace Assets
{
	LoadedSceneManifest SceneManifestLoader::Decode(
	    const std::filesystem::path& path,
	    std::span<const std::uint8_t> bytes) const
	{
		const CookedAssetLoaderDiagnostics diagnostics(path, "CookedSceneManifest");

		CookedAssetByteReader reader(bytes);
		LoadedSceneManifest manifest;
		manifest.header = reader.Read<CookedSceneManifestHeader>();
		SceneManifestValidator::ValidateHeader(manifest);

		manifest.meshAssetReferences = reader.ReadArray<CookedSceneMeshAssetRef>(manifest.header.meshAssetReferenceCount);
		manifest.materialAssetReferences = reader.ReadArray<CookedSceneMaterialAssetRef>(manifest.header.materialAssetReferenceCount);
		manifest.instances = reader.ReadArray<CookedSceneInstanceRecord>(manifest.header.instanceCount);
		manifest.instanceGroups = reader.ReadArray<CookedSceneInstanceGroupRecord>(manifest.header.instanceGroupCount);
		manifest.cameras = reader.ReadArray<CookedSceneCameraRecord>(manifest.header.cameraCount);
		manifest.lights = reader.ReadArray<CookedSceneLightRecord>(manifest.header.lightCount);
		manifest.skeletonRefs = reader.ReadArray<CookedSceneSkeletonRef>(manifest.header.skeletonRefCount);
		manifest.animationReferences = reader.ReadArray<CookedAnimationReference>(manifest.header.animationRefCount);
		manifest.morphWeights = reader.ReadArray<float>(manifest.header.morphWeightCount);
		manifest.materialVariants = reader.ReadArray<CookedSceneMaterialVariantRecord>(manifest.header.materialVariantCount);
		manifest.materialVariantMappings =
		    reader.ReadArray<CookedSceneMaterialVariantMappingRecord>(manifest.header.materialVariantMappingCount);
		SceneManifestValidator::ValidateRecords(manifest);

		if (reader.GetRemainingByteCount() != 0)
		{
			throw diagnostics.MakeError(
			    "payload",
			    "no trailing bytes after declared scene manifest records",
			    "Cooked scene manifest contains unexpected trailing bytes");
		}

		return manifest;
	}
}

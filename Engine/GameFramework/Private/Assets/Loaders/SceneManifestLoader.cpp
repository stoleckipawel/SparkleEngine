#include "PCH.h"

#include "Assets/Loaders/SceneManifestLoader.h"

#include "Assets/Cooked/LoadedSceneManifest.h"
#include "Assets/Loaders/CookedAssetByteReader.h"
#include "Assets/Loaders/SceneManifestValidator.h"
#include "Core/Public/Files/FileUtils.h"

#include <cstdint>
#include <vector>

namespace Assets
{
	bool SceneManifestLoader::Load(const std::filesystem::path& path, LoadedSceneManifest& outManifest, std::string& outErrorMessage) const
	{
		std::vector<std::uint8_t> fileBytes;
		if (!Files::TryReadAllBytes(path, fileBytes, outErrorMessage))
		{
			return false;
		}

		CookedAssetByteReader reader(fileBytes);
		if (!reader.Read(outManifest.header, outErrorMessage))
		{
			return false;
		}

		if (!SceneManifestValidator::ValidateHeader(outManifest, outErrorMessage))
		{
			return false;
		}

		if (!reader.ReadArray(outManifest.header.meshAssetReferenceCount, outManifest.meshAssetReferences, outErrorMessage) ||
		    !reader.ReadArray(outManifest.header.materialAssetReferenceCount, outManifest.materialAssetReferences, outErrorMessage) ||
		    !reader.ReadArray(outManifest.header.instanceCount, outManifest.instances, outErrorMessage) ||
		    !reader.ReadArray(outManifest.header.instanceGroupCount, outManifest.instanceGroups, outErrorMessage) ||
		    !reader.ReadArray(outManifest.header.cameraCount, outManifest.cameras, outErrorMessage) ||
		    !reader.ReadArray(outManifest.header.lightCount, outManifest.lights, outErrorMessage) ||
		    !reader.ReadArray(outManifest.header.skeletonRefCount, outManifest.skeletonRefs, outErrorMessage) ||
		    !reader.ReadArray(outManifest.header.animationRefCount, outManifest.animationRefs, outErrorMessage) ||
		    !reader.ReadArray(outManifest.header.morphWeightCount, outManifest.morphWeights, outErrorMessage) ||
		    !reader.ReadArray(outManifest.header.materialVariantCount, outManifest.materialVariants, outErrorMessage) ||
		    !reader.ReadArray(
		        outManifest.header.materialVariantMappingCount,
		        outManifest.materialVariantMappings,
		        outErrorMessage))
	{
		return false;
	}

		if (!SceneManifestValidator::ValidateRecords(outManifest, outErrorMessage))
		{
			return false;
		}

		if (reader.GetRemainingByteCount() != 0)
		{
			outErrorMessage = "Cooked scene manifest contains unexpected trailing bytes";
			return false;
		}

		outErrorMessage.clear();
		return true;
	}
}

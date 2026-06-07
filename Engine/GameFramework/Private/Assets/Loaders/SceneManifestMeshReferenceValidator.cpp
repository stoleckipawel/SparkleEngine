#include "PCH.h"

#include "Assets/Loaders/SceneManifestMeshReferenceValidator.h"

#include "Assets/Cooked/LoadedSceneManifest.h"

#include <cstddef>
#include <format>

namespace Assets::SceneManifestMeshReferenceValidator
{
	bool Validate(const LoadedSceneManifest& manifest, std::string& outErrorMessage)
	{
		for (std::size_t meshReferenceIndex = 0; meshReferenceIndex < manifest.meshAssetReferences.size(); ++meshReferenceIndex)
		{
			if (manifest.meshAssetReferences[meshReferenceIndex].meshAssetId == InvalidCookedAssetId)
			{
				outErrorMessage = std::format("Cooked scene manifest has an invalid mesh asset reference at index {}", meshReferenceIndex);
				return false;
			}
			if (manifest.meshAssetReferences[meshReferenceIndex].meshAssetKind != CookedMeshAssetKind::Static &&
			    manifest.meshAssetReferences[meshReferenceIndex].meshAssetKind != CookedMeshAssetKind::Skeletal)
			{
				outErrorMessage = std::format("Cooked scene manifest has an invalid mesh asset kind at index {}", meshReferenceIndex);
				return false;
			}
		}

		return true;
	}
}

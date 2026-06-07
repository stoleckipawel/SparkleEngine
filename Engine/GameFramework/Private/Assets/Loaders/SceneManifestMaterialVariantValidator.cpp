#include "PCH.h"

#include "Assets/Loaders/SceneManifestMaterialVariantValidator.h"

#include "Assets/Cooked/LoadedSceneManifest.h"

#include <cstddef>
#include <format>

namespace Assets::SceneManifestMaterialVariantValidator
{
	bool Validate(const LoadedSceneManifest& manifest, std::string& outErrorMessage)
	{
		for (std::size_t mappingIndex = 0; mappingIndex < manifest.materialVariantMappings.size(); ++mappingIndex)
		{
			const CookedSceneMaterialVariantMappingRecord& mapping = manifest.materialVariantMappings[mappingIndex];
			if (mapping.meshAssetIndex >= manifest.meshAssetReferences.size())
			{
				outErrorMessage = std::format("Cooked scene material variant mapping {} references an invalid mesh asset", mappingIndex);
				return false;
			}

			if (mapping.variantIndex >= manifest.materialVariants.size())
			{
				outErrorMessage = std::format("Cooked scene material variant mapping {} references an invalid variant", mappingIndex);
				return false;
			}

			if (mapping.materialAssetIndex >= manifest.materialAssetReferences.size())
			{
				outErrorMessage = std::format("Cooked scene material variant mapping {} references an invalid material", mappingIndex);
				return false;
			}
		}

		outErrorMessage.clear();
		return true;
	}
}

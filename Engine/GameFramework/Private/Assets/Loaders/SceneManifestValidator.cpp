#include "PCH.h"

#include "Assets/Loaders/SceneManifestValidator.h"

#include "Assets/Cooked/LoadedSceneManifest.h"
#include "Assets/Loaders/SceneManifestFeatureValidator.h"
#include "Assets/Loaders/SceneManifestMaterialVariantValidator.h"
#include "Assets/Loaders/SceneManifestMeshValidator.h"
#include "Assets/Loaders/SceneManifestMetadataValidator.h"

#include <format>

namespace Assets
{
	bool SceneManifestValidator::ValidateHeader(const LoadedSceneManifest& manifest, std::string& outErrorMessage)
	{
		if (manifest.header.fileHeader.magic != kCookedSceneManifestMagic)
		{
			outErrorMessage = "Invalid cooked scene manifest magic";
			return false;
		}

		if (manifest.header.fileHeader.version != kCookedSceneManifestVersion)
		{
			outErrorMessage = std::format(
			    "Cooked scene manifest version {} is not supported by this runtime; expected version {}. Recook the scene asset.",
			    manifest.header.fileHeader.version,
			    kCookedSceneManifestVersion);
			return false;
		}

		outErrorMessage.clear();
		return true;
	}

	bool SceneManifestValidator::ValidateRecords(const LoadedSceneManifest& manifest, std::string& outErrorMessage)
	{
		if (!SceneManifestFeatureValidator::Validate(manifest, outErrorMessage) ||
		    !SceneManifestMeshValidator::Validate(manifest, outErrorMessage) ||
		    !SceneManifestMetadataValidator::Validate(manifest, outErrorMessage) ||
		    !SceneManifestMaterialVariantValidator::Validate(manifest, outErrorMessage))
		{
			return false;
		}

		outErrorMessage.clear();
		return true;
	}
}

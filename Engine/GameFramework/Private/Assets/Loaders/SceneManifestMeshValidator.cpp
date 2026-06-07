#include "PCH.h"

#include "Assets/Loaders/SceneManifestMeshValidator.h"

#include "Assets/Cooked/LoadedSceneManifest.h"
#include "Assets/Loaders/SceneManifestInstanceGroupValidator.h"
#include "Assets/Loaders/SceneManifestInstanceValidator.h"
#include "Assets/Loaders/SceneManifestMeshReferenceValidator.h"

namespace Assets::SceneManifestMeshValidator
{
	bool Validate(const LoadedSceneManifest& manifest, std::string& outErrorMessage)
	{
		return SceneManifestMeshReferenceValidator::Validate(manifest, outErrorMessage) &&
		       SceneManifestInstanceValidator::Validate(manifest, outErrorMessage) &&
		       SceneManifestInstanceGroupValidator::Validate(manifest, outErrorMessage);
	}
}

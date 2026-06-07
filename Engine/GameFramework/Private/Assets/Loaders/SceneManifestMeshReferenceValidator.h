#pragma once

#include <string>

namespace Assets
{
	struct LoadedSceneManifest;

	namespace SceneManifestMeshReferenceValidator
	{
		bool Validate(const LoadedSceneManifest& manifest, std::string& outErrorMessage);
	}
}

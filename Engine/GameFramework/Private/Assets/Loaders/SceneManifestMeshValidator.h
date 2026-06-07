#pragma once

#include <string>

namespace Assets
{
	struct LoadedSceneManifest;

	namespace SceneManifestMeshValidator
	{
		bool Validate(const LoadedSceneManifest& manifest, std::string& outErrorMessage);
	}
}

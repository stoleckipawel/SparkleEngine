#pragma once

#include <string>

namespace Assets
{
	struct LoadedSceneManifest;

	namespace SceneManifestMetadataValidator
	{
		bool Validate(const LoadedSceneManifest& manifest, std::string& outErrorMessage);
	}
}

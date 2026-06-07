#pragma once

#include <string>

namespace Assets
{
	struct LoadedSceneManifest;

	namespace SceneManifestInstanceValidator
	{
		bool Validate(const LoadedSceneManifest& manifest, std::string& outErrorMessage);
	}
}

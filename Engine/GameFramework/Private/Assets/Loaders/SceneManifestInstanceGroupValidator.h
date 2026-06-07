#pragma once

#include <string>

namespace Assets
{
	struct LoadedSceneManifest;

	namespace SceneManifestInstanceGroupValidator
	{
		bool Validate(const LoadedSceneManifest& manifest, std::string& outErrorMessage);
	}
}

#pragma once

#include <string>

namespace Assets
{
	struct LoadedSceneManifest;

	class SceneManifestValidator final
	{
	  public:
		static bool ValidateHeader(const LoadedSceneManifest& manifest, std::string& outErrorMessage);
		static bool ValidateRecords(const LoadedSceneManifest& manifest, std::string& outErrorMessage);
	};
}

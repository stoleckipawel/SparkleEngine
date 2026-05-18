#pragma once

#include <filesystem>
#include <string>

namespace Assets
{
	struct LoadedSceneManifest;

	class SceneManifestLoader final
	{
	  public:
		bool Load(const std::filesystem::path& path, LoadedSceneManifest& outManifest, std::string& outErrorMessage) const;

	  private:
		static bool ValidateHeader(const LoadedSceneManifest& manifest, std::string& outErrorMessage);
		static bool ValidateRecords(const LoadedSceneManifest& manifest, std::string& outErrorMessage);
	};
}
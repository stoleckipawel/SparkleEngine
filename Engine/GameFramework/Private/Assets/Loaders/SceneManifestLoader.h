#pragma once

#include <filesystem>
#include <string>

namespace Engine::Assets
{
	struct LoadedSceneManifest;

	class SceneManifestLoader final
	{
	  public:
		bool Load(const std::filesystem::path& path, LoadedSceneManifest& outManifest, std::string& outErrorMessage) const;

	  private:
		static bool HasValidHeader(const LoadedSceneManifest& manifest) noexcept;
	};
}
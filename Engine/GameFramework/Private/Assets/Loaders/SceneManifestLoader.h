#pragma once

#include <filesystem>
#include <string>
#include <span>
#include <cstdint>

namespace Assets
{
	struct LoadedSceneManifest;

	class SceneManifestLoader final
	{
	  public:
		bool Decode(
		    const std::filesystem::path& path,
		    std::span<const std::uint8_t> bytes,
		    LoadedSceneManifest& outManifest,
		    std::string& outErrorMessage) const;
	};
}

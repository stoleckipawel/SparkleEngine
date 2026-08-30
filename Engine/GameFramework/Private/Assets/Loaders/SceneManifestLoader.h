#pragma once

#include <filesystem>
#include <span>
#include <cstdint>

namespace Assets
{
	struct LoadedSceneManifest;

	class SceneManifestLoader final
	{
	public:
		LoadedSceneManifest Decode(const std::filesystem::path& path, std::span<const std::uint8_t> bytes) const;
	};
}

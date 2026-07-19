#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

#include <filesystem>
#include <vector>
#include <cstdint>

struct SPARKLE_ENGINE_API TextureSnapshot
{
	std::vector<std::filesystem::path> texturePaths;
	std::uint32_t generation = 0;

	bool HasTextures() const noexcept { return !texturePaths.empty(); }

	void Reset() noexcept { texturePaths.clear(); generation = 0; }
};

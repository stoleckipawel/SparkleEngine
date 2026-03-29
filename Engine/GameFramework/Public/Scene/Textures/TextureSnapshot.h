#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

#include <filesystem>
#include <vector>

struct SPARKLE_ENGINE_API TextureSnapshot
{
	std::vector<std::filesystem::path> texturePaths;

	bool HasTextures() const noexcept { return !texturePaths.empty(); }

	void Reset() noexcept { texturePaths.clear(); }
};
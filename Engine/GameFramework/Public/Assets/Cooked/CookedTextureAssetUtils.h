#pragma once

#include "CookedAssetCommon.h"

#include <filesystem>

namespace Engine::Assets
{
	SPARKLE_ENGINE_API std::filesystem::path BuildCookedTextureAssetPath(CookedAssetId textureAssetId);
}
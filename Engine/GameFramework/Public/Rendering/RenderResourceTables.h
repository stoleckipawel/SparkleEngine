#pragma once

#include "GameFramework/Public/Scene/Materials/MaterialDesc.h"

#include <cstdint>
#include <filesystem>
#include <vector>

struct RenderMaterialTable final
{
	std::vector<MaterialDesc> Values;
	std::uint32_t Generation = 0;
};

struct RenderTextureTable final
{
	std::vector<std::filesystem::path> Paths;
	std::uint32_t Generation = 0;
};

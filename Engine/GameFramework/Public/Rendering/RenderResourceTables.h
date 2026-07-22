#pragma once

#include "GameFramework/Public/Rendering/RenderAssetHandles.h"
#include "GameFramework/Public/Scene/Materials/MaterialDesc.h"

#include <cstdint>
#include <filesystem>
#include <vector>

struct RenderMaterialTable final
{
	std::vector<MaterialDesc> Values;
	std::uint32_t Generation = 0;
};

struct RenderTextureAsset final
{
	RenderTextureAssetHandle Handle;
	std::filesystem::path Path;
};

struct RenderTextureTable final
{
	std::vector<RenderTextureAsset> Assets;
	std::uint32_t Generation = 0;
};

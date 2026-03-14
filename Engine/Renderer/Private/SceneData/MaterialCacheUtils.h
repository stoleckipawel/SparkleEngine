#pragma once

#include "GameFramework/Public/Assets/MaterialDesc.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

class D3D12Texture;
class TextureManager;
enum class MaterialFallbackTexture : std::uint8_t;

namespace MaterialCacheUtils
{
	std::uint32_t ResolveMaterialId(std::uint32_t materialId, std::size_t materialCount);
	bool MaterialDescSetEquals(const std::vector<MaterialDesc>& left, const std::vector<MaterialDesc>& right);
	const D3D12Texture* ResolveMaterialTexture(
	    TextureManager& textureManager,
	    const std::optional<std::filesystem::path>& texturePath,
	    MaterialFallbackTexture fallbackType);
}  // namespace MaterialCacheUtils
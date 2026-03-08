// =============================================================================
// MaterialCacheUtils.cpp
// =============================================================================

#include "PCH.h"
#include "SceneData/MaterialCacheUtils.h"

#include "D3D12Texture.h"
#include "TextureManager.h"
#include "Renderer/Public/Textures/MaterialFallbackTextures.h"

namespace
{
	bool OptionalPathEquals(const std::optional<std::filesystem::path>& left, const std::optional<std::filesystem::path>& right)
	{
		if (left.has_value() != right.has_value())
		{
			return false;
		}

		return !left.has_value() || *left == *right;
	}

	bool MaterialDescEquals(const MaterialDesc& left, const MaterialDesc& right)
	{
		return left.name == right.name &&
		       left.baseColor.x == right.baseColor.x && left.baseColor.y == right.baseColor.y &&
		       left.baseColor.z == right.baseColor.z && left.baseColor.w == right.baseColor.w &&
		       left.metallic == right.metallic &&
		       left.roughness == right.roughness &&
		       left.f0 == right.f0 &&
		       left.emissiveColor.x == right.emissiveColor.x && left.emissiveColor.y == right.emissiveColor.y &&
		       left.emissiveColor.z == right.emissiveColor.z &&
		       left.alphaMode == right.alphaMode &&
		       left.alphaCutoff == right.alphaCutoff &&
		       OptionalPathEquals(left.albedoTexture, right.albedoTexture) &&
		       OptionalPathEquals(left.normalTexture, right.normalTexture) &&
		       OptionalPathEquals(left.metallicRoughnessTexture, right.metallicRoughnessTexture) &&
		       OptionalPathEquals(left.occlusionTexture, right.occlusionTexture) &&
		       OptionalPathEquals(left.emissiveTexture, right.emissiveTexture);
	}
}

std::uint32_t MaterialCacheUtils::ResolveMaterialId(std::uint32_t materialId, std::size_t materialCount)
{
	if (materialId < materialCount)
	{
		return materialId;
	}

	LOG_WARNING(std::format("MaterialCacheUtils::ResolveMaterialId: Material {} is out of range ({} materials); falling back to material 0",
	                        materialId,
	                        materialCount));
	return 0;
}

bool MaterialCacheUtils::MaterialDescSetEquals(const std::vector<MaterialDesc>& left, const std::vector<MaterialDesc>& right)
{
	if (left.size() != right.size())
	{
		return false;
	}

	for (std::size_t index = 0; index < left.size(); ++index)
	{
		if (!MaterialDescEquals(left[index], right[index]))
		{
			return false;
		}
	}

	return true;
}

const D3D12Texture* MaterialCacheUtils::ResolveMaterialTexture(
	TextureManager& textureManager,
	const std::optional<std::filesystem::path>& texturePath,
	MaterialFallbackTexture fallbackType)
{
	if (texturePath)
	{
		if (D3D12Texture* texture = textureManager.LoadFromPath(*texturePath))
		{
			return texture;
		}
	}

	return textureManager.GetDefaultTexture(MaterialFallbackTextures::GetDefaultTexture(fallbackType));
}
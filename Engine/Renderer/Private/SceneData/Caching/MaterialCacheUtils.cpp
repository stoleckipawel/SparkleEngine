#include "PCH.h"
#include "MaterialCacheUtils.h"

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
		return left.name == right.name && left.baseColor.x == right.baseColor.x && left.baseColor.y == right.baseColor.y &&
		       left.baseColor.z == right.baseColor.z && left.baseColor.w == right.baseColor.w && left.metallic == right.metallic &&
		       left.roughness == right.roughness && left.f0 == right.f0 && left.emissiveColor.x == right.emissiveColor.x &&
		       left.emissiveColor.y == right.emissiveColor.y && left.emissiveColor.z == right.emissiveColor.z &&
		       left.alphaMode == right.alphaMode && left.alphaCutoff == right.alphaCutoff &&
		       OptionalPathEquals(left.albedoTexture, right.albedoTexture) && OptionalPathEquals(left.normalTexture, right.normalTexture) &&
		       OptionalPathEquals(left.metallicRoughnessTexture, right.metallicRoughnessTexture) &&
		       OptionalPathEquals(left.occlusionTexture, right.occlusionTexture) &&
		       OptionalPathEquals(left.emissiveTexture, right.emissiveTexture);
	}
}  // namespace
std::uint32_t MaterialCacheUtils::ResolveMaterialSlot(MaterialHandle materialHandle, std::size_t materialCount)
{
	const std::uint32_t materialSlot = materialHandle.IsValid() ? materialHandle.GetIndex() : 0;

	if (materialSlot < materialCount)
	{
		return materialSlot;
	}

	LOG_WARNING(
	    std::format(
	        "MaterialCacheUtils::ResolveMaterialSlot: Material {} is out of range ({} materials); falling back to material 0",
	        materialSlot,
	        materialCount));
	return 0;
}

bool MaterialCacheUtils::MaterialSnapshotEquals(const MaterialSnapshot& left, const MaterialSnapshot& right)
{
	if (left.materialDescs.size() != right.materialDescs.size())
	{
		return false;
	}

	for (std::size_t index = 0; index < left.materialDescs.size(); ++index)
	{
		if (!MaterialDescEquals(left.materialDescs[index], right.materialDescs[index]))
		{
			return false;
		}
	}

	return true;
}

#include "PCH.h"
#include "MaterialCacheUtils.h"

namespace MaterialCacheUtils
{
	static bool TextureReferenceEquals(const MaterialDesc& left, const MaterialDesc& right, TextureGroup textureGroup)
	{
		const Assets::CookedTextureReference* leftTextureReference = left.FindTextureReference(textureGroup);
		const Assets::CookedTextureReference* rightTextureReference = right.FindTextureReference(textureGroup);
		if ((leftTextureReference == nullptr) != (rightTextureReference == nullptr))
		{
			return false;
		}

		if (!leftTextureReference)
		{
			return true;
		}

		return leftTextureReference->texturePath == rightTextureReference->texturePath;
	}

	static bool MaterialDescEquals(const MaterialDesc& left, const MaterialDesc& right)
	{
		return left.name == right.name && left.baseColor.x == right.baseColor.x && left.baseColor.y == right.baseColor.y &&
		       left.baseColor.z == right.baseColor.z && left.baseColor.w == right.baseColor.w && left.metallic == right.metallic &&
		       left.roughness == right.roughness && left.f0 == right.f0 && left.subsurfaceColor.x == right.subsurfaceColor.x &&
		       left.subsurfaceColor.y == right.subsurfaceColor.y && left.subsurfaceColor.z == right.subsurfaceColor.z &&
		       left.subsurfaceStrength == right.subsurfaceStrength && left.emissiveColor.x == right.emissiveColor.x &&
		       left.emissiveColor.y == right.emissiveColor.y && left.emissiveColor.z == right.emissiveColor.z &&
		       left.alphaMode == right.alphaMode && left.alphaCutoff == right.alphaCutoff && left.doubleSided == right.doubleSided &&
		       TextureReferenceEquals(left, right, TextureGroup::Diffuse) &&
		       TextureReferenceEquals(left, right, TextureGroup::NormalMap) &&
		       TextureReferenceEquals(left, right, TextureGroup::Roughness) &&
		       TextureReferenceEquals(left, right, TextureGroup::Metallic) &&
		       TextureReferenceEquals(left, right, TextureGroup::AmbientOcclusion) &&
		       TextureReferenceEquals(left, right, TextureGroup::Emissive) &&
		       TextureReferenceEquals(left, right, TextureGroup::SubsurfaceColor) &&
		       TextureReferenceEquals(left, right, TextureGroup::SubsurfaceStrength);
	}

	std::uint32_t ResolveMaterialSlot(MaterialHandle materialHandle, std::size_t materialCount)
	{
		const std::uint32_t materialSlot = materialHandle.IsValid() ? materialHandle.GetIndex() : 0;

		if (materialSlot < materialCount)
		{
			return materialSlot;
		}

		return 0;
	}

	bool MaterialTableEquals(const RenderMaterialTable& left, const RenderMaterialTable& right)
	{
		if (left.Generation != right.Generation)
		{
			return false;
		}
		if (left.Values.size() != right.Values.size())
		{
			return false;
		}

		for (std::size_t index = 0; index < left.Values.size(); ++index)
		{
			if (!MaterialDescEquals(left.Values[index], right.Values[index]))
			{
				return false;
			}
		}

		return true;
	}
}  // namespace MaterialCacheUtils

#include "PCH.h"

#include "Assets/Import/MaterialDesc.h"

void MaterialDesc::SetTexturePath(
	MaterialTextureType textureType,
	const std::optional<std::filesystem::path>& texturePath)
{
	if (!texturePath)
	{
		return;
	}

	switch (textureType)
	{
		case MaterialTextureType::Albedo:
			albedoTexture = *texturePath;
			break;
		case MaterialTextureType::Normal:
			normalTexture = *texturePath;
			break;
		case MaterialTextureType::MetallicRoughness:
			metallicRoughnessTexture = *texturePath;
			break;
		case MaterialTextureType::Occlusion:
			occlusionTexture = *texturePath;
			break;
		case MaterialTextureType::Emissive:
			emissiveTexture = *texturePath;
			break;
	}
}
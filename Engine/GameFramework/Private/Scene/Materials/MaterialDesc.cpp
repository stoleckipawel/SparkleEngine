#include "PCH.h"

#include "Scene/Materials/MaterialDesc.h"

void MaterialDesc::SetTexturePath(TextureGroup textureGroup, const std::optional<std::filesystem::path>& texturePath)
{
	if (!texturePath)
	{
		return;
	}

	switch (textureGroup)
	{
		case TextureGroup::Diffuse:
			albedoTexture = *texturePath;
			break;
		case TextureGroup::NormalMap:
			normalTexture = *texturePath;
			break;
		case TextureGroup::Roughness:
			roughnessTexture = *texturePath;
			break;
		case TextureGroup::Metallic:
			metallicTexture = *texturePath;
			break;
		case TextureGroup::AmbientOcclusion:
			occlusionTexture = *texturePath;
			break;
		case TextureGroup::Emissive:
			emissiveTexture = *texturePath;
			break;
		case TextureGroup::SubsurfaceColor:
			subsurfaceColorTexture = *texturePath;
			break;
		case TextureGroup::SubsurfaceStrength:
			subsurfaceStrengthTexture = *texturePath;
			break;
		case TextureGroup::Default:
		case TextureGroup::HdrColor:
			break;
	}
}
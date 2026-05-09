#include "PCH.h"
#include "SceneData/MaterialData.h"
#include "Scene/Materials/MaterialDesc.h"

MaterialData MaterialData::FromDesc(const MaterialDesc& desc)
{
	MaterialData mat = {};
	mat.baseColor = desc.baseColor;
	mat.metallic = desc.metallic;
	mat.roughness = desc.roughness;
	mat.f0 = desc.f0;
	mat.subsurfaceColor = desc.subsurfaceColor;
	mat.subsurfaceStrength = desc.subsurfaceStrength;
	mat.emissiveColor = desc.emissiveColor;
	mat.alphaMode = static_cast<std::uint32_t>(desc.alphaMode);
	mat.alphaCutoff = desc.alphaCutoff;

	auto setTextureFlag = [&mat](const std::optional<std::filesystem::path>& texturePath, TextureGroup textureGroup)
	{
		if (texturePath)
		{
			mat.textureFlags |= GetTextureGroupFlag(textureGroup);
		}
	};

	setTextureFlag(desc.albedoTexture, TextureGroup::Diffuse);
	setTextureFlag(desc.normalTexture, TextureGroup::NormalMap);
	setTextureFlag(desc.roughnessTexture, TextureGroup::Roughness);
	setTextureFlag(desc.metallicTexture, TextureGroup::Metallic);
	setTextureFlag(desc.occlusionTexture, TextureGroup::AmbientOcclusion);
	setTextureFlag(desc.emissiveTexture, TextureGroup::Emissive);
	setTextureFlag(desc.subsurfaceColorTexture, TextureGroup::SubsurfaceColor);
	setTextureFlag(desc.subsurfaceStrengthTexture, TextureGroup::SubsurfaceStrength);

	return mat;
}

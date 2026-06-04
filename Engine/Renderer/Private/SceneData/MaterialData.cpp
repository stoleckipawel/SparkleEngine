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
	mat.doubleSided = desc.doubleSided;

	auto setTextureFlag = [&mat, &desc](TextureGroup textureGroup)
	{
		if (desc.HasTextureReference(textureGroup))
		{
			mat.textureFlags |= GetTextureGroupFlag(textureGroup);
		}
	};

	setTextureFlag(TextureGroup::Diffuse);
	setTextureFlag(TextureGroup::NormalMap);
	setTextureFlag(TextureGroup::Roughness);
	setTextureFlag(TextureGroup::Metallic);
	setTextureFlag(TextureGroup::AmbientOcclusion);
	setTextureFlag(TextureGroup::Emissive);
	setTextureFlag(TextureGroup::SubsurfaceColor);
	setTextureFlag(TextureGroup::SubsurfaceStrength);

	return mat;
}

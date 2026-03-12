#include "PCH.h"
#include "Renderer/Public/SceneData/MaterialData.h"
#include "Assets/MaterialDesc.h"

MaterialData MaterialData::FromDesc(const MaterialDesc& desc)
{
	MaterialData mat = {};
	mat.baseColor = desc.baseColor;
	mat.metallic = desc.metallic;
	mat.roughness = desc.roughness;
	mat.f0 = desc.f0;
	mat.emissiveColor = desc.emissiveColor;
	mat.alphaMode = static_cast<std::uint32_t>(desc.alphaMode);
	mat.alphaCutoff = desc.alphaCutoff;

	if (desc.albedoTexture)
		mat.textureFlags |= MaterialTextureFlags::Albedo;
	if (desc.normalTexture)
		mat.textureFlags |= MaterialTextureFlags::Normal;
	if (desc.metallicRoughnessTexture)
		mat.textureFlags |= MaterialTextureFlags::MetallicRoughness;
	if (desc.occlusionTexture)
		mat.textureFlags |= MaterialTextureFlags::Occlusion;
	if (desc.emissiveTexture)
		mat.textureFlags |= MaterialTextureFlags::Emissive;

	return mat;
}

#include "PCH.h"
#include "Scene/Materials/MaterialData.h"
#include "Scene/Materials/MaterialDesc.h"

#include <cmath>

MaterialGpuHandle::operator bool() const noexcept
{
	return Index != UINT32_MAX && Generation != 0u;
}

bool MaterialGpuHandle::operator==(const MaterialGpuHandle&) const noexcept = default;

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

	auto setTexture = [&mat, &desc](TextureGroup textureGroup, std::uint32_t slot)
	{
		if (const Assets::CookedTextureReference* texture = desc.FindTextureReference(textureGroup))
		{
			mat.textureFlags |= GetTextureGroupFlag(textureGroup);
			const float cosine = std::cos(texture->mapping.Rotation);
			const float sine = std::sin(texture->mapping.Rotation);
			mat.materialTextureMappings[slot] = MaterialTextureMappingData{
			    .UvLinear = {cosine * texture->mapping.Scale.x,
			                 -sine * texture->mapping.Scale.y,
			                 sine * texture->mapping.Scale.x,
			                 cosine * texture->mapping.Scale.y},
			    .UvOffset = texture->mapping.Offset,
			    .Strength = texture->mapping.Strength,
			    .AddressModes = static_cast<std::uint32_t>(texture->mapping.AddressU)
			        | (static_cast<std::uint32_t>(texture->mapping.AddressV) << 2u)};
		}
	};

	setTexture(TextureGroup::Diffuse, MaterialTextureSlots::BaseColor);
	setTexture(TextureGroup::NormalMap, MaterialTextureSlots::Normal);
	setTexture(TextureGroup::Roughness, MaterialTextureSlots::Roughness);
	setTexture(TextureGroup::Metallic, MaterialTextureSlots::Metallic);
	setTexture(TextureGroup::AmbientOcclusion, MaterialTextureSlots::Occlusion);
	setTexture(TextureGroup::Emissive, MaterialTextureSlots::Emissive);
	setTexture(TextureGroup::SubsurfaceColor, MaterialTextureSlots::SubsurfaceColor);
	setTexture(TextureGroup::SubsurfaceStrength, MaterialTextureSlots::SubsurfaceStrength);

	return mat;
}

PerObjectPSConstantBufferData MaterialData::ToPerObjectPSData() const
{
	PerObjectPSConstantBufferData data{};
	data.BaseColor = baseColor;
	data.EmissiveColor = emissiveColor;
	data.Metallic = metallic;
	data.Roughness = roughness;
	data.F0 = f0;
	data.AlphaCutoff = alphaCutoff;
	data.AlphaMode = alphaMode;
	data.TextureFlags = textureFlags;
	data.SubsurfaceColor = subsurfaceColor;
	data.SubsurfaceStrength = subsurfaceStrength;
	return data;
}

#include "PCH.h"
#include "Scene/Materials/MaterialData.h"
#include "Scene/Materials/MaterialDesc.h"

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

#include "PCH.h"

#include "Scene/Materials/MaterialDesc.h"

#include <utility>

static bool IsMaterialTextureGroup(TextureGroup textureGroup) noexcept
{
	switch (textureGroup)
	{
		case TextureGroup::Diffuse:
		case TextureGroup::NormalMap:
		case TextureGroup::Roughness:
		case TextureGroup::Metallic:
		case TextureGroup::AmbientOcclusion:
		case TextureGroup::Emissive:
		case TextureGroup::SubsurfaceColor:
		case TextureGroup::SubsurfaceStrength:
			return true;
		case TextureGroup::Default:
		case TextureGroup::HdrColor:
			return false;
	}

	return false;
}

void MaterialDesc::AddTextureReference(Assets::CookedTextureReference textureReference)
{
	if (!textureReference.IsValid() || !IsMaterialTextureGroup(textureReference.textureGroup))
	{
		return;
	}

	for (Assets::CookedTextureReference& existingTextureReference : textureReferences)
	{
		if (existingTextureReference.textureGroup == textureReference.textureGroup)
		{
			existingTextureReference = std::move(textureReference);
			return;
		}
	}

	textureReferences.push_back(std::move(textureReference));
}

const Assets::CookedTextureReference* MaterialDesc::FindTextureReference(TextureGroup textureGroup) const noexcept
{
	for (const Assets::CookedTextureReference& textureReference : textureReferences)
	{
		if (textureReference.textureGroup == textureGroup)
		{
			return &textureReference;
		}
	}

	return nullptr;
}

bool MaterialDesc::HasTextureReference(TextureGroup textureGroup) const noexcept
{
	return FindTextureReference(textureGroup) != nullptr;
}